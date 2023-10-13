#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>




// Function to extract an individual frame from an MP4 file and store it as raw RGB data
int extractFrame(const char *inputFilename, int frameNumber, unsigned char **frameData, int *width, int *height) {

    // Check if the file exists
    if (access(inputFilename, F_OK) != -1) {
        printf("File exists: %s\n", inputFilename);

        // Check if you have read permissions
        if (access(inputFilename, R_OK) == 0) {
            printf("You have read permissions for %s\n", inputFilename);
        } else {
            printf("You do not have read permissions for %s\n", inputFilename);
        }
    } else {
        printf("File does not exist: %s\n", inputFilename);
    }

    AVFormatContext *formatCtx = NULL;
    AVCodecContext *codecCtx = NULL;
    AVStream *videoStream = NULL;
    AVPacket packet;
    AVFrame *frame = NULL;
    struct SwsContext *swsCtx = NULL;
    int ret;

    // Initialize FFmpeg
    avformat_network_init();

    // Open the input MP4 file
    if (avformat_open_input(&formatCtx, inputFilename, NULL, NULL) != 0) {
        fprintf(stderr, "Failed to open input file.\n");
        return -1;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        fprintf(stderr, "Failed to retrieve stream information.\n");
        return -1;
    }

    // Find the video stream
    for (int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = formatCtx->streams[i];
            break;
        }
    }

    if (!videoStream) {
        fprintf(stderr, "No video stream found in the input file.\n");
        return -1;
    }

    // Find the decoder for the video stream
    AVCodec *codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found.\n");
        return -1;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        fprintf(stderr, "Failed to allocate codec context.\n");
        return -1;
    }

    if (avcodec_parameters_to_context(codecCtx, videoStream->codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to context.\n");
        return -1;
    }

    // Open the codec
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec.\n");
        return -1;
    }

    // Allocate a frame
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Failed to allocate frame.\n");
        return -1;
    }

    // Seek to the desired frame
    int seek_frame = frameNumber;
    if (av_seek_frame(formatCtx, videoStream->index, seek_frame, AVSEEK_FLAG_BACKWARD) < 0) {
        fprintf(stderr, "Failed to seek to frame %d.\n", seek_frame);
        return -1;
    }

    // Read and decode frames until the specified frame is reached
    while (av_read_frame(formatCtx, &packet) >= 0) {
        if (packet.stream_index == videoStream->index) {
            if (avcodec_send_packet(codecCtx, &packet) < 0) {
                fprintf(stderr, "Failed to send packet to codec.\n");
                av_packet_unref(&packet);
                break;
            }

            ret = avcodec_receive_frame(codecCtx, frame);

            if (ret == AVERROR(EAGAIN)) {
                // This is not an error; it indicates that more packets are needed.
                av_packet_unref(&packet);
                continue;
            } else if (ret < 0) {
                fprintf(stderr, "Failed to receive frame from codec: %s\n", av_err2str(ret));
                av_packet_unref(&packet);
                return -1;
            }

            *width = codecCtx->width;
            *height = codecCtx->height;
            *frameData = (unsigned char *)malloc(3 * codecCtx->width * codecCtx->height);

            sws_scale(swsCtx, (const uint8_t *const *)frame->data, frame->linesize, 0, codecCtx->height, (uint8_t * const *)frameData, &codecCtx->width);

            av_packet_unref(&packet);
            av_frame_free(&frame);
            avcodec_close(codecCtx);
            avformat_close_input(&formatCtx);
            sws_freeContext(swsCtx);

            return 0;
        }
        av_packet_unref(&packet);
    }

    // Cleanup in case of failure
    av_frame_free(&frame);
    avcodec_close(codecCtx);
    avformat_close_input(&formatCtx);
    sws_freeContext(swsCtx);

    fprintf(stderr, "Failed to extract frame from the input file.\n");
    return -1;
}