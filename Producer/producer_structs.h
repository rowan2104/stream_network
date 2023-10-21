//
// Created by rowan on 21/10/2023.
//

#ifndef STREAM_NETWORK_PRODUCER_STRUCTS_H
#define STREAM_NETWORK_PRODUCER_STRUCTS_H

struct stream{
    int streaming;
    unsigned char streamID[4];
    char name[9];
    unsigned char type;
    char * target;
    char * vPath;
    short vWidth;
    short vHeight;
    int fps;
    unsigned char * decodedFrameBuf[6];
    int currentFrame;
    int codec_opened;
    unsigned char * textBuffer;
    int textFrame;
    char * tPath;
    char * aPath;
    struct timeval  startA, endA;
    struct timeval  startV, endV;
    struct timeval  startT, endT;
    double aRate;
    double vRate;
    double tRate;
    int aFrame;
    int vFrame;
    int tFrame;
};

struct stream_node {
    struct stream* data;
    struct stream_node* next;
};

struct stream_list {
    struct stream_node* head;
    int size;
};


#endif //STREAM_NETWORK_PRODUCER_STRUCTS_H
