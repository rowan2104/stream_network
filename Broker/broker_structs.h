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





#endif //STREAM_NETWORK_NODE_STRUCTS_H
