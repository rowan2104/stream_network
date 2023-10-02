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
    buf[0] = 0b00010000;
    BMPImage *theImage = read_BMP_image(filename);
    memcpy(&buf[1], theImage->pixelData, theImage->size);
    return theImage->size + 1;
}

int prot_text_frame(unsigned char * buf, char * filename){
    message[0] = 0b01000000;
    strcpy(message + 1, argv[2]);
    printf("message: %s\n", message);
    length = strlen(message);
}