#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 50000  // The port on which the server will listen
#define MAX_BUFFER_SIZE 65536  // Maximum buffer size for incoming messages
#define MAX_CLIENTS 16

struct packet_header{
    char packetType;
};

typedef struct {
    char * ipAddr;
    uint16_t portNum;
} consumer;

struct producer {
    char * ipAddr;
    int portNum;
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


int create_listening_socket() {
    int serverSocket = create_local_socket();

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    int rc = bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (rc < 0) {
        printf("Error binding server socket, return code %i\n", rc);
        exit(1);
    }

    return serverSocket;
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


int main() {
    printf("Broker starting!\n");

    int serverSocket = create_listening_socket();
    consumer connected_clients[MAX_CLIENTS];
    int client_count = 0;

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    unsigned char * buffer = malloc(MAX_BUFFER_SIZE);
    memset(buffer, 0, MAX_BUFFER_SIZE);

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        int recv_len = recvfrom(serverSocket, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (recv_len < 0) {
            perror("Error receiving data");
            exit(1);
        }

        printf("Received message from %s:%d: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);
        if (buffer[0] == 0b01000000){
            for (int i = 0; i < client_count; ++i) {
                printf("fowarding text message to client: %s : %d\n",
                       connected_clients[i].ipAddr, connected_clients[i].portNum);
                send_UDP_datagram(serverSocket, buffer, recv_len,
                                  create_destination_socket(connected_clients[i].ipAddr, connected_clients[i].portNum));
            }
        } else if (buffer[0] == 0b10000011){
            connected_clients[client_count].ipAddr = strdup(inet_ntoa(clientAddr.sin_addr));
            connected_clients[client_count].portNum = ntohs(clientAddr.sin_port);
            printf("Added client : %s : %d\n", connected_clients[client_count].ipAddr,
                   connected_clients[client_count].portNum);
            client_count++;
        }
    }

    close(serverSocket);
    printf("Broker exiting!\n");
    return 0;
}