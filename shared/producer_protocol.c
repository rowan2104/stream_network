//
// Created by rowan on 01/10/2023.
//

#include "protocol_constants.h"


int prot_request_connect(unsigned char * buf, char id[3]){
    buf[0] = CONTROL_REQUEST_CONNECT;
    memcpy(&buf[1], id, 3);
    int length = 4;
    return length;
}

int prot_video_frame(unsigned char * buf, char * filename){
    buf[0] = CONTROL_VIDEO_FRAME;
    BMPImage *theImage = read_BMP_image(filename);
    memcpy(&buf[1], theImage->pixelData, theImage->size);
    return theImage->size + 1;
}

int prot_text_frame(unsigned char * buf, char * filename){
    buf[0] = CONTROL_TEXT_FRAME;
    strcpy(buf + 1, readFileIntoString(filename));
    printf("message: %s\n", &buf[1]);
    return strlen(buf);
}