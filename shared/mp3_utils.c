#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#define MAX_DATA_SIZE 50000  // Maximum buffer size for incoming messages
typedef struct {
    AVFormatContext *inputFormatContext;
    AVCodecContext *mp3CodecContext;
    int current_chunk_index;
    int chunk_size;
    FILE *outputFile;
    int chunk_duration_ms;
    int byte_threshold;
    AVPacket partial_packet;
} MP3File;

// Function to open an MP3 file and initialize the codec
int open_mp3(MP3File *mp3, const char *inputFileName) {
    avformat_network_init();

    mp3->inputFormatContext = NULL;
    mp3->mp3CodecContext = NULL;
    mp3->current_chunk_index = 1;
    mp3->chunk_size = MAX_DATA_SIZE;
    mp3->outputFile = NULL;
    mp3->chunk_duration_ms = 0;
    mp3->byte_threshold = 0;

    // Open input file
    int ret = avformat_open_input(&mp3->inputFormatContext, inputFileName, NULL, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open input file\n");
        return ret;
    }

    // Find stream information
    ret = avformat_find_stream_info(mp3->inputFormatContext, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find stream information\n");
        return ret;
    }

    // Find the decoder
    AVCodec *codec = avcodec_find_decoder(mp3->inputFormatContext->streams[0]->codecpar->codec_id);
    mp3->mp3CodecContext = avcodec_alloc_context3(codec);

    // Copy codec parameters to codec context
    ret = avcodec_parameters_to_context(mp3->mp3CodecContext, mp3->inputFormatContext->streams[0]->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters to codec context\n");
        return ret;
    }

    // Open the codec
    ret = avcodec_open2(mp3->mp3CodecContext, codec, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open codec\n");
        return ret;
    }

    return 0;
}

// Function to read and encode the next chunk of audio data
int mp3_read_chunk(MP3File *mp3, unsigned char *output, struct timeval *chunk_duration, int *length) {
    AVPacket packet;
    int ret;
    int buf_ptr = 0;

    mp3->byte_threshold = 0;
    mp3->chunk_duration_ms = 0;

    while (av_read_frame(mp3->inputFormatContext, &packet) >= 0) {
        if (mp3->byte_threshold + packet.size <= mp3->chunk_size) {
            // Write the packet data to the chunk file
            memcpy(&output[buf_ptr], packet.data, packet.size);
            buf_ptr += packet.size;

            // Update byte threshold and duration
            mp3->byte_threshold += packet.size;
            mp3->chunk_duration_ms += packet.duration * 1000 * av_q2d(mp3->inputFormatContext->streams[0]->time_base);
        } else {
            // Save the partial packet for the next chunk
            av_packet_ref(&mp3->partial_packet, &packet);
            chunk_duration->tv_usec = mp3->chunk_duration_ms * 1000;
            chunk_duration->tv_sec = chunk_duration->tv_usec / 1000000;
            chunk_duration->tv_usec %= 1000000;
            mp3->current_chunk_index++;
            mp3->byte_threshold = 0;
            av_packet_unref(&packet);
            *length = buf_ptr;
            return 0;
        }
        av_packet_unref(&packet);
    }

    chunk_duration->tv_usec = mp3->chunk_duration_ms * 1000;
    chunk_duration->tv_sec = chunk_duration->tv_usec / 1000000;
    chunk_duration->tv_usec %= 1000000;
    mp3->current_chunk_index++;
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
void close_mp3(MP3File *mp3) {
    if (mp3->outputFile != NULL) {
        fclose(mp3->outputFile);
    }

    if (mp3->mp3CodecContext != NULL) {
        avcodec_free_context(&mp3->mp3CodecContext);
    }

    if (mp3->inputFormatContext != NULL) {
        avformat_close_input(&mp3->inputFormatContext);
    }
}

// Function to reset the MP3 reader
void reset_mp3_reader(MP3File *mp3, char *fileName) {
    close_mp3(mp3);
    av_packet_unref(&mp3->partial_packet);

    // Reset global variables in the struct
    mp3->current_chunk_index = 1;
    mp3->chunk_duration_ms = 0;
    mp3->byte_threshold = 0;

    // Reopen the input file
    int ret = open_mp3(mp3, fileName);
    if (ret < 0) {
        printf("Failed to reopen MP3\n");
    }
}
