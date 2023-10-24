//
// Created by rowan on 23/10/2023.
//

#ifndef STREAM_NETWORK_CONSUMER_STRUCTS_H
#define STREAM_NETWORK_CONSUMER_STRUCTS_H
struct stream{
    unsigned char streamID[4];
    char name[9];
    unsigned char type;
    unsigned char * frameBuffer;
    unsigned char * jpegBuffer;
    int aFrame;
    int vFrame;
    int tFrame;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Event event;
    int display_Window;
    unsigned short vWidth;
    unsigned short vHeight;
};

struct stream_node {
    struct stream* data;
    struct stream_node* next;
};

struct stream_list {
    struct stream_node* head;
    int size;
};
#endif //STREAM_NETWORK_CONSUMER_STRUCTS_H
