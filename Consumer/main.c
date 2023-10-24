//Created by Rowan Barr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#include "bmp_utils.c"
#include "txt_utils.c"
#include "consumer_structs.h"
#include "stream_handler.c"
#include "protocol_constants.h"
#include "consumer_protocol.c"
#include "jpg_utils.c"
#include "mp3_utils.c"

#define MAX_BUFFER_SIZE 66000  // Maximum buffer size for incoming messages
#define  MAX_VIDEO_SIZE 8294400  // Maximum buffer size for incoming messages

struct stream_list * subscribed;

int localSocket;


char * BROKER_IP_ADDRESS = "172.22.0.2";
const int DESTINATION_PORT = 50000;

char connected;



struct stream_string{
    unsigned char id[4];
    unsigned char type;
} __attribute__((packed));

struct list_packet{
    unsigned char header;
    uint32_t size;
    struct stream_string listOfStream[];

} __attribute__((packed));



int check_in_range(unsigned int x, unsigned int y){
    if (x==y){
        return 0;
    }
    // Calculate the upper bound of the range and handle wrapping
    unsigned int upper_bound = y + 2147483648;
    // Check if x is within the range [Y, Y + 8] (including wraparound)
    if (x >= y && x <= upper_bound) {
        return 1;
    } else {
        return -1;
    }
}

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

int create_SDL_window(struct stream * strm){
    const int PIXEL_FORMAT = SDL_PIXELFORMAT_RGB24;
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    // Create a window
    strm->window = SDL_CreateWindow("Real-time Video Streaming", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, strm->vWidth, strm->vHeight, 0);
    if (!strm->window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return 2;
    }

    // Create a renderer for the window
    strm->renderer = SDL_CreateRenderer(strm->window, -1, 0);
    if (!strm->renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        return 3;
    }

    // Create a texture to write pixel data to
    strm->texture = SDL_CreateTexture(strm->renderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING, strm->vWidth, strm->vHeight);

    if (!strm->texture) {
        SDL_Log("Texture creation failed: %s", SDL_GetError());
        return 4;
    }
}

void close_window(struct stream * strm) {
    // Cleanup and exit
    SDL_DestroyTexture(strm->texture);
    SDL_DestroyRenderer(strm->renderer);
    SDL_DestroyWindow(strm->window);
    SDL_Quit();
    strm->display_Window = 0;
    return;
}
void update_window(struct stream * strm){
    if (SDL_PollEvent(&strm->event) && strm->event.type == SDL_QUIT) {
        close_window(strm);
    }
    SDL_UpdateTexture(strm->texture, NULL, strm->frameBuffer, strm->vWidth * 3);

    // Clear the renderer and copy the texture to the window
    SDL_RenderClear(strm->renderer);
    SDL_RenderCopy(strm->renderer, strm->texture, NULL, NULL);
    SDL_RenderPresent(strm->renderer);
}

struct stream * create_stream(unsigned char type, char id[4]){
    struct stream * strm = malloc(sizeof(struct stream));
    memcpy(strm->streamID, id, 4);
    snprintf(strm->name, 9, "%02x%02x%02x%02x",id[0], id[1], id[2], id[4]);
    strm->type = strm->type | (type & ~TYPE_MASK);
    if (strm->type & VIDEO_BIT) {
        strm->frameBuffer = malloc(MAX_VIDEO_SIZE);
        strm->jpegBuffer = malloc(MAX_BUFFER_SIZE);
    }
    strm->aFrame = 0;
    strm->vFrame = 0;
    strm->tFrame = 0;
    strm->display_Window = 0;
    return strm;

}

void handle_packet(unsigned char * buffer, unsigned int packetLength) {

    if (buffer[0] == DATA_VIDEO_FRAME){
        struct stream * strm = search_stream_id(&buffer[1], subscribed);
        if (strm->display_Window == 1) {
            unsigned int temp;
            memcpy(&temp, &buffer[5], 4);
            if (check_in_range(temp, strm->vFrame)) {
                decode_jpeg(&buffer[9], strm->frameBuffer, packetLength - 9);
                update_window(strm);
                strm->vFrame = temp;
            } else {
                printf("Recieved out of order video frames, recieved: %u, on %u\n", temp, strm->vFrame);
            }
        }
    } else if (buffer[0] == DATA_TEXT_FRAME){
        printf("%02x%02x%02x%02x: %s\n", buffer[1], buffer[2], buffer[3], buffer[4], &buffer[5]);
    } else if (buffer[0] == DATA_AUDIO_FRAME){
        struct stream * strm = search_stream_id(&buffer[1], subscribed);
        char outputFileName[100];
        unsigned int part;
        memcpy(&part, &buffer[5],4);
        if (check_in_range(part, strm->aFrame)) {
            strm->aFrame = part;
            printf("Recieved audio from %02x%02x%02x%02x: saving as %02x%02x%02x%02x_%d.mp3\n",
                   buffer[1], buffer[2], buffer[3], buffer[4], buffer[1], buffer[2], buffer[3], buffer[4], strm->aFrame);
            snprintf(outputFileName, sizeof(outputFileName), " %02x%02x%02x%02x_%d.mp3", buffer[1], buffer[2],
                     buffer[3], buffer[4], strm->aFrame);
            save_mp3(outputFileName, &buffer[9], packetLength - 7);
        } else {
            printf("Recieved out of order audio frames, recieved: %u, on %u\n", part, strm->aFrame);
        }
    } else if (buffer[0] == ERROR) {
        printf("ERROR PACKET RECEIVED, ERROR CODE %d\n", (int) buffer[1]);
    } else if (buffer[0] == CONTROL_CONS_CONNECT) {
        printf("Received Connection confirmed from broker!\n");
        connected = 1;
    } else if ((buffer[0] & TYPE_MASK) == CONTROL_LIST_STREAM) {
        printf("Received Connection confirmed from broker!\n");
        struct list_packet *packet = (struct list_packet *) buffer;
        printf("%d streams found!\n", packet->size);
        unsigned char tempString[5];
        tempString[4] = 0;
        for (int i = 0; i < packet->size; ++i) {
            memcpy(&tempString[0], packet->listOfStream[i].id, 4);
            printf("%02x%02x%02x%02x: ", tempString[0], tempString[1],
                   tempString[2], tempString[3]);
            printf("%s", (packet->listOfStream[i].type & AUDIO_BIT) ? "a" : "");
            printf("%s", (packet->listOfStream[i].type & VIDEO_BIT) ? "v" : "");
            printf("%s", (packet->listOfStream[i].type & TEXT_BIT) ? "t" : "");
            for (int j = 0; j < subscribed->size; ++j) {
                struct stream * strm = search_stream_id(tempString, subscribed);
                if (strm != NULL){
                    printf(" (SUBSCRIBED)");
                }
            }
            printf("\n");
        }
    } else if ((buffer[0] & TYPE_MASK) == CONTROL_SUBSCRIBE) {
        printf("Received subscription confirmed to %02x%02x%02x%02x\n", buffer[1], buffer[2], buffer[3],
               buffer[4]);
        char types = buffer[0] & (~TYPE_MASK);
        struct stream * strm = create_stream(types, &buffer[1]);
        appendStream(subscribed, strm);
        int header = 5;
        if (types & VIDEO_BIT) {
            memcpy(&strm->vWidth, &buffer[header], 2);
            memcpy(&strm->vHeight, &buffer[header + 2], 2);
            printf("video: Width: %hu| Height: %hu\n", strm->vWidth, strm->vHeight);
            if (strm->display_Window == 0) {
                create_SDL_window(strm);
                strm->display_Window = 1;
            }
            if (strm->display_Window) {
                update_window(strm);
            }
        }
    } else if ((buffer[0] & TYPE_MASK) == CONTROL_UNSUBSCRIBE) {
        printf("Received unsubscripted confirmation to %02x%02x%02x%02x\n", buffer[1], buffer[2], buffer[3], buffer[4]);
        struct stream * strm = search_stream_id(&buffer[1], subscribed);
        if (strm->display_Window == 1) {
            close_window(strm);
        }
        removeStream(subscribed, getStreamPosition(subscribed, strm));
    } else if (buffer[0] == CONTROL_CONS_DISCONNECT) {
        printf("Received disconnect confirmation!\n");
        for (int i = 0; i < subscribed->size; ++i) {
            struct stream * strm = search_stream_id(&buffer[1], subscribed);
            if (strm->display_Window == 1) {
                close_window(strm);
            }
        }

        close(localSocket);
        system("./main");
        exit(0);
    } else if (buffer[0] == DATA_STREAM_DELETED) {
        printf("Received stream deletion from %02x%02x%02x\n", buffer[1], buffer[2], buffer[3]);
        struct stream * strm = search_stream_id(&buffer[1], subscribed);
        if (strm->display_Window == 1) {
            close_window(strm);
        }
        removeStream(subscribed, getStreamPosition(subscribed, strm));
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
    printf("Consumer now running!\n");
    connected = -1;
    subscribed = malloc(sizeof(struct stream_list));
    subscribed->head = NULL;
    subscribed->size = 0;

    localSocket = create_listening_socket();

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
                    if (input_count > 1) {
                        printf("restraints: only %s/%s/%s\n", check_for(input_array[1], 'a', "audio"),
                               check_for(input_array[1], 'v', "video"), check_for(input_array[1], 't', "text"));
                        length = send_req_list_stream(message, input_array[1]);
                    } else {
                        length = send_req_list_stream(message, "");
                    }

                } else if (strcmp(input_array[0], "subscribe") == 0) {
                    if (input_count < 3) { printf("Error not enough arguments!\n"); }
                    else {
                        printf("Sending subscribe request for %s to broker\n", input_array[1]);
                        length = send_req_subscribe(message, input_array[1]);
                    }
                } else if (strcmp(input_array[0], "disconnect") == 0){
                    printf("sending disconnection request!\n");
                    length = send_cons_request_disconnect(message);
                } else if (strcmp(input_array[0], "unsubscribe") == 0) {
                    printf("Sending unsubscribe request for %s to broker\n", input_array[1]);
                    length = send_req_unsubscribe(message, input_array[1], input_array[2]);
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
                if (buffer[0] == DATA_STREAM_DELETED){
                    printf("stream has suddenly stopped!\n");
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