#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "mp4_utils.c"
#include "mp3_utils.c"
#include "producer_structs.h"
#include "stream_handler.c"
#include "bmp_utils.c"
#include "txt_utils.c"
#include "producer_protocol.c"
#include "protocol_constants.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include "jpg_utils.c"
#include "mem_tool.c"
#include <sys/stat.h>




#define MAX_BUFFER_SIZE 66000  // Maximum buffer size for incoming messages
#define MAX_DATA_SIZE 50000  // Maximum buffer size for incoming messages






char * BROKER_IP_ADDRESS = "172.22.0.2";
const int DESTINATION_PORT = 50000;

char * stream_target;
int streaming = 0;
unsigned char streamType = 0;
int connected = 0;



const int vBatchSize = 6;




unsigned char myID[3];

struct stream_list * myStreams;




int dirExists(char * directoryPath) {

    struct stat st;
    char * dir = malloc(strlen(directoryPath) +1);
    strcpy(&dir[1], directoryPath);
    dir[0] = '/';
    if (stat(directoryPath, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        } else {
            printf("%s: The path is not a directory.\n", directoryPath);
            return -1;
        }
    } else {
        printf("%s: The directory does not exist or there was an error.\n", directoryPath);
        return -1;
    }

    return 0;
}

// Function to parse a string of the format "[X:Y]" and create a struct timeval
struct timeval addTime(char *input) {
    struct timeval result;

    // Ensure the input starts with "[X:Y]"
    if (input[0] == '[') {
        int seconds, tenths;
        if (sscanf(input, "[%d:%d]", &seconds, &tenths) == 2) {
            // Convert tenths of a second to microseconds
            long microseconds = tenths * 100000;

            // Make a copy of the global time struct
            gettimeofday(&result, NULL);
            // Add the specified time interval to the copy
            result.tv_sec += seconds;
            result.tv_usec += microseconds;

            // Normalize the timeval struct
            if (result.tv_usec >= 1000000) {
                result.tv_sec += result.tv_usec / 1000000;
                result.tv_usec %= 1000000;
            }

            return result;
        }
    }

    // Return an invalid timeval if the input is not in the expected format
    result.tv_sec = -1;
    result.tv_usec = 0;
    return result;
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
        perror("sendto error");
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

struct stream * create_stream(char * streamNum, char * streamTypes, char * fileName){
    struct stream * newStream = malloc(sizeof(struct stream));
    newStream->streaming = 0;
    memcpy(newStream->streamID, myID, 3);
    hexStringToBytes(streamNum, &newStream->streamID[3]);
    snprintf(newStream->name, 9, "%02x%02x%02x%02x", newStream->streamID[0],newStream->streamID[1],newStream->streamID[2],newStream->streamID[3]);
    newStream->target = strdup(fileName);
    for (int i = 0; i < strlen(streamTypes); i++) {
        if (streamTypes[i] == 't'){newStream->type = newStream->type | TEXT_BIT;}
        if (streamTypes[i] == 'v'){newStream->type = newStream->type | VIDEO_BIT;}
        if (streamTypes[i] == 'a'){newStream->type = newStream->type | AUDIO_BIT;}
    }
    printf("Stream target set at %s\n", stream_target);
    if (newStream->type & VIDEO_BIT) {
        newStream->vPath = malloc(128);
        newStream->vPath[0] = 0;
        strcat(newStream->vPath, newStream->target);
        strcat(newStream->vPath, "/video.mp4");
        newStream->mp4file = malloc(sizeof(MP4File));
        open_input(newStream->mp4file, newStream->vPath, newStream->decodedFrameBuf);
        newStream->vWidth = newStream->mp4file->width;
        newStream->vHeight = newStream->mp4file->height;
        newStream->fps = newStream->mp4file->fps;
        newStream->codec_opened = 1;
        newStream->currentFrame = vBatchSize;
        newStream->vRate = (1.0 / (double) (newStream->fps));
        newStream->vFrame = 0;
        printf("Video width: %d\n", newStream->vWidth);
        printf("Video height: %d\n", newStream->vHeight);
        printf("Video fps: %d\n", newStream->fps);
        printf("vRate: %lf\n", newStream->vRate);
        printf("vpath: %s\n", newStream->vPath);
    }

    if (newStream->type & TEXT_BIT){
        newStream->textBuffer = malloc(2048);
        memset(newStream->textBuffer,0,2048);
        newStream->tPath = malloc(128);
        newStream->tPath[0] = 0;
        strcat(newStream->tPath, newStream->target);
        strcat(newStream->tPath, "/text.txt");
    }

    if (newStream->type & AUDIO_BIT){
        MP3File * newmp3 = malloc(sizeof(MP3File));
        newStream->mp3file = newmp3;
        newStream->aPath = malloc(128);
        newStream->aPath[0] = 0;
        strcat(newStream->aPath, newStream->target);
        strcat(newStream->aPath, "/audio.mp3");
        newStream->aFrame = 0;
    }

    return newStream;
}

void handle_packet(unsigned char * buffer){
    if (buffer[0] == ERROR){
        printf("ERROR PACKET RECEIVED, ERROR CODE %d\n", (int) buffer[1]);
    } else if (buffer[0] == CONTROL_PROD_CONNECT) {
        printf("Received Connection confirmed from broker!\n");
        memcpy(myID, &buffer[1], 3);
        connected = 1;
    }else if (buffer[0] == CONTROL_STREAM_UPDATE) {
        printf("Recieved stream deletion confirmation!\n");
        struct stream * strm = search_stream_id(&buffer[1], myStreams);
        int index = getStreamPosition(myStreams, strm);
        strm->streaming = 0;
        strm->vFrame = 0;
        strm->currentFrame = 0;
        if (strm->type & AUDIO_BIT){
            close_mp3(strm->mp3file);
        }
        if (strm->type & VIDEO_BIT){
            close_mp4(strm->mp4file);
            strm->codec_opened = 0;
        }
        if (strm->type & TEXT_BIT){
            strm->textFrame = 0;
        }
        streamType = 0;
    } else if ((buffer[0] & TYPE_MASK) == CONTROL_STREAM_UPDATE){
        printf("Received stream creation/validation from broker!\n");
        if (access(stream_target, F_OK) != -1) {
            printf("streaming content from %s.\n", stream_target);
            struct stream * strm = search_stream_id(&buffer[1], myStreams);
            if (strm == NULL) {printf("ERROR STREAM NOT FOUND!\n");}
            strm->streaming = 1;
            if (strm->type & VIDEO_BIT){
                printf("streaming video.mp4 content\n");
                strm->streaming = 1;
                strm->vFrame = 0;
                strm->currentFrame = 0;
                gettimeofday(&strm->startV, NULL);
            }
            if (strm->type & TEXT_BIT){
                printf("streaming text.txt content\n");
                strm->streaming = 1;
                gettimeofday(&strm->startT, NULL);
                strm->textFrame = 0;
                readLineFromFile(strm->tPath, strm->textFrame, strm->textBuffer, 2048);
                strm->endT = addTime(strm->textBuffer);
                strm->textBuffer = &strm->textBuffer[6];
                printf("start time: %lf\n",(double)(strm->startT.tv_sec) + (double)(strm->startT.tv_usec) / 1000000.0);
                printf("end time: %lf\n",(double)(strm->endT.tv_sec) + (double)(strm->endT.tv_usec) / 1000000.0);

            }
            if (strm->type & AUDIO_BIT){
                strm->streaming = 1;
                strm->aPath[0] = 0;
                strcat(strm->aPath, stream_target);
                strcat(strm->aPath, "/audio.mp3");
                printf("streaming %s content\n", strm->aPath);
                open_mp3(strm->mp3file, strm->aPath);
                gettimeofday(&strm->startA, NULL);
                strm->endA.tv_sec = 0;
                strm->endA.tv_usec = 0;
                strm->aFrame = 0;
            }
            if (strm->type == 0){printf("No type! Stopping stream!\n");streaming = 0;}
        } else {
            printf("%s does not exist in the current directory.\n", stream_target);
            streaming = 0;
        }
    } else if  (buffer[0] == CONTROL_PROD_DISCONNECT){
        printf("Received successful disconnect!\n");
        int sSize = myStreams->size;
        for (int i = 0; i < sSize; ++i) {
            struct stream * strm = getStream(myStreams, 0);
            if (strm->type & AUDIO_BIT){
                close_mp3(strm->mp3file);
            }
            if (strm->type & VIDEO_BIT){
                close_mp4(strm->mp4file);
                strm->codec_opened = 0;
            }
            if (strm->type & TEXT_BIT){
                free(strm->textBuffer);
            }
        }
        freeStreamList(myStreams);
        myStreams->head = NULL;
        myStreams->size = 0;
        memset(myID, 0, 3);
    }
}


void measure_print_time(struct timeval a, struct timeval b, char * message){
    double elapsedTime  = (double)(b.tv_sec - a.tv_sec) + (double)(b.tv_usec - a.tv_usec) / 1000000.0;
    printf("%s : %lf\n", message, elapsedTime);
}

int main() {
    printf("Server Container now running!\n");

    myStreams = malloc(sizeof(struct stream_list));
    myStreams->head = NULL;
    myStreams->size = 0;

    int localSocket = create_listening_socket();

    struct sockaddr_in brokerAddr = create_destination_socket(BROKER_IP_ADDRESS, DESTINATION_PORT);

    struct sockaddr_in sourceAddr;
    socklen_t sourceAddrLen = sizeof(sourceAddr);

    unsigned char * message = malloc(MAX_BUFFER_SIZE);
    unsigned char * buffer = malloc(MAX_BUFFER_SIZE);
    int length = -1;

    char userInput[1024];

    fd_set readfds; // File descriptor set for select

    //extractFrame("ace_combat_gameplay/video.mp4", 800, &pixelData, &frame_width, &frame_height);
    //createBMP("video.mp4-frame0.bmp", pixelData, frame_width, frame_height);

    struct timeval timeout;
    struct timeval sT2, eT2;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(localSocket, &readfds);
        int max_fd = (STDIN_FILENO > localSocket) ? STDIN_FILENO : localSocket;


        timeout.tv_sec = 0;
        timeout.tv_usec = 1;

        int ready_fds = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

        if (ready_fds == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (ready_fds == 0) {
            for (int i = 0; i < myStreams->size; ++i) {
                struct stream * strm = getStream(myStreams, i);
                if (strm->streaming) {
                    gettimeofday(&strm->endV, NULL);
                    double elapsedTime = (double) (strm->endV.tv_sec - strm->startV.tv_sec) +
                                         (double) (strm->endV.tv_usec - strm->startV.tv_usec) / 1000000.0;
                    if (elapsedTime > strm->vRate && (strm->type & VIDEO_BIT)) {
                        gettimeofday(&strm->startV, NULL);
                        if (strm->codec_opened == 0) {
                            open_input(strm->mp4file, strm->vPath, strm->decodedFrameBuf);
                            strm->vWidth = strm->mp4file->width;
                            strm->vHeight = strm->mp4file->height;
                            strm->fps = strm->mp4file->fps;
                            strm->codec_opened = 1;
                            strm->currentFrame = vBatchSize;
                        }
                        if (strm->currentFrame == vBatchSize) {
                            strm->currentFrame = 0;
                            int rc = decode_frames(strm->mp4file, vBatchSize, strm->decodedFrameBuf);
                            if (rc == -1) {
                                printf("Reached end of video file, resetting!");
                                decode_frames(strm->mp4file, vBatchSize, strm->decodedFrameBuf);
                            }
                        }
                        int Jsize = 0;
                        //char fName[50];
                        //snprintf(fName, sizeof(fName), "frame_%d.bmp", vFrame);
                        //createBMP(fName, decodedFrameBuf[currentFrame], vWidth, vHeight);
                        convert_to_jpeg(strm->decodedFrameBuf[strm->currentFrame], strm->vWidth, strm->vHeight, &message[9], &Jsize,
                                        MAX_DATA_SIZE / 2);
                        strm->vFrame++;
                        short vidWidth = strm->vWidth;
                        short vidHeight = strm->vHeight;
                        int packetSize = (MAX_DATA_SIZE / 2) + 9;
                        int vDataSize = packetSize - 9;
                        message[0] = DATA_VIDEO_FRAME;
                        unsigned int totalSize = Jsize;
                        int parts = (totalSize / vDataSize) + 1;
                        memcpy(&message[1], strm->streamID, 4);

                        memcpy(&message[5], &strm->vFrame, 4);
                        int length = 0;
                        if (totalSize < vDataSize) {
                            memcpy(&message[9], &message[9 + (vDataSize * i)], totalSize);
                            length = 9 + totalSize;
                        } else {
                            memcpy(&message[9], &message[9 + (vDataSize * i)], vDataSize);
                            length = 9 + vDataSize;
                        }
                        send_UDP_datagram(localSocket, message, length, brokerAddr);

                        strm->currentFrame++;
                        memset(message, 0, Jsize);

                    }
                    struct timeval currentTime;
                    gettimeofday(&currentTime, NULL);
                    elapsedTime = (double) (strm->endT.tv_sec - currentTime.tv_sec) +
                                  (double) (strm->endT.tv_usec - currentTime.tv_usec) / 1000000.0;
                    if (elapsedTime < 0 && (strm->type & TEXT_BIT)) {
                        strm->textFrame += 1;
                        readLineFromFile(strm->tPath, strm->textFrame, strm->textBuffer, 2048);
                        strm->endT = addTime(strm->textBuffer);
                        strm->textBuffer = &strm->textBuffer[6]; //Remmove the [X:Y]
                        message[0] = DATA_TEXT_FRAME;
                        memcpy(&message[1], strm->streamID, 4);
                        memcpy(&message[5], strm->textBuffer, strlen(strm->textBuffer) + 1);
                        int length = 5 + strlen(strm->textBuffer) + 1;
                        send_UDP_datagram(localSocket, message, length, brokerAddr);
                        memset(message, 0, MAX_BUFFER_SIZE);
                    }
                    elapsedTime = (double) (currentTime.tv_sec - strm->startA.tv_sec) +
                                  (double) (currentTime.tv_usec - strm->startA.tv_usec) / 1000000.0;
                    //printf("%lf > %lf\n", elapsedTime, ((double)endA.tv_sec + ( (double)endA.tv_usec /1000000.0)));
                    if (elapsedTime > ((double) strm->endA.tv_sec + ((double) strm->endA.tv_usec / 1000000.0)) &&
                        (strm->type & AUDIO_BIT)) {
                        gettimeofday(&strm->startA, NULL);
                        message[0] = DATA_AUDIO_FRAME;
                        memcpy(&message[1], strm->streamID, 4);
                        memcpy(&message[5], (&strm->aFrame), 4);
                        int length = 0;
                        if (mp3_read_chunk(strm->mp3file, &message[9], &strm->endA, &length) == -1) {
                            reset_mp3_reader(strm->mp3file, strm->aPath);
                            mp3_read_chunk(strm->mp3file, &message[9], &strm->endA, &length);;
                        }
                        length += 9;
                        send_UDP_datagram(localSocket, message, length, brokerAddr);
                        memset(message, 0, MAX_BUFFER_SIZE);
                        strm->aFrame++;
                        //printf("startA: %lf\n", (double)(startA.tv_sec + ((double)startA.tv_usec/1000000.0)));
                        //printf("endA: %lf\n", (double)(endA.tv_sec + ((double)endA.tv_usec/1000000.0)));
                    }
                }
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(userInput, sizeof(userInput), stdin);
            trimNewline(userInput);
            int input_count;
            char **input_array = splitString(userInput, &input_count);
            if (input_count == 0){
                printf("Zero args inputted\n");
            } else {
                if (strcmp(input_array[0], "connect") == 0 && input_count > 1) {
                    if (connected) {
                        printf("Error, still connect with ID: %02x%02x%02x!\n", myID[0], myID[1], myID[2]);
                    } else {
                        printf("Sending connect request to broker, ID: %s\n", input_array[1]);
                        length = send_prod_request_connect(message, input_array[1]);
                    }
                } else if (strcmp(input_array[0], "stream") == 0) {
                    if (input_count < 3) { printf("Error no types or stream num specified!\n"); }
                    else {
                        printf("requesting start stream %d, of type %s\n", atoi(input_array[1]), input_array[2]);
                        struct stream * newStream = create_stream(input_array[1], input_array[2], stream_target);
                        printf("Stream entry created!\n");
                        appendStream(myStreams, newStream);
                        length = send_request_stream_creation(message, newStream);
                        if (newStream->type & VIDEO_BIT) {
                            length = add_video_details(message, length, newStream->vWidth, newStream->vHeight);
                        }
                    }
                } else if (strcmp(input_array[0], "target") == 0) {
                    if (input_count < 2) { printf("Error no target specified!\n"); }
                    else if (dirExists(input_array[1]) == -1) { printf("target does not exist!\n"); }
                    else {

                        stream_target = input_array[1];
                        length = 0;
                        printf("Target set as %s\n", stream_target);
                    }
                } else if(strcmp(input_array[0], "delete") == 0) {
                    length = send_request_stream_deletion(message);
                    memcpy(&message[1], myID, 3);
                } else if (strcmp(input_array[0], "disconnect") == 0){
                    length = send_request_prod_disconnect(message);
                    memcpy(&message[1], myID, 3);
                } else if (strcmp(input_array[0], "error") == 0) {
                    length = 1;
                    message[0] = ERROR;
                } else {
                    printf("Invalid Command!\n");
                }
                if (length > 0) {
                    send_UDP_datagram(localSocket, message, length, brokerAddr);
                    memset(message, 0, MAX_BUFFER_SIZE);
                    length = -1;
                } else if (length == 0){
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