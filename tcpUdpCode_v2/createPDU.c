#include "createPDU.h"


int createPDU(uint8_t **pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t * payload, int payLoadLen){

    //pduLength = (4 bytes for sequence num) + (2-bytes for checksum) + (1-byte for flag) + (payloadLen)
    int pduLength = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + payLoadLen;
    //dynamically create the pduBuffer
    *pduBuffer = (uint8_t*)malloc(pduLength);
    uint8_t pduBuff1[pduLength];

    uint16_t checkSum = 0;  //initially setting this to 0
    uint32_t sequenceNumber_NO = htonl(sequenceNumber);

    memcpy(pduBuff1, &sequenceNumber_NO, sizeof(uint32_t));
    memcpy(pduBuff1 + sizeof(uint32_t), &checkSum, sizeof(uint16_t));
    memcpy(pduBuff1 + sizeof(uint32_t) + sizeof(uint16_t), &flag, sizeof(uint8_t));
    memcpy(pduBuff1 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), payload, payLoadLen);

    memcpy(*pduBuffer,pduBuff1,pduLength);

    //running the checksum on the pdu packet
    checkSum = in_cksum((uint16_t *)pduBuff1, pduLength);

    //putting the new checksum value in the packet in the NO
    memcpy(pduBuff1 + sizeof(uint32_t), &checkSum, sizeof(uint16_t));

    //setting the buffer to passed in buffer
    memcpy(*pduBuffer,pduBuff1,pduLength);

    return pduLength;
}

void printPDU(uint8_t *aPDU, int pduLength){
    uint32_t sequenceNumber_NO = 0;
    uint32_t sequenceNumber_HO = 0;

    uint16_t checkSum = 0;
    uint8_t flag = 0;
    int payLoadLen = pduLength - sizeof(uint32_t) - sizeof(uint16_t) - sizeof(uint8_t);
    uint8_t payloadData[payLoadLen + 1];    //extra bytes to add the null terminated char
    payloadData[payLoadLen] = '\0';

    //getting the sequence number
    memcpy(&sequenceNumber_NO, aPDU, sizeof(uint32_t));
    sequenceNumber_HO = ntohl(sequenceNumber_NO);

    //verifying the checksum:
    checkSum = in_cksum((uint16_t*)aPDU, pduLength);
    checkSum == 0 ? printf("checksum verification successful\n") : printf("checksum verification failed.\n");

    //getting the flag
    memcpy(&flag,aPDU + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));

    //getting the payload data
    memcpy(payloadData,aPDU + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), payLoadLen);

    printf("Sequence Number: %d\n", sequenceNumber_HO);
    printf("Flag: %d\n", flag);
    printf("Payload: %s\n",payloadData);
    printf("Payload Length: %d\n",payLoadLen);
    fflush(stdout);
}

void extractFirstFilePacket(uint8_t * buffer, int payloadLen){

    memcpy(&globalServerBuffer.serverWindowSize,buffer, sizeof(uint32_t));
    memcpy(&globalServerBuffer.serverBufferSize, buffer + sizeof(uint32_t), sizeof(uint16_t));

    int toFileNameLen = payloadLen - sizeof(uint32_t) - sizeof(uint16_t);
    globalServerBuffer.toFileName = (char *)malloc(toFileNameLen + 1);
    memcpy(globalServerBuffer.toFileName, buffer + sizeof(uint32_t) + sizeof(uint16_t), toFileNameLen);
    globalServerBuffer.toFileName[toFileNameLen] = '\0';
}