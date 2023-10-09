//
// Created by rowan on 09/10/2023.
//
int send_cons_request_connect(unsigned char * buf){
    buf[0] = CONTROL_CONS_REQUEST_CONNECT;
    int length = 4; //Length is 4 because otherwise UDP gives out errors
    return length;
}