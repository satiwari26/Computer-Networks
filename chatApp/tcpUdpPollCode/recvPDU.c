#include "recvPDU.h"

int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize){
    int bytesReceived = safeRecv(socketNumber, dataBuffer, sizeof(uint16_t), MSG_WAITALL);

    if(bytesReceived < 0 || bytesReceived == 0){    //if failed to received the pdu length
        return bytesReceived;
    }

    uint16_t pudLength_NO;
    memcpy(&pudLength_NO, dataBuffer,sizeof(uint16_t)); //copy first 16 bits to access the PDU length
    uint16_t pudLength_HO = ntohs(pudLength_NO);    //conver it to host order

    if(pudLength_HO > bufferSize) {
        printf("Error: Provided bufferSize for receive is not big enough.");
        return -1;
    }

    bytesReceived = safeRecv(socketNumber, dataBuffer, pudLength_HO - sizeof(uint16_t), MSG_WAITALL);

    return bytesReceived;
}