#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>





int extractFrame(const char *inputFilename, int frameNum, uint8_t **frameData, int *width, int *height) {
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

    int currentFrame = 0;
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStream) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                int rc = avcodec_receive_frame(codecContext, frame);
                if (rc == 0) {
                    if (currentFrame == frameNum) {
                        *width = frame->width;
                        *height = frame->height;

                        struct SwsContext *swsContext = sws_getContext(
                                frame->width, frame->height, codecpar->format,
                                frame->width, frame->height, AV_PIX_FMT_RGB24,
                                SWS_BILINEAR, NULL, NULL, NULL);

                        *frameData = (uint8_t *)malloc(frame->width * frame->height * 3);

                        uint8_t *frameDataArray[1] = { *frameData };
                        int linesize[1] = { frame->width * 3 };

                        sws_scale(swsContext, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height, frameDataArray, linesize);

                        sws_freeContext(swsContext);
                        av_packet_unref(packet);
                        av_frame_free(&frame);
                        avcodec_free_context(&codecContext);
                        avformat_close_input(&formatContext);
                        av_packet_free(&packet);


                        // Vertically flip the image and swap R and B channels
                        uint8_t *flippedFrameData = malloc((*width) * (*height) * 3);
                        int srcRow, destRow;

                        for (srcRow = 0, destRow = (*height - 1); srcRow < *height; srcRow++, destRow--) {
                            for (int col = 0; col < *width; col++) {
                                uint8_t *srcPixel = &(*frameData)[(srcRow * (*width) + col) * 3];
                                uint8_t *destPixel = &flippedFrameData[(destRow * (*width) + col) * 3];

                                // Swap R and B channels
                                destPixel[0] = srcPixel[2];
                                destPixel[1] = srcPixel[1];
                                destPixel[2] = srcPixel[0];
                            }
                        }
                        free(*frameData);
                        *frameData = flippedFrameData;

                        return 0;
                    }
                    currentFrame++;
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
