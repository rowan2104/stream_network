#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "bmp_utils.c"
#include "txt_utils.c"
#include "protocol_constants.h"
#include "consumer_protocol.c"
#include <SDL2/SDL.h>
#include "jpg_utils.c"
#include "mp3_utils.c"

#define MAX_BUFFER_SIZE 8294400  // Maximum buffer size for incoming messages
#define MAX_FRAME_SIZE 65000
unsigned char * frameBuffer;
unsigned char * jpegBuffer;
int frameHead;

int frame_to_update;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Event event;

int display_Window;
int imageSize;
int frameName = 0;

int audio_num = 0;

char * BROKER_IP_ADDRESS = "172.22.0.2";
const int DESTINATION_PORT = 50000;

char connected;

unsigned char * subscribed;

unsigned short vWidth;
unsigned short vHeight;

struct stream_string{
    unsigned char id[4];
    unsigned char type;
} __attribute__((packed));

struct list_packet{
    unsigned char header;
    uint32_t size;
    struct stream_string listOfStream[];

} __attribute__((packed));




int create_local_socket(){
    int clientSocket;

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        printf("Error creating local Socket! Return code %i", clientSocket);
        exit(1);
    }
    return clientSocket;
}

struct sockaddr_in create_destination_socket(char * destIP, int destPort){
    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));

    destAddr.sin_family = AF_INET; //set family to ipv4
    destAddr.sin_port = htons(destPort);  //convert port to binary
    int rc = inet_pton(AF_INET, destIP, &(destAddr.sin_addr));
    if (rc <= 0) {
        printf("Error converting IP to binary format, return code: %d", rc);
        exit(1);
    }
    return destAddr;
}

int create_listening_socket() {
    int serverSocket = create_local_socket();

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(DESTINATION_PORT);

    int rc = bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (rc < 0) {
        printf("Error binding server socket, return code %i\n", rc);
        exit(1);
    }

    return serverSocket;
}

void send_UDP_datagram(int clientSocket, unsigned char * buffer, int buf_size, struct sockaddr_in destAddr){
    socklen_t destAddrLen = sizeof(destAddr);
    int rc = sendto(clientSocket, buffer, buf_size, 0, (struct sockaddr *)&destAddr, destAddrLen);
    if (rc < 0) {
        printf("Error sending data to client, return code: %i", rc);
        exit(1);
    }
}

int create_SDL_window(short width, short height){
    const int PIXEL_FORMAT = SDL_PIXELFORMAT_RGB24;
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    // Create a window
    window = SDL_CreateWindow("Real-time Video Streaming", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, 0);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return 2;
    }

    // Create a renderer for the window
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        return 3;
    }

    // Create a texture to write pixel data to
    texture = SDL_CreateTexture(renderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING, width, height);

    if (!texture) {
        SDL_Log("Texture creation failed: %s", SDL_GetError());
        return 4;
    }
}

void update_window(){
    if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
        // Cleanup and exit
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        display_Window = 0;
        return;
    }
    SDL_UpdateTexture(texture, NULL, frameBuffer, vWidth * 3);

    // Clear the renderer and copy the texture to the window
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


void handle_packet(unsigned char * buffer, unsigned int packetLength){
    if (buffer[0] == ERROR){
        printf("ERROR PACKET RECEIVED, ERROR CODE %d\n", (int) buffer[1]);
    } else if (buffer[0] == CONTROL_CONS_CONNECT) {
        printf("Received Connection confirmed from broker!\n");
        connected = 1;
    } else if ((buffer[0] & TYPE_MASK) == CONTROL_LIST_STREAM) {
        printf("Received Connection confirmed from broker!\n");
        struct list_packet* packet = (struct list_packet*)buffer;
        printf("%d streams found!\n", packet->size);
        unsigned char tempString[5];
        tempString[4] = 0;
        for (int i = 0; i < packet->size; ++i) {
            memcpy(&tempString[0], packet->listOfStream[i].id, 4);
            printf("%02x%02x%02x%02x: ", tempString[0],tempString[1],
                   tempString[2],tempString[3]);
            printf("%s", (packet->listOfStream[i].type & AUDIO_BIT) ? "a" : "");
            printf("%s", (packet->listOfStream[i].type & VIDEO_BIT) ? "v" : "");
            printf("%s", (packet->listOfStream[i].type & TEXT_BIT) ? "t" : "");
            if (strcmp(subscribed, tempString) == 0){
                printf(" (SUBSCRIBED)");
            }
            printf("\n");
        }
    } else if ((buffer[0] & TYPE_MASK) == CONTROL_SUBSCRIBE){
        memcpy(subscribed, &buffer[1], 4);
        printf("Received subscription confirmed to %02x%02x%02x%02x\n", subscribed[0],subscribed[1],subscribed[2],subscribed[3]);
        printf("Content types: ");
        char types = buffer[0] & (~TYPE_MASK);
        int header = 5;
        if (types & AUDIO_BIT){
            audio_num = 0;
            printf("a");
        }
        if (types & VIDEO_BIT){
            memcpy(&vWidth, &buffer[header], 2);
            memcpy(&vHeight, &buffer[header+2], 2);
            header+=4;
            printf("video: Width: %hu| Height: %hu\n", vWidth, vHeight);
            if (display_Window == 0) {
                create_SDL_window(vWidth, vHeight);
                display_Window = 1;
                frame_to_update = vHeight;
            }

            if (display_Window) {
                update_window();
            }


        }
        if (types & TEXT_BIT){
            printf("t");
        }

        printf("\n");
    } else if (buffer[0] == DATA_VIDEO_FRAME){
        //unsigned int part;

        //memcpy(&part, &buffer[4], 4);
        //printf("part of frame: %d | %02x%02x%02x%02x\n", part, buffer[8], buffer[9], buffer[10],buffer[11]);
        //printf("packetLength: %d\n", packetLength);

        //printf("Writing to %d\n", 0);
        //memcpy(&jpegBuffer[0], &buffer[8], packetLength-8);
        if (display_Window == 1) {
            decode_jpeg(&buffer[8], frameBuffer, packetLength - 8);
            update_window();
        }
        //imageSize += (packetLength-8);
        //printf("Copied memory succefully\n");
        //update_window();
        //if (part == 0) {
            //printf("part 0, doing stuff!\n");
            //snprintf(ffmpegCommand, sizeof(ffmpegCommand), "ffmpeg -y -i %s -vf \"select=gte(n\\,%d)\" -vframes 1 %s> /dev/null 2>&1", vPath, vFrame, "frame.bmp");
            //int result = system(ffmpegCommand

            //printf("\n");
            //char imageName[1024];
            //snprintf(imageName, sizeof(imageName), "frame%d.bmp", frameName);
            //createBMP(imageName, frameBuffer, vWidth, vHeight);
            /*for (int i = 0; i < vWidth*vHeight; ++i) {
                char temp = frameBuffer[i*3];
                frameBuffer[i*3] = frameBuffer[(i*3)+2];
                frameBuffer[(i*3)+2] = temp;
            }*/
            //printf("Saved as file!\n");

            //frameName+= 1;
            //frameHead = 0;
            //imageSize = 0;
        //}
    } else if (buffer[0] == DATA_TEXT_FRAME){
        printf("%s\n", &buffer[4]);
    } else if (buffer[0] == DATA_AUDIO_FRAME){
        char outputFileName[100];
        snprintf(outputFileName, sizeof(outputFileName), "audio_%d.mp3", audio_num);
        save_mp3(outputFileName, &buffer[6], packetLength - 6);
        audio_num+=1;
    }
}


void trimNewline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

char** splitString(char *input, int *count) {
    char **tokens = NULL;
    char *token = strtok(input, " ");
    *count = 0;

    while (token != NULL) {
        tokens = realloc(tokens, (*count + 1) * sizeof(char *));
        tokens[*count] = strdup(token);
        (*count)++;
        token = strtok(NULL, " ");
    }
    return tokens;
}

char *  check_for(char * input, char searchfor, char * retString){
    for (int i = 0; i < strlen(input); ++i) {
        if (input[i] == searchfor)
            return retString;
    }
    return "";
}

int main() {
    printf("Consumer Container now running!\n");
    connected = -1;
    subscribed = malloc(sizeof(char)*5);
    subscribed[4] = 0 ;

    frameBuffer = malloc(MAX_BUFFER_SIZE);
    jpegBuffer = malloc(MAX_BUFFER_SIZE);
    frameHead = 0;


    int localSocket = create_listening_socket();

    struct sockaddr_in sourceAddr;
    socklen_t sourceAddrLen = sizeof(sourceAddr);


    struct sockaddr_in brokerAddr = create_destination_socket(BROKER_IP_ADDRESS, DESTINATION_PORT);
    unsigned char * message = malloc(MAX_BUFFER_SIZE);
    memset(message, 0, MAX_BUFFER_SIZE);
    unsigned char * buffer = malloc(MAX_BUFFER_SIZE);
    memset(buffer, 0, MAX_BUFFER_SIZE);
    int length = -1;

    char userInput[1024];

    fd_set readfds;
    display_Window = 0;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Add STDIN to the set
        FD_SET(localSocket, &readfds); // Add the socket to the set

        int max_fd = (STDIN_FILENO > localSocket) ? STDIN_FILENO : localSocket;

        struct timeval timeout;
        timeout.tv_sec = 0; // Set the timeout to 5 seconds
        timeout.tv_usec = 10;

        int ready_fds = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

        if (ready_fds == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (ready_fds == 0) {
            continue;
        }



        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(userInput, sizeof(userInput), stdin);
            trimNewline(userInput);
            int input_count;
            char **input_array = splitString(userInput, &input_count);

            if (input_count == 0) {
                printf("Zero args inputted\n");
            } else {
                if (strcmp(input_array[0], "connect") == 0) {
                    printf("Sending connect request to broker.\n");
                    connected = 0;
                    length = send_cons_request_connect(message);
                } else if (strcmp(input_array[0], "list") == 0) {
                    printf("Requesting list of active streams.\n");
                    if (input_count > 1){
                        printf("restraints: only %s/%s/%s\n", check_for(input_array[1], 'a', "audio"),
                               check_for(input_array[1], 'v', "video"), check_for(input_array[1], 't', "text"));
                        length = send_req_list_stream(message, input_array[1]);
                    } else {
                        length = send_req_list_stream(message, "");
                    }

                } else if (strcmp(input_array[0], "subscribe") == 0) {
                    if (input_count < 3){printf("Error not enough arguments!\n");} else {
                        printf("Sending subscribe request for %s broker\n", input_array[1]);
                        printf("content types: only %s/%s/%s\n", check_for(input_array[2], 'a', "audio"),
                               check_for(input_array[2], 'v', "video"), check_for(input_array[2], 't', "text"));
                        length = send_req_subscribe(message, input_array[1], input_array[2]);
                    }
                } else {
                    printf("Invalid Command!\n");
                }

                if (length > -1) {
                    send_UDP_datagram(localSocket, message, length, brokerAddr);
                    memset(message, 0, MAX_BUFFER_SIZE);
                    length = -1;
                } else {
                    printf("Error, something has gone wrong!\n");
                }
            }

        }

        if (FD_ISSET(localSocket, &readfds)) {
            int recv_len = recvfrom(localSocket, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&sourceAddr, &sourceAddrLen);
            if (connected != -1) {
                if (recv_len < 0) {
                    perror("Error receiving data");
                    exit(1);
                }
                if (buffer[0] & 0b10000000) {
                    printf("Received packet from %s:%d\n", inet_ntoa(sourceAddr.sin_addr), ntohs(sourceAddr.sin_port));
                }
                handle_packet(buffer, recv_len);
                memset(buffer, 0, MAX_BUFFER_SIZE);
            }
        }
    }

    // Close the socket
    close(localSocket);

    printf("Consumer: exiting!\n");
    return 0;
}