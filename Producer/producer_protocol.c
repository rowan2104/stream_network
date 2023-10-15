//
// Created by rowan on 01/10/2023.
//

#include "protocol_constants.h"

unsigned char hexCharToByte(char hexChar) {
    if (hexChar >= '0' && hexChar <= '9') {
        return hexChar - '0';
    } else if (hexChar >= 'A' && hexChar <= 'F') {
        return hexChar - 'A' + 10;
    } else if (hexChar >= 'a' && hexChar <= 'f') {
        return hexChar - 'a' + 10;
    }
    return 0;
}

void hexStringToBytes(const char* hexString, unsigned char* bytes) {
    for (int i = 0; i < 3; i++) {
        bytes[i] = (hexCharToByte(hexString[i * 2]) << 4) | hexCharToByte(hexString[i * 2 + 1]);
    }
}

int send_prod_request_connect(unsigned char * buf, char id[6]){
    buf[0] = CONTROL_PROD_REQUEST_CONNECT;
    hexStringToBytes(id, &buf[1]);
    int length = 4;
    return length;
}

int send_request_stream_creation(unsigned char * buf, char * type){
    buf[0] = CONTROL_REQUEST_STREAM_UPDATE;
    for (int i = 0; i < strlen(type); i++) {
        if (type[i] == 't'){buf[0] = buf[0] | TEXT_BIT;}
        if (type[i] == 'v'){buf[0] = buf[0] | VIDEO_BIT;}
        if (type[i] == 'a'){buf[0] = buf[0] | AUDIO_BIT;}
    }
    int length = 4;
    return length;
}

int add_video_details(unsigned char * buf, int length, int width, int height){
    short w = width;
    short h = height;
    memcpy(&buf[length], &w, 2);
    memcpy(&buf[length+2], &h, 2);
    return length+4;
}



