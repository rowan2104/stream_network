//Created by Rowan Barr
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

typedef struct {
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    AVFrame* frame;
    int videoStreamIndex;
    AVCodecParameters* codecpar;
    struct SwsContext* swsContext;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
} MP4File;

int open_input(MP4File* mp4, const char* filename, unsigned char* decodedFrameBuf[]) {
    // Initialize FFmpeg
    avformat_network_init();
    mp4->formatContext = NULL;
    mp4->codecContext = NULL;
    mp4->frame = NULL;
    mp4->videoStreamIndex = -1;
    mp4->codecpar = NULL;
    mp4->swsContext = NULL;
    mp4->width = 0;
    mp4->height = 0;
    mp4->fps = 0;

    // Open the input video file
    if (avformat_open_input(&mp4->formatContext, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Error: Could not open the input file.\n");
        return -1;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(mp4->formatContext, NULL) < 0) {
        fprintf(stderr, "Error: Could not find stream information.\n");
        return -1;
    }

    // Find the video stream
    for (int i = 0; i < mp4->formatContext->nb_streams; i++) {
        if (mp4->formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mp4->videoStreamIndex = i;
            mp4->codecpar = mp4->formatContext->streams[i]->codecpar;
            break;
        }
    }

    if (mp4->videoStreamIndex == -1) {
        fprintf(stderr, "Error: Could not find a video stream in the input file.\n");
        return -1;
    }

    // Allocate a codec context and open the codec
    mp4->codecContext = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(mp4->codecContext, mp4->formatContext->streams[mp4->videoStreamIndex]->codecpar);
    AVCodec* codec = avcodec_find_decoder(mp4->codecContext->codec_id);
    if (!codec) {
        fprintf(stderr, "Error: Unsupported codec.\n");
        return -1;
    }

    if (avcodec_open2(mp4->codecContext, codec, NULL) < 0) {
        fprintf(stderr, "Error: Could not open the codec.\n");
        return -1;
    }

    // Allocate a frame
    mp4->frame = av_frame_alloc();

    for (int i = 0; i < 6; i++) {
        decodedFrameBuf[i] = (unsigned char*)av_malloc(3 * mp4->codecContext->width * mp4->codecContext->height);
    }

    // Create swsContext
    mp4->swsContext = sws_getContext(
            mp4->codecContext->width, mp4->codecContext->height, mp4->codecpar->format,
            mp4->codecContext->width, mp4->codecContext->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, NULL, NULL, NULL
    );

    // Fill in the width, height, and fps variables
    mp4->width = mp4->codecContext->width;
    mp4->height = mp4->codecContext->height;
    mp4->fps = av_q2d(mp4->formatContext->streams[mp4->videoStreamIndex]->avg_frame_rate);

    return 0;
}

void close_mp4(MP4File* mp4) {
    if (mp4->formatContext) {
        avformat_close_input(&mp4->formatContext);
        mp4->formatContext = NULL;
    }

    if (mp4->codecContext) {
        avcodec_free_context(&mp4->codecContext);
        mp4->codecContext = NULL;
    }

    if (mp4->frame) {
        av_frame_free(&mp4->frame);
        mp4->frame = NULL;
    }

    if (mp4->swsContext) {
        sws_freeContext(mp4->swsContext);
        mp4->swsContext = NULL;
    }

    // Reset the global variables
    mp4->videoStreamIndex = -1;
    mp4->codecpar = NULL;
}

int reset_reader(MP4File* mp4) {
    // Seek to the beginning of the video stream
    if (av_seek_frame(mp4->formatContext, mp4->videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD) < 0) {
        fprintf(stderr, "Error: Could not seek to the beginning of the video stream.\n");
        return -1;
    }

    return 0;
}

int decode_frames(MP4File* mp4, int batch_size, unsigned char* decodedFrameBuf[]) {
    AVPacket packet;
    int frames_decoded = 0;
    while (av_read_frame(mp4->formatContext, &packet) >= 0) {
        if (packet.stream_index == mp4->videoStreamIndex) {
            // Decode video frame
            if (avcodec_send_packet(mp4->codecContext, &packet) == 0) {
                while (avcodec_receive_frame(mp4->codecContext, mp4->frame) == 0) {
                    // Store the frame in the buffer
                    if (frames_decoded < batch_size) {
                        // Here, you can copy the frame data to the buffer as needed
                        // The frame data is available in mp4->frame->data[0], mp4->frame->linesize[0]
                        uint8_t* frameDataArray[1] = { decodedFrameBuf[frames_decoded] };
                        int linesize[1] = { mp4->frame->width * 3 };

                        sws_scale(mp4->swsContext, (const uint8_t* const*)mp4->frame->data, mp4->frame->linesize, 0, mp4->frame->height, frameDataArray, linesize);
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
        reset_reader(mp4);
        return -1; // End of file
    } else {
        printf("ret2!\n");
        return -2; // Error
    }
}
