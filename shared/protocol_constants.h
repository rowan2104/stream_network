//
// Created by rowan on 01/10/2023.
//

#ifndef STREAM_NETWORK_PROTOCOL_CONSTANTS_H
#define STREAM_NETWORK_PROTOCOL_CONSTANTS_H

const unsigned char CONTROL_CONS_REQUEST_CONNECT = 0b10010000;
const unsigned char CONTROL_CONS_CONNECT = 0b10011000;

const unsigned char CONTROL_PROD_REQUEST_CONNECT = 0b11011000;
const unsigned char CONTROL_PROD_CONNECT = 0b11001000;

const unsigned char CONTROL_REQ_SUBSCRIBE = 0b11001000;
const unsigned char CONTROL_SUBSCRIBE = 0b10101000;

const unsigned char CONTROL_REQUEST_STREAM_UPDATE = 0b11111000; //bits 1,2,3 are variable
const unsigned char TEXT_BIT = 0b00000100; //TEXT
const unsigned char AUDIO_BIT = 0b00000010; //AUDIO
const unsigned char VIDEO_BIT = 0b00000001; //VIDEO
const unsigned char TYPE_MASK = 0b11111000;
const unsigned char CONTROL_STREAM_UPDATE = 0b11101000; //bits 1,2,3 are variable

const unsigned char CONTROL_REQ_LIST_STREAM = 0b11110000;
const unsigned char CONTROL_LIST_STREAM = 0b11100000;


const unsigned char DATA_VIDEO_FRAME = 0b00000001;
const unsigned char DATA_AUDIO_FRAME = 0b00000010;
const unsigned char DATA_TEXT_FRAME = 0b00000100;

const unsigned char ERROR = 0b00000000;

const unsigned char ERROR_PROD_ALREADY_EXISTS = 0b00000001;

#endif //STREAM_NETWORK_PROTOCOL_CONSTANTS_H
