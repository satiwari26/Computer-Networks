#include"sendPDU.h"

int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData){

    uint16_t pduLength = lengthOfData + sizeof(uint16_t);
    uint16_t pduLength_NO = htons(pduLength);   //convert the length to the network order

    uint8_t pduData[pduLength];

    memcpy(pduData,&pduLength_NO,sizeof(uint16_t)); //add the length of the PDU in the pdu-buffer
    memcpy(pduData + sizeof(uint16_t), dataBuffer, lengthOfData);   //add the data in the pdu-buffer

    int bytesSend = safeSend(clientSocket, pduData, pduLength, 0);  //safe send sent from the 

    return bytesSend;
}