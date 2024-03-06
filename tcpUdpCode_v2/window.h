#ifndef WINDOW_H
#define WINDOW_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_PACKET_SIZE 1407
#define valid 1
#define invalid -1
#define empty 0

//pointers to keep track where my upper, current and lower are for window buffer
struct windowVars{
    uint32_t windowSize;    //to create the window buffer of size windowSize
    uint16_t packetSize;
    uint8_t **windowBuffer; //window buffer to hold the packets
    uint32_t upper;
    uint32_t lower;
    uint32_t current;
};
extern struct windowVars globalWindow;


//-----window functions ----------

/**
 * @brief
 * setup the buffer to store the data packets
*/
void init(uint32_t sequenceNumber);

/**
 * @brief
 * adding the packet in the buffer based of the sequence number
*/
void addPacket(uint8_t * packetPdu, uint16_t packetSize);

/**
 * @brief
 * getting packet from the buffer based of sequence number
*/
uint8_t * getPacket(uint32_t sequenceNumber);

/**
 * @brief
 * updates the window based on the rr number is received
 * if rr is n lower = n, upper = lower + windowsize
*/
void updateWindow(uint32_t rr_packetNumber);

/**
 * @brief
 * print one specific packet
*/
void printPacket(uint8_t * specificPacket);

/**
 * @brief
 * print entire buffer packets
*/
void printEntireWindow();

/**
 * @brief
 * free window buffer and deallocating memory
*/
void freeWindowBuffer();


//------buffer functions----------//
struct serverBuffer{
    uint32_t highest;
    uint32_t expected;
    uint32_t receive;
    uint32_t serverBufferSize;  //the first packet send by the client has this information
    uint32_t serverWindowSize;  //the first packet also contains this information
    uint8_t **ServerBuffer; //Server buffer to hold the packets that are received later
    int8_t * validationBuffer;
    char * toFileName;  //to store the 
};

extern struct serverBuffer globalServerBuffer;

/**
 * @brief
 * initialize the server side of the buffer
*/
void initServerbuffer();

/**
 * @brief
 * add element to the server buffer
*/
void addServerPacket(uint8_t * packetPdu, uint16_t packetSize);

/**
 * @brief
 * flushes the specific index value value in the buffer
*/
void flushPacket(uint32_t indexVal);

/**
 * @brief
 * to print the server packet
*/
void printServerPacket(uint8_t * specificPacket);

/**
 * @brief
 * to print the entire server buffer
*/
void printServerEntireWindow();

/**
 * @brief
 * set the invalid bit if there are sequence # missing between received vs expected
 * 
 * NOTE: only execute when in the buffer state
*/
void invalidationCheck();

/**
 * @brief
 * to get the specific packet form the buffer
*/
uint8_t * getServerPacket(uint32_t sequenceNumber);

/**
 * @brief
 * clean the server buffers and validation memory allocation
*/
void cleanServerUP();

#endif