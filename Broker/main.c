#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 50000  // The port on which the server will listen
#define MAX_BUFFER_SIZE 4096  // Maximum buffer size for incoming messages


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

    int rc = bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr))
    if (rc < 0) {
        printf("Error binding server socket, return code %i\n", rc);
        exit(1);
    }

    return serverSocket;
}


int main() {
    printf("Broker starting!\n");
    int serverSocket = create_listening_socket();


    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    unsigned char buffer[MAX_BUFFER_SIZE];

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Receive incoming messages
        int recv_len = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (recv_len < 0) {
            perror("Error receiving data");
            exit(1);
        }

        // Print received message
        //buffer[recv_len] = '\0'; // Null-terminate the received data
        printf("Received message from %s:%d: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);
    }

    close(serverSocket);
    printf("Broker exiting!\n");
    return 0;
}