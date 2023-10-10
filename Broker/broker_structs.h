//
// Created by rowan on 09/10/2023.
//

#ifndef STREAM_NETWORK_NODE_STRUCTS_H
#define STREAM_NETWORK_NODE_STRUCTS_H

typedef struct {
    char * ipAddr;
    short portNum;
} address;

struct packet_header{
    char packetType;
};

struct stream{
    unsigned char stremID[4];
    unsigned char type;
    producer * creator;
    consumer_list;
};




#endif //STREAM_NETWORK_NODE_STRUCTS_H
