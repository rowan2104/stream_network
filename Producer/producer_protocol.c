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
    for (int i = 0; i < strlen(hexString); i++) {
        bytes[i] = (hexCharToByte(hexString[i * 2]) << 4) | hexCharToByte(hexString[i * 2 + 1]);
    }
}

struct stream * search_stream_id(unsigned char newID[4], struct stream_list * streamList){
    int i = 0;
    int y = (newID[0] << 0) | (newID[1] << 8) | (newID[2] << 16) |  (newID[3] << 24);
    struct stream * temp;
    while (getStream(streamList, i) != NULL){
        temp = getStream(streamList, i);
        int x = (temp->streamID[0] << 0) | (temp->streamID[1] << 8) | (temp->streamID[2] << 16) | (temp->streamID[3] << 24);
        if (x == y){
            return temp;
        }
        i++;
    }
    return NULL;
}




int send_prod_request_connect(unsigned char * buf, char id[6]){
    buf[0] = CONTROL_PROD_REQUEST_CONNECT;
    hexStringToBytes(id, &buf[1]);
    int length = 4;
    return length;
}

int send_request_stream_creation(unsigned char * buf, struct stream * newStream){
    buf[0] = CONTROL_REQUEST_STREAM_UPDATE;
    buf[0] = buf[0] | newStream->type;
    memcpy(&buf[1], newStream->streamID, 4);
    int length = 5;
    printf("packet header: %02x%02x%02x%02x%02x\n",buf[0],buf[1],buf[2],buf[3],buf[4]);
    return length;
}

int send_request_stream_deletion(unsigned char * buf){
    buf[0] = CONTROL_REQUEST_STREAM_UPDATE;
    return 4;
}

int send_request_prod_disconnect(unsigned char * buf){
    buf[0] = CONTROL_PROD_REQUEST_DISCONNECT;
    return 4;
}

int add_video_details(unsigned char * buf, int length, int width, int height){
    short w = width;
    short h = height;
    memcpy(&buf[length], &w, 2);
    memcpy(&buf[length+2], &h, 2);
    return length+4;
}



