#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>

// Global variables to maintain state
AVFormatContext *inputFormatContext = NULL;
AVCodecContext *mp3CodecContext = NULL;
int current_chunk_index = 1;
int chunk_size = 65000;
FILE *outputFile = NULL;
int chunk_duration_ms = 0; // Duration of the current chunk in milliseconds
int byte_threshold = 0;
AVPacket partial_packet;

// Function to open an MP3 file and initialize the codec
int open_mp3(const char *inputFileName) {
    avformat_network_init();

    // Open input file
    int ret = avformat_open_input(&inputFormatContext, inputFileName, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open input file\n");
        return ret;
    }

    // Find stream information
    ret = avformat_find_stream_info(inputFormatContext, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find stream information\n");
        return ret;
    }

    // Find the decoder
    AVCodec *codec = avcodec_find_decoder(inputFormatContext->streams[0]->codecpar->codec_id);
    mp3CodecContext = avcodec_alloc_context3(codec);

    // Copy codec parameters to codec context
    ret = avcodec_parameters_to_context(mp3CodecContext, inputFormatContext->streams[0]->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters to codec context\n");
        return ret;
    }

    // Open the codec
    ret = avcodec_open2(mp3CodecContext, codec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open codec\n");
        return ret;
    }

    return 0;
}



void reset_mp3_reader() {
    // Close the current input and output
    avformat_close_input(&inputFormatContext);
    avcodec_free_context(&mp3CodecContext);
    av_packet_unref(&partial_packet);
    if (outputFile) {
        fclose(outputFile);
        outputFile = NULL;
    }

    // Reset global variables
    current_chunk_index = 1;
    chunk_duration_ms = 0;
    byte_threshold = 0;

    // Reopen the input file
    const char* inputFileName = "your_input_file.mp3";  // Replace with your actual file name
    int ret = open_mp3(inputFileName);
    if (ret < 0) {
        // Handle error
    }
}


// Function to read and encode the next chunk of audio data
int mp3_read_chunk(unsigned char * output, struct timeval *chunk_duration, int * length) {
    AVPacket packet;
    int ret;
    int buf_ptr = 0;

    byte_threshold = 0;
    chunk_duration_ms = 0;

    while (av_read_frame(inputFormatContext, &packet) >= 0) {
        if (byte_threshold + packet.size <= chunk_size) {
            // Write the packet data to the chunk file
            memcpy(&output[buf_ptr], packet.data, packet.size);
            buf_ptr += packet.size;

            // Update byte threshold and duration
            byte_threshold += packet.size;
            chunk_duration_ms += packet.duration * 1000 * av_q2d(inputFormatContext->streams[0]->time_base);
        } else {
            // Save the partial packet for the next chunk
            av_packet_ref(&partial_packet, &packet);
            chunk_duration->tv_usec = chunk_duration_ms * 1000;
            chunk_duration->tv_sec = chunk_duration->tv_usec / 1000000;
            chunk_duration->tv_usec %= 1000000;
            current_chunk_index++;
            byte_threshold = 0;
            av_packet_unref(&packet);
            *length = buf_ptr;
            return 0;
        }
        av_packet_unref(&packet);
    }

    chunk_duration->tv_usec = chunk_duration_ms * 1000;
    chunk_duration->tv_sec = chunk_duration->tv_usec / 1000000;
    chunk_duration->tv_usec %= 1000000;
    current_chunk_index++;

    return -1; // Signal that there are no more frames
}

// Function to save the buffer as an MP3 file
int save_mp3(const char *outputFileName, uint8_t *data, int size) {
    FILE *outputFile = fopen(outputFileName, "wb");
    if (!outputFile) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open output file\n");
        return -1;
    }

    fwrite(data, 1, size, outputFile);
    fclose(outputFile);
    return 0;
}

// Function to close the MP3 file and cleanup
void close_mp3() {
    if (outputFile != NULL) {
        fclose(outputFile);
    }

    if (mp3CodecContext != NULL) {
        avcodec_free_context(&mp3CodecContext);
    }

    if (inputFormatContext != NULL) {
        avformat_close_input(&inputFormatContext);
    }
}
