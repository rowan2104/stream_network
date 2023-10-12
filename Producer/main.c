#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "bmp_utils.c"
#include "txt_utils.c"
#include "producer_protocol.c"
#include "protocol_constants.h"


#define MAX_BUFFER_SIZE 65536  // Maximum buffer size for incoming messages
char * BROKER_IP_ADDRESS = "172.22.0.3";
const int DESTINATION_PORT = 50000;


unsigned char myID[3];

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
        tokens = realloc(tokens, (*count + 1) * sizeof(char*));
        tokens[*count] = strdup(token);
        (*count)++;
        token = strtok(NULL, " ");
    }
    return tokens;
}

void handle_packet(unsigned char * buffer){
    if (buffer[0] == ERROR){
        printf("ERROR PACKET RECEIVED, ERROR CODE %d\n", (int) buffer[1]);
    } else if (buffer[0] == CONTROL_PROD_CONNECT){
        printf("Received Connection confirmed from broker!\n");
    } else if ((buffer[0] & TYPE_MASK) == CONTROL_STREAM_UPDATE){
        printf("Received stream creation/validation from broker!\n");
    }
}


int main() {
    printf("Server Container now running!\n");

    int localSocket = create_listening_socket();

    struct sockaddr_in brokerAddr = create_destination_socket(BROKER_IP_ADDRESS, DESTINATION_PORT);

    struct sockaddr_in sourceAddr;
    socklen_t sourceAddrLen = sizeof(sourceAddr);

    unsigned char * message = malloc(MAX_BUFFER_SIZE);
    unsigned char * buffer = malloc(MAX_BUFFER_SIZE);
    int length = -1;

    char userInput[1024];

    fd_set readfds; // File descriptor set for select



    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(localSocket, &readfds);
        int max_fd = (STDIN_FILENO > localSocket) ? STDIN_FILENO : localSocket;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(userInput, sizeof(userInput), stdin);
            trimNewline(userInput);
            int input_count;
            char **input_array = splitString(userInput, &input_count);
            if (input_count == 0){
                printf("Zero args inputted\n");
            } else {
                if (strcmp(input_array[0], "text") == 0) {
                    printf("Sending text to broker, filename: %s\n", input_array[1]);
                    length = send_text_frame(message, input_array[1]);
                } else if (strcmp(input_array[0], "image") == 0) {
                    printf("Sending image file to broker, filename: %s\n", input_array[1]);
                    length = send_video_frame(message, input_array[1]);
                } else if (strcmp(input_array[0], "connect") == 0 && input_count > 1) {
                    printf("Sending connect request to broker, ID: %s\n", input_array[1]);
                    length = send_prod_request_connect(message, input_array[1]);
                    memcpy(myID, &message[1], 3);
                } else if (strcmp(input_array[0], "stream") == 0) {
                    printf("requesting start stream of type %s\n", input_array[1]);
                    memcpy(&message[1], myID, 3);
                    length = send_request_stream_creation(message, input_array[1]);
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
            if (recv_len < 0) {
                perror("Error receiving data");
                exit(1);
            }
            printf("Received packet from %s:%d\n", inet_ntoa(sourceAddr.sin_addr), ntohs(sourceAddr.sin_port));
            handle_packet(buffer);
            memset(buffer, 0, MAX_BUFFER_SIZE);
        }






    }
    // Close the socket
    close(localSocket);

    printf("Server: exiting!\n");
    return 0;
}