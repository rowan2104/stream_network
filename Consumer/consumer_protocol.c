//
// Created by rowan on 09/10/2023.
//
int send_cons_request_connect(unsigned char * buf){
    buf[0] = CONTROL_CONS_REQUEST_CONNECT;
    int length = 4; //Length is 4 because otherwise UDP gives out errors
    return length;
}

int send_req_list_stream(unsigned char * buf, char * filter) {
    buf[0] = CONTROL_REQ_LIST_STREAM;
    if (strcmp(filter, "") == 0){
        buf[0] = buf[0] | 0b01110000;
    } else {
        for (int i = 0; i < strlen(filter); ++i) {
            if (filter[i] == 'a'){buf[0] = buf[0] | AUDIO_BIT;}
            if (filter[i] == 'v'){buf[0] = buf[0] | VIDEO_BIT;}
            if (filter[i] == 't'){buf[0] = buf[0] | TEXT_BIT;}
        }
    }
    return 4; //length
}