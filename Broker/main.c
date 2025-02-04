//Created by Rowan Barr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "protocol_constants.h"
#include "broker_structs.h"
#include "producer_handler.c"
#include "consumer_handler.c"
#include "stream_handler.c"
#include "broker_protocol.c"
#include "jpg_utils.c"


#define PORT 50000  // The port on which the server will listen
#define MAX_BUFFER_SIZE 8294400  // Maximum buffer size for incoming messages
#define MAX_DATA_SIZE 50000  // Maximum buffer size for incoming messages


address dest_addr;
address source_addr;

struct  producer_list * connected_producers;
struct  consumer_list * connected_consumers;

struct sockaddr_in theDestSocket;

int serverSocket;



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
    int rc = inet_pton(AF_INET, strdup(destIP), &(destAddr.sin_addr));
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

void remove_client(struct consumer * con){
    removeConsumer(connected_consumers, getConsumerPosition(connected_consumers, con));
}


int unsub_all(struct consumer * requester){
    struct producer * current_producer;
    for (int i = 0; i < connected_producers->size; ++i) {
        current_producer = getProducer(connected_producers, i);
        for (int j = 0; j < current_producer->myStreams->size; ++j) {
            struct stream * strm = getStream(current_producer->myStreams, j);
            if (search_consumers_ip(requester->caddr.ipAddr, strm->subscribers) != NULL) {
                removeConsumer(strm->subscribers,
                               getConsumerPosition(strm->subscribers, requester));
                printf(" subscriber %s:%hu for Stream %s now has unsubscribed\n", requester->caddr.ipAddr,
                       requester->caddr.portNum,
                       strm->name);
            }
        }

    }
    return 0;
}

int stop_all(struct producer * requester){
    unsigned char prodID[3];
    usleep(40000);
    memcpy(prodID, requester->id, 3);
    char buf[12];
    buf[0] = DATA_STREAM_DELETED;
    for (int i = 0; i < requester->myStreams->size; ++i) {
        struct stream * currentStream = getStream(requester->myStreams, i);
        memcpy(&buf[1], currentStream->streamID, 4);
        for (int j = 0; j < currentStream->subscribers->size; ++j) {
            printf("unsubbing %s\n", getConsumer(currentStream->subscribers, j)->caddr.ipAddr);
            struct consumer * cConsumer = getConsumer(currentStream->subscribers, j);
            send_UDP_datagram(serverSocket, buf, 4,
                              create_destination_socket(cConsumer->caddr.ipAddr, cConsumer->caddr.portNum));
        }
    }

    memcpy(&buf[1], prodID, 3);
    if (recv_request_delete_stream(buf, connected_producers) == 1){
        printf("ERROR DELETING STREAM!\n");
    }
}

void handle_packet(unsigned char * buffer, int packetLength){

    int length = -1;

    if (buffer[0] == DATA_VIDEO_FRAME || buffer[0] == DATA_TEXT_FRAME|| buffer[0] == DATA_AUDIO_FRAME) {
        struct stream * currentStream = search_stream_id(&buffer[1], search_producers_id(&buffer[1], connected_producers)->myStreams);
        for (int i = 0; i < currentStream->subscribers->size; ++i) {
            struct consumer * cConsumer = getConsumer(currentStream->subscribers, i);
            send_UDP_datagram(serverSocket, buffer, packetLength,
                              create_destination_socket(cConsumer->caddr.ipAddr, cConsumer->caddr.portNum));
        }
        length = -1;

    } else {
        if (buffer[0] & 0b10000000) {
            printf("Control byte from %X:\n", buffer[0]);
        }
        if (buffer[0] == CONTROL_PROD_REQUEST_CONNECT) {
            int result = recv_prod_request_connect(buffer, connected_producers, source_addr);
            if (result != -1) {
                printf("sending connection confirmed to %s\n",
                       getProducer(connected_producers, result)->name);
                buffer[0] = CONTROL_PROD_CONNECT;
                memcpy(&buffer[1], getProducer(connected_producers, result)->id, 3);
                length = 4;
                dest_addr = getProducer(connected_producers, result)->paddr;
            }
        } else if (buffer[0] == CONTROL_CONS_REQUEST_CONNECT) {
            int result = recv_cons_request_connect(buffer, connected_consumers, source_addr);
            if (result != -1) {
                printf("sending connection confirmed client at %s:%hu\n", source_addr.ipAddr, source_addr.portNum);
                buffer[0] = CONTROL_CONS_CONNECT;
                length = 4;
                dest_addr = getConsumer(connected_consumers, result)->caddr;
            }
        } else if (buffer[0] == CONTROL_REQUEST_STREAM_UPDATE) {
            unsigned char prodID[3];
            buffer[0] = DATA_STREAM_DELETED;
            struct producer * cProd = search_producers_id(&buffer[1], connected_producers);
            struct stream *currentStream = search_stream_id(&buffer[1], cProd->myStreams);
            for (int i = 0; i < currentStream->subscribers->size; ++i) {
                struct consumer *cConsumer = getConsumer(currentStream->subscribers, i);
                send_UDP_datagram(serverSocket, buffer, 4,
                                  create_destination_socket(cConsumer->caddr.ipAddr, cConsumer->caddr.portNum));
            }
            if (recv_request_delete_stream(buffer, connected_producers) == 1) {
                printf("ERROR DELETING STREAM!\n");
            } else {
                buffer[0] = CONTROL_STREAM_UPDATE;
                length = 5;
            }
        } else if ((buffer[0] & TYPE_MASK) == CONTROL_REQUEST_STREAM_UPDATE) {
            printf("Received Stream creation request!\n");
            struct stream *newStream = recv_request_create_stream(buffer, connected_producers);
            if (newStream != NULL) {
                struct producer *theProd = newStream->creator;
                if (search_stream_id(newStream->streamID, theProd->myStreams) == NULL) {
                    appendStream(theProd->myStreams, newStream);
                    printf("created a new stream %s, from producer %s\n", newStream->name, theProd->name);
                    printf("stream of type: ");
                    if (newStream->type & AUDIO_BIT) { printf("a"); }
                    if (newStream->type & VIDEO_BIT) { printf("v"); }
                    if (newStream->type & TEXT_BIT) { printf("t"); }
                    printf("\n");
                    dest_addr = theProd->paddr;
                } else {
                    printf("stream already exists, updating!\n");
                    struct stream * strm = search_stream_id(newStream->streamID, theProd->myStreams);
                    printf("stream of type: ");
                    strm->type = newStream->type;
                    if (newStream->type & AUDIO_BIT) { printf("a"); }
                    if (newStream->type & VIDEO_BIT) { printf("v"); }
                    if (newStream->type & TEXT_BIT) { printf("t"); }
                    printf("\n");
                    dest_addr = theProd->paddr;
                }
                printf("Sending stream confirmation!\n");
                buffer[0] = CONTROL_STREAM_UPDATE | newStream->type;
                memcpy(&buffer[1], newStream->streamID, 4);
                length = 5;
            } else {
                printf("Error creating stream!\n");
            }

        } else if (buffer[0] == CONTROL_CONS_REQUEST_DISCONNECT) {
            printf("Receive disconnect request from, client at %s:%hu\n", source_addr.ipAddr, source_addr.portNum);
            printf("First, unsubscribing client from all streams!\n");
            unsub_all(search_consumers_ip(source_addr.ipAddr, connected_consumers));
            printf("Second, removing client from connect list!\n");
            remove_client(search_consumers_ip(source_addr.ipAddr, connected_consumers));
            printf("Sending disconnect confirmation!\n");
            buffer[0] = CONTROL_CONS_DISCONNECT;
            length = 4;
        } else if (buffer[0] == CONTROL_PROD_REQUEST_DISCONNECT) {
            printf("Receive disconnect request from, producer at %s:%hu\n", source_addr.ipAddr, source_addr.portNum);
            printf("First, stopping all producer streams!\n");
            stop_all(search_producer_ip(source_addr.ipAddr, connected_producers));
            printf("Second, removing producer from connect list!\n");
            dest_addr = source_addr;
            removeProducer(connected_producers, getProducerPosition(connected_producers,
                                                                    search_producer_ip(source_addr.ipAddr,
                                                                                       connected_producers)));

            printf("Sending disconnect confirmation!\n");
            buffer[0] = CONTROL_PROD_DISCONNECT;
            length = 4;

        } else if ((buffer[0] & TYPE_MASK) == CONTROL_REQ_LIST_STREAM) {
            printf("Receive request list of streams request\n");
            length = send_list_stream(buffer, connected_producers);
            printf("Sending back list of size: %d\n", *(int *) &buffer[1]);
            dest_addr = source_addr;
        } else if ((buffer[0] & TYPE_MASK) == CONTROL_REQ_SUBSCRIBE) {
            printf("Recieve subscribe request from, client at %s:%hu\n", source_addr.ipAddr, source_addr.portNum);
            length = recv_req_stream_subscribe(buffer, search_consumers_ip(source_addr.ipAddr, connected_consumers),
                                               connected_producers);
        } else if ((buffer[0] & TYPE_MASK) == CONTROL_REQ_UNSUBSCRIBE) {
            printf("Recieve unsubscribe request from, client at %s:%hu\n", source_addr.ipAddr, source_addr.portNum);
            length = recv_req_stream_unsubscribe(buffer, search_consumers_ip(source_addr.ipAddr, connected_consumers),
                                                 connected_producers);
        } else if (ERROR) {
            printf("RECEIVED ERROR PACKET!\n");
        }
    }


    if (length != -1) {
        send_UDP_datagram(serverSocket, buffer, length,
                          create_destination_socket(dest_addr.ipAddr, dest_addr.portNum));
        length = -1;
    }
    return;
}


int main() {
    printf("Broker now running!\n");
    theDestSocket = create_destination_socket("0.0.0.0", 0);
    serverSocket = create_listening_socket();
    connected_producers = malloc(sizeof(struct producer_list));
    connected_producers->head = NULL;
    connected_producers->size = 0;

    connected_consumers = malloc(sizeof(struct consumer_list));
    connected_consumers->head = NULL;
    connected_consumers->size = 0;

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
        if (buffer[0] == ERROR){
            break;
        }
        // Free the previous allocation, if any
        if (source_addr.ipAddr) {
            free(source_addr.ipAddr);
        }
        source_addr.ipAddr = strdup(inet_ntoa(clientAddr.sin_addr));
        source_addr.portNum = ntohs(clientAddr.sin_port);
        if (buffer[0] == CONTROL_PROD_REQUEST_CONNECT || buffer[0] == CONTROL_CONS_REQUEST_CONNECT || search_consumers_ip(source_addr.ipAddr, connected_consumers) != NULL || search_producer_ip(source_addr.ipAddr, connected_producers)){
            if (buffer[0] & 0b10000000) {
                printf("Received packet from %s:%hu\n", source_addr.ipAddr, source_addr.portNum);
            }
            handle_packet(buffer, recv_len);
        }
/*
        if (buffer[0] == 0b01000000){
            printf("Received text: %s\n", &buffer[1]);
            for (int i = 0; i < client_count; ++i) {
                printf("forwarding text message to client: %s : %d\n",
                       connected_clients[i].ipAddr, connected_clients[i].portNum);
                send_UDP_datagram(serverSocket, buffer, recv_len,
                                  create_destination_socket(connected_clients[i].ipAddr, connected_clients[i].portNum));
            }

        }   else if (buffer[0] == 0b00010000) {
            printf("Received image, first 8 bytes: %x%x%x%x%x%x%x%x\n", buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],buffer[8]);
            for (int i = 0; i < client_count; ++i) {
                printf("forwarding image to client: %s : %d\n",
                       connected_clients[i].ipAddr, connected_clients[i].portNum);
                send_UDP_datagram(serverSocket, buffer, recv_len,
                                  create_destination_socket(connected_clients[i].ipAddr, connected_clients[i].portNum));
            }
        }   else if (buffer[0] == 0b10000011){
            connected_clients[client_count].ipAddr = strdup(inet_ntoa(clientAddr.sin_addr));
            connected_clients[client_count].portNum = ntohs(clientAddr.sin_port);
            printf("Added client : %s : %d\n", connected_clients[client_count].ipAddr,
                   connected_clients[client_count].portNum);
            client_count++;
        }*/
    }

    close(serverSocket);
    printf("Broker exiting!\n");
    return 0;
}