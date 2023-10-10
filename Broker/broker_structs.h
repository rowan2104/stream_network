//
// Created by rowan on 10/10/2023.
//

#ifndef STREAM_NETWORK_BROKER_STRUCTS_H
#define STREAM_NETWORK_BROKER_STRUCTS_H

typedef struct {
    char * ipAddr;
    short portNum;
} address;

struct packet_header{
    char packetType;
};

struct stream{
    unsigned char streamID[4];
    char name[9];
    unsigned char type;
    struct producer * creator;
};

struct consumer{
    address caddr;
    char * name;
};

struct consumer_node {
    struct consumer* cons;
    struct consumer_node* next;
};

struct consumer_list {
    struct consumer_node* head;
    int size;
};


struct producer{
    address paddr;
    unsigned char id[3];
    char * name;
    struct stream * myStream;
};

struct producer_node {
    struct producer* prod;
    struct producer_node* next;
};

struct producer_list {
    struct producer_node* head;
    int size;
};


#endif //STREAM_NETWORK_BROKER_STRUCTS_H
