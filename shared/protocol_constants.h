//
// Created by rowan on 01/10/2023.
//

#ifndef STREAM_NETWORK_PROTOCOL_CONSTANTS_H
#define STREAM_NETWORK_PROTOCOL_CONSTANTS_H

const unsigned char CONTROL_CONS_REQUEST_CONNECT = 0b10000011;
const unsigned char CONTROL_CONS_CONNECT = 0b10000001;

const unsigned char CONTROL_PROD_REQUEST_CONNECT = 0b10001011;
const unsigned char CONTROL_PROD_CONNECT = 0b10001001;

const unsigned char CONTROL_REQUEST_STREAM_CREATE = 0b10001011; //bits 1,2,3 are variable
const unsigned char TEXT_BIT = 0b01000000; //TEXT
const unsigned char AUDIO_BIT = 0b00100000; //AUDIO
const unsigned char VIDEO_BIT = 0b00010000; //VIDEO
const unsigned char CONTROL_STREAM_CREATE = 0b10001001;

const unsigned char CONTROL_VIDEO_FRAME = 0b00010000;
const unsigned char CONTROL_TEXT_FRAME = 0b01000000;

const unsigned char ERROR = 0b11111111;

const unsigned char ERROR_PROD_ALREADY_EXISTS = 0b00000001;

#endif //STREAM_NETWORK_PROTOCOL_CONSTANTS_H
