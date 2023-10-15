#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ffms.h>

int extractFrame(const char *inputFilename, int frameNum, uint8_t **frameData, int *fps, int *width, int *height) {
    /* Initialize the library. */
    FFMS_Init(0, 0);

    /* Index the source file. Note that this example does not index any audio tracks. */
    char errmsg[1024];
    FFMS_ErrorInfo errinfo;
    errinfo.Buffer      = errmsg;
    errinfo.BufferSize  = sizeof(errmsg);
    errinfo.ErrorType   = FFMS_ERROR_SUCCESS;
    errinfo.SubType     = FFMS_ERROR_SUCCESS;
    const char *sourcefile = inputFilename;

    FFMS_Indexer *indexer = FFMS_CreateIndexer(sourcefile, &errinfo);
    if (indexer == NULL) {
        printf("Error1!\n");
    }

    FFMS_Index *index = FFMS_DoIndexing2(indexer, FFMS_IEH_ABORT, &errinfo);
    if (index == NULL) {
        printf("Error2!\n");
    }

    /* Retrieve the track number of the first video track */
    int trackno = FFMS_GetFirstTrackOfType(index, FFMS_TYPE_VIDEO, &errinfo);
    if (trackno < 0) {
        printf("Error3!\n");
    }

    /* We now have enough information to create the video source object */
    FFMS_VideoSource *videosource = FFMS_CreateVideoSource(sourcefile, trackno, index, 1, FFMS_SEEK_NORMAL, &errinfo);
    if (videosource == NULL) {
        printf("Error4!\n");
    }

    /* Since the index is copied into the video source object upon its creation,
    we can and should now destroy the index object. */
    FFMS_DestroyIndex(index);

    /* Retrieve video properties so we know what we're getting.
    As the lack of the errmsg parameter indicates, this function cannot fail. */
    const FFMS_VideoProperties *videoprops = FFMS_GetVideoProperties(videosource);

    /* Now you may want to do something with the info, like check how many frames the video has */
    int num_frames = videoprops->NumFrames;

    /* Get the first frame for examination so we know what we're getting. This is required
    because resolution and colorspace is a per frame property and NOT global for the video. */
    const FFMS_Frame *propframe = FFMS_GetFrame(videosource, 0, &errinfo);

    /* Now you may want to do something with the info; particularly interesting values are:
    propframe->EncodedWidth; (frame width in pixels)
    propframe->EncodedHeight; (frame height in pixels)
    propframe->EncodedPixelFormat; (actual frame colorspace)
    */

    /* If you want to change the output colorspace or resize the output frame size,
    now is the time to do it. IMPORTANT: This step is also required to prevent
    resolution and colorspace changes midstream. You can always tell a frame's
    original properties by examining the Encoded* properties in FFMS_Frame. */
    /* See libavutil/pixfmt.h for the list of pixel formats/colorspaces.
    To get the name of a given pixel format, strip the leading PIX_FMT_
    and convert to lowercase. For example, PIX_FMT_YUV420P becomes "yuv420p". */

    /* A -1 terminated list of the acceptable output formats. */
    int pixfmts[2];
    pixfmts[0] = FFMS_GetPixFmt("bgr24");
    pixfmts[1] = -1;

    if (FFMS_SetOutputFormatV2(videosource, pixfmts, propframe->EncodedWidth, propframe->EncodedHeight,
                               FFMS_RESIZER_BICUBIC, &errinfo)) {
        /* handle error */
    }

    /* now we're ready to actually retrieve the video frames */
    int framenumber = frameNum; /* valid until next call to FFMS_GetFrame* on the same video object */
    const FFMS_Frame *curframe = FFMS_GetFrame(videosource, framenumber, &errinfo);
    if (curframe == NULL) {
        /* handle error */
    }

    /* Allocate memory for the frame data buffer in BGR format */
    int bufferSize = curframe->EncodedWidth * curframe->EncodedHeight * 3; // 3 bytes per pixel (BGR)
    *frameData = (uint8_t*)malloc(bufferSize);
    if (*frameData == NULL) {
        /* handle memory allocation error */
    }


    /* Copy the pixel data to the frameData buffer */
    int stride = curframe->EncodedWidth * 3; // 3 bytes per pixel (BGR)
    for (int i = 0; i < curframe->EncodedHeight; i++) {
        memcpy(*frameData + i * curframe->EncodedWidth * 3, curframe->Data[0] + i * curframe->Linesize[0], curframe->EncodedWidth * 3);
    }


    /* Set the width, height, and fps values */
    *width = curframe->EncodedWidth;
    *height = curframe->EncodedHeight;
    *fps = videoprops->NumFrames;

    /* now it's time to clean up */
    FFMS_DestroyVideoSource(videosource);

    /* uninitialize the library. */
    FFMS_Deinit();

    return 0;
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