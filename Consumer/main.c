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


#define MAX_BUFFER_SIZE 65536  // Maximum buffer size for incoming messages

char * BROKER_IP_ADDRESS = "172.22.0.3";
const int DESTINATION_PORT = 50000;


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


void handle_packet(unsigned char * packet){
    printf("Control byte from %X:\n", packet[0]);
    if (packet[0] == CONTROL_TEXT_FRAME){
        printf("Received text: %s\n", &packet[1]);
        writeStringToFile("output_text.txt", &packet[1]);
    }   else if (packet[0] == CONTROL_VIDEO_FRAME) {
        printf("Received image, first 8 bytes: %x%x%x%x%x%x%x%x\n", packet[1],packet[2],packet[3],packet[4],packet[5],packet[6],packet[7],packet[8]);
        createBMP("frame0.bmp", &packet[1], 64, 64);
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

int main() {
    printf("Consumer Container now running!\n");


    int localSocket = create_listening_socket();

    struct sockaddr_in brokerAddr = create_destination_socket(BROKER_IP_ADDRESS, DESTINATION_PORT);
    unsigned char * message = malloc(MAX_BUFFER_SIZE);
    memset(message, 0, MAX_BUFFER_SIZE);
    int length = -1;

    char userInput[1024];

    while (1) {
        printf("command>");
        fgets(userInput, sizeof(userInput), stdin);

        trimNewline(userInput);
        int input_count;
        char **input_array = splitString(userInput, &input_count);

        if (input_count == 0) {
            printf("Zero args inputted\n");
        } else {
            printf("command: %s\n", input_array[0]);
            if (strcmp(input_array[0], "connect") == 0) {
                printf("Sending connect request to broker.\n");
                length = send_cons_request_connect(message);
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

    // Close the socket
    close(localSocket);

    printf("Consumer: exiting!\n");
    return 0;
}