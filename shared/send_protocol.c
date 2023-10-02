//
// Created by rowan on 01/10/2023.
//

#include "protocol_constants.h"

int prot_request_connect(unsigned char * buf, char id[3]){
    buf[0] = CONTROL_REQUEST_CONNECT;
    memcpy(&buf[1], id, 3);
    int length = 4;
    return length;
}