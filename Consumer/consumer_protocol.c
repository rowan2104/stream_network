//
// Created by rowan on 09/10/2023.
//

unsigned char hexCharToByte(char hexChar) {
    if (hexChar >= '0' && hexChar <= '9') {
        return hexChar - '0';
    } else if (hexChar >= 'A' && hexChar <= 'F') {
        return hexChar - 'A' + 10;
    } else if (hexChar >= 'a' && hexChar <= 'f') {
        return hexChar - 'a' + 10;
    }
    return 0;
}

void hexStringToBytes(const char* hexString, unsigned char* bytes) {
    for (int i = 0; i < 4; i++) {
        bytes[i] = (hexCharToByte(hexString[i * 2]) << 4) | hexCharToByte(hexString[i * 2 + 1]);
    }
}


struct stream * search_stream_id(unsigned char newID[4], struct stream_list * streamList){
    int i = 0;
    struct stream * temp;
    while (getStream(streamList, i) != NULL){
        temp = getStream(streamList, i);
        if (memcmp(newID,temp->streamID,4)==0){
            return temp;
        }
        i++;
    }
    return NULL;
}

int send_cons_request_connect(unsigned char * buf){
    buf[0] = CONTROL_CONS_REQUEST_CONNECT;
    int length = 4; //Length is 4 because otherwise UDP gives out errors
    return length;
}

int send_cons_request_disconnect(unsigned char * buf){
    buf[0] = CONTROL_CONS_REQUEST_DISCONNECT;
    int length = 4;
    return length;
}

int send_req_list_stream(unsigned char * buf, char * filter) {
    buf[0] = CONTROL_REQ_LIST_STREAM;
    if (strcmp(filter, "") == 0){
        buf[0] = buf[0] | (~TYPE_MASK);
    } else {
        for (int i = 0; i < strlen(filter); ++i) {
            if (filter[i] == 'a'){buf[0] = buf[0] | AUDIO_BIT;}
            if (filter[i] == 'v'){buf[0] = buf[0] | VIDEO_BIT;}
            if (filter[i] == 't'){buf[0] = buf[0] | TEXT_BIT;}
        }
    }
    return 4; //length
}

int send_req_subscribe(unsigned char * buf, char * id) {
    buf[0] = CONTROL_REQ_SUBSCRIBE;
    char theID[4];
    hexStringToBytes(id, theID);
    memcpy(&buf[1], theID, 4);

    return 5; //length
}

int send_req_unsubscribe(unsigned char * buf, char * id, char * type) {
    buf[0] = CONTROL_REQ_UNSUBSCRIBE;

    char theID[4];
    hexStringToBytes(id, theID);
    memcpy(&buf[1], theID, 4);

    return 5; //length
}