#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "bmp_utils.c"

#define MAX_BUFFER_SIZE 65536  // Maximum buffer size for incoming messages

struct packet_header{
    char packetType;
};

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

void send_UDP_datagram(int clientSocket, unsigned char * buffer, int buf_size, struct sockaddr_in destAddr){
    socklen_t destAddrLen = sizeof(destAddr);
    int rc = sendto(clientSocket, buffer, buf_size, 0, (struct sockaddr *)&destAddr, destAddrLen);
    if (rc < 0) {
        printf("Error sending data to client, return code: %i", rc);
        exit(1);
    }
}



int main(int argc, char *argv[]) {
    printf("Server Container now running!\n");
    char * BROKER_IP_ADDRESS = "172.22.0.3"; //Broker IP
    const int DESTINATION_PORT = 50000;

    int localSocket = create_local_socket();

    struct sockaddr_in brokerAddr = create_destination_socket(BROKER_IP_ADDRESS, DESTINATION_PORT);
    unsigned char * message = malloc(MAX_BUFFER_SIZE);
    int length = MAX_BUFFER_SIZE;
    printf("argv[1]: %s\n", argv[0]);
    if (strcmp(argv[1], "text") == 0){
        printf("argv[2]: %s\n", argv[2]);
        message[0] = 0b01000000;
        strcpy(message+1, argv[2]);
        printf("message: %s\n", message);
        length = strlen(message);
    } else if (strcmp(argv[1], "image") == 0){
        message = read_BMP_image(argv[2])->pixelData;
        length = read_BMP_image(argv[2])->size;
    }

    send_UDP_datagram(localSocket, message, length,brokerAddr);

    // Close the socket
    close(localSocket);

    printf("Server: exiting!\n");
    return 0;
}


/*
int main() {
    printf("Producer Container now running!\n");
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr);

    // Server IP and Port
    char *serverIP = "172.22.0.3"; // Replace with the target IP address
    int serverPort = 50000; // Replace with the target port number

    // Create a socket
    if ((clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    // Fill in the server's address information
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    // Create a buffer with data to send
    unsigned char buffer[1400];
    buffer[0] = 'W';
    buffer[1] = 'O';
    buffer[2] = 'A';
    buffer[3] = 'H';

    // Send the UDP packet
    if (sendto(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&serverAddr, addrLen) < 0) {
        perror("sendto");
        exit(1);
    }

    // Close the socket
    close(clientSocket);
    printf("Producer: exiting!\n");
    return 0;
}
*/