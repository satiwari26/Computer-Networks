#include"window.h"

// Define global variables
struct windowVars globalWindow;
struct serverBuffer globalServerBuffer;

//-------------------Window functions---------------------------------//

void init(uint32_t sequenceNumber){
    //creating the buffer to store the data packets
    globalWindow.windowBuffer = (uint8_t**)malloc(globalWindow.windowSize * sizeof(uint8_t*));
    //initialize the window to the current sequence number
    globalWindow.lower = sequenceNumber;
    globalWindow.current = sequenceNumber;
    globalWindow.upper = globalWindow.lower + globalWindow.windowSize;

    //allocate the memory for each packet in the window, 1 extra byte to store the size of the packet
    for(int i=0;i<globalWindow.windowSize;i++){
        globalWindow.windowBuffer[i] = (uint8_t *)malloc(MAX_PACKET_SIZE + sizeof(uint16_t));
    }
}

void freeWindowBuffer(){
    //freeing the server window size for each block
    for(int i=0;i<globalWindow.windowSize;i++){
        if(globalWindow.windowBuffer[i] != NULL){
            free(globalWindow.windowBuffer[i]);
        }
    }

    //free the serverBuffer itself
    if(globalWindow.windowBuffer != NULL){
        free(globalWindow.windowBuffer);
    }
}

//packet-size needs to be constant in all packets make sure sending the same packetsize in each function call
void addPacket(uint8_t * packetPdu, uint16_t packetSize){
    uint32_t sequenceNumber_NO;
    memcpy(&sequenceNumber_NO, packetPdu, sizeof(uint32_t));
    uint32_t sequenceNumber_HO = ntohl(sequenceNumber_NO);

    uint16_t indexVal = sequenceNumber_HO % globalWindow.windowSize;

    //create temp packet that additionally store the size 
    uint8_t * tempDataPacket = (uint8_t *)malloc(packetSize + sizeof(uint16_t));

    memcpy(tempDataPacket, &packetSize, sizeof(uint16_t));
    memcpy(tempDataPacket + sizeof(uint16_t), packetPdu, packetSize);

    //add that tempPacket in the globalWindow.windowBuffer
    memcpy(globalWindow.windowBuffer[indexVal], tempDataPacket, packetSize + sizeof(uint16_t));
    free(tempDataPacket);
}

uint8_t * getPacket(uint32_t sequenceNumber){
    uint16_t indexVal = sequenceNumber % globalWindow.windowSize;
    return(globalWindow.windowBuffer[indexVal]);
}

void updateWindow(uint32_t rr_packetNumber){
    globalWindow.lower = rr_packetNumber;
    globalWindow.upper = globalWindow.lower + globalWindow.windowSize;
}

void printPacket(uint8_t * specificPacket){
    uint16_t packetSize;
    uint32_t sequenceNumber_NO;
    uint32_t sequenceNumber_HO;
    uint16_t checksum;
    uint8_t flag;
    memcpy(&packetSize, specificPacket, sizeof(uint16_t));
    memcpy(&sequenceNumber_NO, specificPacket + sizeof(uint16_t), sizeof(uint32_t));
    memcpy(&checksum, specificPacket + sizeof(uint16_t) + sizeof(uint32_t), sizeof(uint16_t));
    memcpy(&flag, specificPacket + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));

    uint8_t dataStore[packetSize - 7 + 1];  //remove the header from the total size

    dataStore[packetSize - 7] = '\0';

    memcpy(dataStore, specificPacket + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packetSize - 7);

    printf("Packet size: %d\n", packetSize);
    sequenceNumber_HO = ntohl(sequenceNumber_NO);
    printf("sequence number %d\n", sequenceNumber_HO);
    printf("checksum : %d\n",checksum);
    printf("flag %d\n", flag);
    printf("data: %s\n", dataStore);
}

void printEntireWindow(){
    for(int i=0;i<globalWindow.windowSize;i++){
        printPacket(globalWindow.windowBuffer[i]);
        printf("-----------------------end of this packet---------------\n");
    }
}

//-------------------buffer functions---------------------------------//

void initServerbuffer(){
    //creating the buffer to store the data packets
    globalServerBuffer.ServerBuffer = (uint8_t**)malloc(globalServerBuffer.serverWindowSize * sizeof(uint8_t*));

    //creating the validation buffer
    globalServerBuffer.validationBuffer = (int8_t*)malloc(globalServerBuffer.serverWindowSize);

    //allocate the memory for each packet in the server buffer
    for(int i=0;i<globalServerBuffer.serverWindowSize;i++){
        globalServerBuffer.ServerBuffer[i] = (uint8_t *)malloc(MAX_PACKET_SIZE + sizeof(uint16_t));
        memset(globalServerBuffer.ServerBuffer[i], '\0', MAX_PACKET_SIZE + sizeof(uint16_t)); //setting null char in the memset

        //setting the validation buffer to 0
        globalServerBuffer.validationBuffer[i] = invalid;
    }
}

void cleanServerUP(){
    //freeing the server window size for each block
    for(int i=0;i<globalServerBuffer.serverWindowSize;i++){
        if(globalServerBuffer.ServerBuffer[i] != NULL){
            free(globalServerBuffer.ServerBuffer[i]);
        }
    }

    //freeing the validation buffer
    if(globalServerBuffer.validationBuffer != NULL){
        free(globalServerBuffer.validationBuffer);
    }

    //free the serverBuffer itself
    if(globalServerBuffer.ServerBuffer != NULL){
        free(globalServerBuffer.ServerBuffer);
    }
}

void addServerPacket(uint8_t * packetPdu, uint16_t packetSize){
    uint32_t sequenceNumber_NO;
    memcpy(&sequenceNumber_NO, packetPdu, sizeof(uint32_t));
    uint32_t sequenceNumber_HO = ntohl(sequenceNumber_NO);

    uint16_t indexVal = sequenceNumber_HO % globalServerBuffer.serverWindowSize;

    memcpy(globalServerBuffer.ServerBuffer[indexVal],&packetSize, sizeof(uint16_t));

    //copy the content of the packetPdu to serverBuffer at that index
    memcpy(globalServerBuffer.ServerBuffer[indexVal] + sizeof(uint16_t), packetPdu, globalServerBuffer.serverBufferSize + 7);    //7 bytes for the header

    //set the validation index value to valid
    globalServerBuffer.validationBuffer[indexVal] = valid;
}

void flushPacket(uint32_t indexVal){
    //flush that specific buffer for more space
    memset(globalServerBuffer.ServerBuffer[indexVal], '\0', MAX_PACKET_SIZE + sizeof(uint16_t));

    //set the validation index value to invalid
    globalServerBuffer.validationBuffer[indexVal] = invalid;
}

uint8_t * getServerPacket(uint32_t sequenceNumber){
    uint16_t indexVal = sequenceNumber % globalServerBuffer.serverWindowSize;
    return(globalServerBuffer.ServerBuffer[indexVal]);
}

void invalidationCheck(){
    if(globalServerBuffer.receive < globalServerBuffer.highest){    //if receive is lower than the expected
        return;
    }

    for(uint32_t i = globalServerBuffer.receive + 1; i < globalServerBuffer.highest; i++){
        //set all the missing files in between to invalid
        uint32_t indexVal = i % globalServerBuffer.serverWindowSize;
        globalServerBuffer.validationBuffer[indexVal] = invalid;
    }
}

void printServerPacket(uint8_t * specificPacket){
    uint32_t sequenceNumber_NO;
    uint32_t sequenceNumber_HO;
    uint16_t checksum;
    uint8_t flag;
    uint16_t packetSize;
    memcpy(&packetSize, specificPacket, sizeof(uint16_t));
    memcpy(&sequenceNumber_NO, specificPacket + sizeof(uint16_t), sizeof(uint32_t));
    memcpy(&checksum, specificPacket + sizeof(uint16_t) + sizeof(uint32_t), sizeof(uint16_t));
    memcpy(&flag, specificPacket + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));

    uint8_t dataStore[globalServerBuffer.serverBufferSize + 1];  //remove the header from the total size

    dataStore[globalServerBuffer.serverBufferSize] = '\0';

    memcpy(dataStore, specificPacket + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), globalServerBuffer.serverBufferSize);

    printf("Packet size: %d\n", packetSize);
    sequenceNumber_HO = ntohl(sequenceNumber_NO);
    printf("sequence number %d\n", sequenceNumber_HO);
    printf("checksum : %d\n",checksum);
    printf("flag %d\n", flag);
    printf("data: %s\n", dataStore);
}

void printServerEntireWindow(){
    for(int i=0;i<globalServerBuffer.serverWindowSize;i++){
        printServerPacket(globalServerBuffer.ServerBuffer[i]);
        printf("validation: %d\n", globalServerBuffer.validationBuffer[i]);
        printf("-----------------------end of this packet---------------\n");
    }
}




//-------------------------Testing script for server window function--------------------//
// int main(){
//     //setup the server buffer and server window size
//     globalServerBuffer.serverBufferSize = 24;
//     globalServerBuffer.serverWindowSize = 4;

//     initServerbuffer();

//     uint32_t seq1 = htonl(1);
//     uint8_t packet1Data[] = "This is packet 1.";
//     uint16_t checksum = 0;
//     uint8_t flag = 1;
    
//     uint8_t packetSize = strlen((char *)packet1Data) + 7;
//     uint8_t packet1[packetSize];

//     memcpy(packet1, &seq1,sizeof(uint32_t));
//     memcpy(packet1 + sizeof(uint32_t), &checksum,sizeof(uint16_t));
//     memcpy(packet1 + sizeof(uint32_t) + sizeof(uint16_t), &flag,sizeof(uint8_t));
//     memcpy(packet1 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packet1Data, packetSize - 7);

//     addServerPacket(packet1, packetSize);

//     uint32_t seq2 = htonl(2);
//     uint8_t packet2Data[] = "This is packet 2.";
//     uint16_t checksum2 = 0;
//     uint8_t flag2 = 1;

//     uint8_t packetSize2 = strlen((char *)packet2Data) + 7;
//     uint8_t packet2[packetSize2];

//     memcpy(packet2, &seq2,sizeof(uint32_t));
//     memcpy(packet2 + sizeof(uint32_t), &checksum2,sizeof(uint16_t));
//     memcpy(packet2 + sizeof(uint32_t) + sizeof(uint16_t), &flag2,sizeof(uint8_t));
//     memcpy(packet2 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packet2Data, packetSize2 - 7);

//     addServerPacket(packet2, packetSize2);

//     uint32_t seq3 = htonl(3);
//     uint8_t packet3Data[] = "This is packet 3.";
//     uint16_t checksum3 = 0;
//     uint8_t flag3 = 1;

//     uint8_t packetSize3 = strlen((char *)packet3Data) + 7;
//     uint8_t packet3[packetSize3];

//     memcpy(packet3, &seq3,sizeof(uint32_t));
//     memcpy(packet3 + sizeof(uint32_t), &checksum3,sizeof(uint16_t));
//     memcpy(packet3 + sizeof(uint32_t) + sizeof(uint16_t), &flag3,sizeof(uint8_t));
//     memcpy(packet3 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packet3Data, packetSize3 - 7);

//     addServerPacket(packet3, packetSize3);

//     uint32_t seq4 = htonl(4);
//     uint8_t packet4Data[] = "This is packet 4.";
//     uint16_t checksum4 = 0;
//     uint8_t flag4 = 1;

//     uint8_t packetSize4 = strlen((char *)packet4Data) + 7;
//     uint8_t packet4[packetSize4];

//     memcpy(packet4, &seq4,sizeof(uint32_t));
//     memcpy(packet4 + sizeof(uint32_t), &checksum4,sizeof(uint16_t));
//     memcpy(packet4 + sizeof(uint32_t) + sizeof(uint16_t), &flag4,sizeof(uint8_t));
//     memcpy(packet4 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packet4Data, packetSize4 - 7);

//     addServerPacket(packet4, packetSize4);

//     uint8_t * gettingPacket = getServerPacket(3);
//     printServerPacket(gettingPacket);
//     printf("--------------------------------------------\n");

//     printServerEntireWindow();
// }




//-------------------testing script for window function---------------------------------//


// int main(){
//     globalWindow.windowSize = 4;

//     init();

//     uint32_t seq1 = htonl(1);
//     uint8_t packet1Data[] = "This is packet 1.";
//     uint16_t checksum = 0;
//     uint8_t flag = 1;
    
//     uint8_t packetSize = strlen((char *)packet1Data) + 7;
//     uint8_t packet1[packetSize];

//     memcpy(packet1, &seq1,sizeof(uint32_t));
//     memcpy(packet1 + sizeof(uint32_t), &checksum,sizeof(uint16_t));
//     memcpy(packet1 + sizeof(uint32_t) + sizeof(uint16_t), &flag,sizeof(uint8_t));
//     memcpy(packet1 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packet1Data, packetSize - 7);

//     addPacket(packet1, packetSize);

//     uint32_t seq2 = htonl(2);
//     uint8_t packet2Data[] = "This is packet 2.";
//     uint16_t checksum2 = 0;
//     uint8_t flag2 = 1;
//     uint8_t packetSize2 = strlen((char *)packet2Data) + 7;
//     uint8_t packet2[packetSize2];
//     memcpy(packet2, &seq2,sizeof(uint32_t));
//     memcpy(packet2 + sizeof(uint32_t), &checksum2,sizeof(uint16_t));
//     memcpy(packet2 + sizeof(uint32_t) + sizeof(uint16_t), &flag2,sizeof(uint8_t));
//     memcpy(packet2 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packet2Data, packetSize2 - 7);
//     addPacket(packet2, packetSize2);

//     uint32_t seq3 = htonl(3);
//     uint8_t packet3Data[] = "This is packet 3.";
//     uint16_t checksum3 = 0;
//     uint8_t flag3 = 1;
//     uint8_t packetSize3 = strlen((char *)packet3Data) + 7;
//     uint8_t packet3[packetSize3];
//     memcpy(packet3, &seq3,sizeof(uint32_t));
//     memcpy(packet3 + sizeof(uint32_t), &checksum3,sizeof(uint16_t));
//     memcpy(packet3 + sizeof(uint32_t) + sizeof(uint16_t), &flag3,sizeof(uint8_t));
//     memcpy(packet3 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packet3Data, packetSize3 - 7);
//     addPacket(packet3, packetSize3);

//     // Create packet 4
//     uint32_t seq4 = htonl(4);
//     uint8_t packet4Data[] = "This is packet 4.";
//     uint16_t checksum4 = 0;
//     uint8_t flag4 = 1;
//     uint8_t packetSize4 = strlen((char *)packet4Data) + 7;
//     uint8_t packet4[packetSize4];
//     memcpy(packet4, &seq4, sizeof(uint32_t));
//     memcpy(packet4 + sizeof(uint32_t), &checksum4, sizeof(uint16_t));
//     memcpy(packet4 + sizeof(uint32_t) + sizeof(uint16_t), &flag4, sizeof(uint8_t));
//     memcpy(packet4 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), packet4Data, packetSize4 - 7);
//     addPacket(packet4, packetSize4);

//     uint8_t * gettingPacket = getPacket(3);
//     printPacket(gettingPacket);


//     printf("\n\n\nPrinting entire window....................\n");
//     printEntireWindow();

// }