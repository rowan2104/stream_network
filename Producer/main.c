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

#include "bmp_utils.c"
#include "txt_utils.c"
#include "producer_protocol.c"
#include "protocol_constants.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include "mp4_utils.c"
#include "jpg_utils.c"





#define MAX_BUFFER_SIZE 8294400  // Maximum buffer size for incoming messages
char * BROKER_IP_ADDRESS = "172.22.0.2";
const int DESTINATION_PORT = 50000;

char * stream_target;
int streaming = 0;
unsigned char streamType = 0;


clock_t  startA, endA;
struct timeval  startV, endV;
struct timeval  startT, endT;


double aRate;
double vRate;
double tRate;

int aFrame;
int vFrame;
int tFrame;

int vWidth;
int vHeight;
char * vPath;
int fps;

const int vBatchSize = 6;
unsigned char * decodedFrameBuf[6];
int currentFrame = 0;
int codec_opened = 0;


unsigned char myID[3];

unsigned char * textBuffer;
int textFrame;
char * tPath;

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
            result = startT;
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

void handle_packet(unsigned char * buffer){
    if (buffer[0] == ERROR){
        printf("ERROR PACKET RECEIVED, ERROR CODE %d\n", (int) buffer[1]);
    } else if (buffer[0] == CONTROL_PROD_CONNECT){
        printf("Received Connection confirmed from broker!\n");
    } else if ((buffer[0] & TYPE_MASK) == CONTROL_STREAM_UPDATE){
        printf("Received stream creation/validation from broker!\n");
        if (access(stream_target, F_OK) != -1) {
            printf("streaming content from %s.\n", stream_target);
            if (streamType & VIDEO_BIT){
                printf("streaming video.mp4 content\n");
                streaming = 1;
                gettimeofday(&startV, NULL);
            }
            if (streamType & TEXT_BIT){
                printf("streaming text.txt content\n");
                streaming = 1;
                gettimeofday(&startT, NULL);
                readLineFromFile(tPath, textFrame, textBuffer, 2048);
                endT = addTime(textBuffer);
                textBuffer = &textBuffer[5];
                printf("start time: %lf\n",(double)(startT.tv_sec) + (double)(startT.tv_usec) / 1000000.0);
                printf("end time: %lf\n",(double)(endT.tv_sec) + (double)(endT.tv_usec) / 1000000.0);

            }
            if (streamType == 0){printf("No type! Stopping stream!");streaming = 0;}
        } else {
            printf("%s does not exist in the current directory.\n", stream_target);
            streaming = 0;
        }
    }
}


void measure_print_time(struct timeval a, struct timeval b, char * message){
    double elapsedTime  = (double)(b.tv_sec - a.tv_sec) + (double)(b.tv_usec - a.tv_usec) / 1000000.0;
    printf("%s : %lf\n", message, elapsedTime);
}

int main() {
    printf("Server Container now running!\n");

    aRate = 0;
    vRate = 0;
    tRate = 0;

    aFrame = 0;
    vFrame = 0;
    tFrame = 0;

    vWidth = 0;
    vHeight = 0;
    vPath = malloc(1024);
    tPath = malloc(1024);
    fps = 0;

    decodedFrameBuf[vBatchSize];
    textBuffer = malloc(2048);
    memset(textBuffer,0,2048);
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
            if (streaming){
                gettimeofday(&endV, NULL);
                double elapsedTime  = (double)(endV.tv_sec - startV.tv_sec) + (double)(endV.tv_usec - startV.tv_usec) / 1000000.0;
                if (elapsedTime > vRate && (streamType & VIDEO_BIT)){
                    gettimeofday(&startV, NULL);
                    gettimeofday(&eT2, NULL);
                    //measure_print_time(sT2, eT2, "between");
                    struct timeval sT, eT;
                    gettimeofday(&sT, NULL);
                    //printf("Sending frame %d\n", vFrame);
                    if (codec_opened == 0){
                        open_input(vPath, decodedFrameBuf, vWidth, vHeight);
                        codec_opened = 1;
                        currentFrame = vBatchSize;
                    }
                    gettimeofday(&eT, NULL);
                    //measure_print_time(sT, eT, "open codec");


                    if (currentFrame == vBatchSize){
                        gettimeofday(&sT, NULL);
                        currentFrame = 0;
                        decode_frames(vBatchSize, decodedFrameBuf);
                        gettimeofday(&eT, NULL);
                        //measure_print_time(sT, eT, "read 6 frames");
                    }

                    //uint8_t * buffery;
                    //extractFrame(vPath, vFrame, &buffery, &fps, &vWidth, &vHeight);
                    //printf("HERE2\n");
                    //char ffmpegCommand[1024];
                    //snprintf(ffmpegCommand, sizeof(ffmpegCommand), "ffmpeg -y -i %s -vf \"select=gte(n\\,%d)\" -vframes 1 %s> /dev/null 2>&1", vPath, vFrame, "frame.bmp");
                    //int result = system(ffmpegCommand);
                    //BMPImage * theImage = read_BMP_image("frame.bmp");
                    //memcpy(&message[8], theImage->pixelData, vWidth*vHeight*3);
                    //char imageName[1024];
                    //snprintf(imageName, sizeof(imageName), "frame%d.bmp", vFrame);
                    //createBMP(imageName, decodedFrameBuf[currentFrame], vWidth, vHeight);
                    //gettimeofday(&eT, NULL);
                    //measure_print_time(sT, eT, "extract");

                    int Jsize = 0;
                    gettimeofday(&sT, NULL);
                    uint8_t * buffery;
                    convert_to_jpeg(decodedFrameBuf[currentFrame], vWidth, vHeight, &message[8], &Jsize);
                    //free(theImage->pixelData);
                    //free(theImage);
                    gettimeofday(&eT, NULL);
                    //measure_print_time(sT, eT, "JPEG");
                    //printf("Jsize: %d\n", Jsize);
                    //memcpy(&message[8],theImage->pixelData,Jsize);
                    //free(buffery);
                    vFrame++;
                    //printf("message[8-11] | ");
                    //for (int i = 0; i < 4; ++i) {
                    //    printf("%02x",message[8+i]);
                    //}
                    //printf("\n");
                    short vidWidth = vWidth;
                    short vidHeight = vHeight;
                    int packetSize = (65000)+8;
                    int vDataSize = packetSize-8;
                    message[0] = DATA_VIDEO_FRAME;
                    unsigned int totalSize = Jsize;
                    //printf("Total size: %d\n", totalSize);
                    int parts = (totalSize/vDataSize)+1;
                    memcpy(&message[1], myID, 3);
                    unsigned int part = 0;
                    gettimeofday(&sT, NULL);
                    for (int i = 0; i < parts; ++i) {
                        //printf("i! %d\n", i);
                        part = (parts-1)-i;
                        memcpy(&message[4], &part, 4);
                        int length = 0;
                        //printf("i: %d\n", i);
                        //printf("totalSize<vDataSize: %d<%d\n",totalSize, vDataSize);
                        if (totalSize<vDataSize) {
                            //printf("using totalSize: %d\n",totalSize);
                            //printf("Copying to 8 + (vDataSize * i): %d\n",8 + (vDataSize * i));
                            memcpy(&message[8], &message[8 + (vDataSize * i)], totalSize);
                            length = 8 + totalSize;
                        } else {
                            //printf("using vDataSize: %d\n",vDataSize);
                            //printf("Copying to 8 + (vDataSize * i): %d\n",(8 + (vDataSize * i)));
                            memcpy(&message[8], &message[8 + (vDataSize * i)], vDataSize);
                            length = 8 + vDataSize;
                        }
                        totalSize -= (length-8);
                        //printf("Total size!: %d\n", totalSize);
                        send_UDP_datagram(localSocket, message, length, brokerAddr);
                    }
                    currentFrame++;
                    gettimeofday(&eT, NULL);
                    //measure_print_time(sT, eT, "packets");
                    memset(message, 0, Jsize);
                    gettimeofday(&sT2, NULL);
                }
                struct timeval currentTime;
                gettimeofday(&currentTime, NULL);
                elapsedTime  = (double)(endT.tv_sec - currentTime.tv_sec) + (double)(endT.tv_usec - currentTime.tv_usec) / 1000000.0;
                if (elapsedTime < 0 && (streamType & TEXT_BIT)) {
                    textFrame+=1;
                    if (textFrame == 20) {textFrame = 0;}
                    readLineFromFile(tPath, textFrame, textBuffer, 2048);
                    endT = addTime(textBuffer);
                    message[0] = DATA_TEXT_FRAME;
                    memcpy(&message[1], myID, 3);
                    memcpy(&message[4], textBuffer, strlen(textBuffer)+1);
                    int length = 4 + strlen(textBuffer)+1;
                    send_UDP_datagram(localSocket, message, length, brokerAddr);
                    memset(message, 0, MAX_BUFFER_SIZE);
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
                    printf("Sending connect request to broker, ID: %s\n", input_array[1]);
                    length = send_prod_request_connect(message, input_array[1]);
                    memcpy(myID, &message[1], 3);
                } else if (strcmp(input_array[0], "stream") == 0) {
                    if (input_count < 2){printf("Error no types specified!\n");} else {
                        printf("requesting start stream of type %s\n", input_array[1]);
                        memcpy(&message[1], myID, 3);
                        length = send_request_stream_creation(message, input_array[1]);
                        streamType = message[0] & (~TYPE_MASK);
                        if (streamType & VIDEO_BIT){
                            getDetails(vPath, &fps, &vWidth, &vHeight);
                            vRate = 1.0/(double)fps;
                            vFrame = 0;
                            printf("Video width: %d\n", vWidth);
                            printf("Video height: %d\n", vHeight);
                            printf("Video fps: %d\n", fps);
                            printf("vRate: %lf\n",vRate );

                            printf("vpath: %s\n",vPath);
                        }
                        printf("streamType: %02x\n", streamType);
                    }
                    if ((message[0] & (~TYPE_MASK)) & VIDEO_BIT){length = add_video_details(message, length, vWidth, vHeight);}
                } else if (strcmp(input_array[0], "target") == 0){
                    if (input_count < 2) {printf("Error no target specified!\n");} else {
                        stream_target = input_array[1];
                        printf("Stream target set at %s\n", stream_target);
                        vPath[0] = 0;
                        strcat(vPath, stream_target);
                        strcat(vPath, "/video.mp4");
                        tPath[0] = 0;
                        strcat(tPath, stream_target);
                        strcat(tPath, "/text.txt");
                        length = 0;
                    }
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