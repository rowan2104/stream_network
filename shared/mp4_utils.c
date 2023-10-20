#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

AVFormatContext* formatContext = NULL;
AVCodecContext* codecContext = NULL;
AVFrame* frame = NULL;
int videoStreamIndex = -1;
AVCodecParameters* codecpar = NULL; // Global codecpar

struct SwsContext* swsContext; // Global swsContext

int open_input(const char* filename, unsigned char * decodedFrameBuf[], int width, int height) {
    // Initialize FFmpeg
    avformat_network_init();


    // Open the input video file
    if (avformat_open_input(&formatContext, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Error: Could not open the input file.\n");
        return -1;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        fprintf(stderr, "Error: Could not find stream information.\n");
        return -1;
    }

    // Find the video stream
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            codecpar = formatContext->streams[i]->codecpar;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        fprintf(stderr, "Error: Could not find a video stream in the input file.\n");
        return -1;
    }

    // Allocate a codec context and open the codec
    codecContext = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codecContext, formatContext->streams[videoStreamIndex]->codecpar);
    AVCodec* codec = avcodec_find_decoder(codecContext->codec_id);
    if (!codec) {
        fprintf(stderr, "Error: Unsupported codec.\n");
        return -1;
    }

    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        fprintf(stderr, "Error: Could not open the codec.\n");
        return -1;
    }

    // Allocate a frame
    frame = av_frame_alloc();

    for (int i = 0; i < 6; i++) {
        decodedFrameBuf[i] = (unsigned char*)av_malloc(3 * codecContext->width * codecContext->height);
    }

    // Create swsContext
    swsContext = sws_getContext(
            codecContext->width, codecContext->height, codecpar->format,
            codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, NULL, NULL, NULL
    );

    return 0;
}

void close_mp4() {
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = NULL;
    }

    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = NULL;
    }

    if (frame) {
        av_frame_free(&frame);
        frame = NULL;
    }

    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = NULL;
    }


    // Reset the global variables
    videoStreamIndex = -1;
    codecpar = NULL;
}

int reset_reader() {
    // Seek to the beginning of the video stream
    if (av_seek_frame(formatContext, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD) < 0) {
        fprintf(stderr, "Error: Could not seek to the beginning of the video stream.\n");
        return -1;
    }

    return 0;
}

int decode_frames(int batch_size, unsigned char* decodedFrameBuf[]) {
    AVPacket packet;
    int frames_decoded = 0;

    while (av_read_frame(formatContext, &packet) >= 0) {
        if (packet.stream_index == videoStreamIndex) {
            // Decode video frame
            if (avcodec_send_packet(codecContext, &packet) == 0) {
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    // Store the frame in the buffer
                    if (frames_decoded < batch_size) {
                        // Here, you can copy the frame data to the buffer as needed
                        // The frame data is available in frame->data[0], frame->linesize[0]
                        uint8_t *frameDataArray[1] = { decodedFrameBuf[frames_decoded] };
                        int linesize[1] = { frame->width * 3 };

                        sws_scale(swsContext, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height, frameDataArray, linesize);
                        frames_decoded++;
                    }

                    if (frames_decoded >= batch_size) {
                        av_packet_unref(&packet);
                        return 0; // Finished decoding batch
                    }
                }
            }
        }
        av_packet_unref(&packet);
    }

    if (frames_decoded > 0) {
        return 0; // Finished decoding batch
    } else if (frames_decoded == 0) {
        reset_reader();
        return -1; // End of file
    } else {
        return -2; // Error
    }
}





int getDetails(const char *inputFilename, int * fps, int *width, int *height) {
    avformat_network_init();

    AVFormatContext *formatContext = avformat_alloc_context();
    if (!formatContext) {
        fprintf(stderr, "Failed to allocate format context\n");
        return -1;
    }

    if (avformat_open_input(&formatContext, inputFilename, NULL, NULL) != 0) {
        fprintf(stderr, "Failed to open input file\n");
        return -1;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        fprintf(stderr, "Failed to find stream information\n");
        return -1;
    }

    int videoStream = -1;
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        fprintf(stderr, "No video stream found in the input file\n");
        return -1;
    }

    AVRational frameRate = formatContext->streams[videoStream]->avg_frame_rate;
    *fps = av_q2d(frameRate);

    AVCodecParameters *codecpar = formatContext->streams[videoStream]->codecpar;
    AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);

    if (!codec) {
        fprintf(stderr, "Unsupported codec\n");
        return -1;
    }

    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        fprintf(stderr, "Failed to allocate codec context\n");
        return -1;
    }

    if (avcodec_parameters_to_context(codecContext, codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to codec context\n");
        return -1;
    }

    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        return -1;
    }

    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    if (!packet) {
        fprintf(stderr, "Failed to allocate packet\n");
        return -1;
    }

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStream) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                int rc = avcodec_receive_frame(codecContext, frame);
                if (rc == 0) {
                    *width = frame->width;
                    *height = frame->height;

                    av_packet_unref(packet);
                    av_frame_free(&frame);
                    avcodec_free_context(&codecContext);
                    avformat_close_input(&formatContext);
                    av_packet_free(&packet);

                    return 0;
                }
                else if (rc == AVERROR(EAGAIN)) {
                    // EAGAIN means no output frame available at this moment, continue reading
                    continue;
                }
                else {
                    fprintf(stderr, "Failed to receive frame\n");
                    return -1;
                }
            }
        }
        av_packet_unref(packet);
    }

    return -1;
}