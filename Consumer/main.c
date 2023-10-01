#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "bmp_utils.c"

#define MAX_BUFFER_SIZE 65536  // Maximum buffer size for incoming messages
#define BROKER_PORT 50000



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
    serverAddr.sin_port = htons(BROKER_PORT);

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



int main(int argc, char *argv[]) {
    printf("Consumer Container now running!\n");
    char * BROKER_IP_ADDRESS = "172.22.0.3"; //Broker IP
    const int DESTINATION_PORT = 50000;

    int localSocket = create_listening_socket();

    struct sockaddr_in brokerAddr = create_destination_socket(BROKER_IP_ADDRESS, DESTINATION_PORT);
    unsigned char * message = malloc(MAX_BUFFER_SIZE);
    memset(message, 0, MAX_BUFFER_SIZE);
    int length = MAX_BUFFER_SIZE;
    printf("argv[1]: %s\n", argv[1]);
    if (strcmp(argv[1], "connect") == 0) {
        message[0] = 0b10000011;
        length = strlen(message);
    }

    send_UDP_datagram(localSocket, message, length,brokerAddr);

    struct sockaddr_in sourceAddr;
    socklen_t sourceAddrLen = sizeof(sourceAddr);
    unsigned char * buffer = malloc(MAX_BUFFER_SIZE);
    memset(buffer, 0, MAX_BUFFER_SIZE);

    while (1){
        printf("Waiting to recieved\n");
        int recv_len = recvfrom(localSocket, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&sourceAddr, &sourceAddrLen);
        printf("Recieved somthing\n");

        if (recv_len < 0) {
            perror("Error receiving data");
            exit(1);
        }
        printf("Received packet from %s:%d\n", inet_ntoa(sourceAddr.sin_addr), ntohs(sourceAddr.sin_port));
        printf("Control byte from %X:\n", buffer[0]);
        if (buffer[0] == 0b01000000){
            printf("Received text: %s\n", &buffer[1]);

        }   else if (buffer[0] == 0b00010000) {
            printf("Received image, first 8 bytes: %x%x%x%x%x%x%x%x\n", buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],buffer[8]);
            createBMP("frame0.bmp", &buffer[1], 64, 64);
        }
    }



    // Close the socket
    close(localSocket);

    printf("Consumer: exiting!\n");
    return 0;
}