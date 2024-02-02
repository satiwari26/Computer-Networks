/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

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
#include <stdint.h>
#include <stdbool.h>

#include "networks.h"
#include "safeUtil.h"
#include "sendPDU.h"
#include "pollLib.h"
#include "recvPDU.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1
#define MAX_HANDLE_SIZE 100
#define CLIENT_INIT_FLAG 1
#define MAX_MESSAGE_LENGTH 200
#define INDIVIDUAL_PACKET_FLAG 5
#define GOOD_HANDEL_FLAG 2
#define MULTI_CAST_FLAG 6
#define ERROR_MESSAGE_FLAG 7
#define HANDLE_LIST_FLAG 10
#define SERVER_HANDEL_FLAG 11
#define EACH_HANDEL_SEND_FLAG 12
#define LIST_END_FLAG 13
#define BROADCAST_FLAG 4


void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);

/**
 * @brief
 * struct defined for the %M and %C commands
*/
struct MultiSingleDestiHeader{
	uint8_t senderHandelLen;
	uint8_t * senderHandelName;
	uint8_t destinationHandels;
	uint8_t destHandelLen;
	uint8_t * destHandelName;
};

int min(int x, int y) {
    return (x < y) ? x : y;
}

/**
 * @brief
 * process the stdin, get the data from the stdin and send it to the server
 * @param
 * takes in the server socketNum
*/
void processStdin(int socketNum, char * argv[]){
	uint8_t sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it*/

	char sendHandleName[strlen(argv[1])];
	int sendHandelLen = strlen(argv[1]);
	strcpy(sendHandleName,argv[1]);	//save the handel name of the user

	//buffer to split the message in parts and then send
	uint8_t messageBuff[MAX_MESSAGE_LENGTH];

	char messageCommands[3];	//to store the message commands
	messageCommands[2] = '\0';	//set the null terminated char
	struct MultiSingleDestiHeader *singleDesHeader = (struct MultiSingleDestiHeader *)malloc(sizeof(struct MultiSingleDestiHeader));
	
	sendLen = readFromStdin(sendBuf);
	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);

	//perform the parsing of the data from the user input
	//parsing the commands
	memcpy(messageCommands, sendBuf, sizeof(uint8_t));	//copy the first two chars
	memcpy(messageCommands + sizeof(uint8_t), sendBuf + sizeof(uint8_t), sizeof(uint8_t));

	//for directMessage handelName
	char destHandleName[MAX_HANDLE_SIZE];
	//creating 9 different static array for multicast handling
	char destHandleName1[9][MAX_HANDLE_SIZE];	//9 array of size max_handel_size 
	uint8_t destHandleNameLenghts1[9];	//to store the length of 
	
	if(strcmp(messageCommands, "%M") == 0 || strcmp(messageCommands, "%m") == 0){	//sending individual message
		sscanf((char *)sendBuf, "%s %s",messageCommands,destHandleName);
		singleDesHeader->destHandelLen = strlen(destHandleName);	//destination handel name length

		singleDesHeader->destinationHandels = 1;	//hard set to one in single message sending
	}
	else if(strcmp(messageCommands, "%C") == 0 || strcmp(messageCommands, "%c") == 0){	//sending multicasting message
		//store the 9 elements handelName
		sscanf((char *)sendBuf, "%s %hhu %s %s %s %s %s %s %s %s %s",
                                              messageCommands,
											  &singleDesHeader->destinationHandels,
                                              destHandleName1[0],
                                              destHandleName1[1],
                                              destHandleName1[2],
                                              destHandleName1[3],
                                              destHandleName1[4],
                                              destHandleName1[5],
                                              destHandleName1[6],
                                              destHandleName1[7],
                                              destHandleName1[8]);
		//get the size of the each handel Name:
		for(int i=0; i<singleDesHeader->destinationHandels;i++){
			destHandleNameLenghts1[i] = strlen(destHandleName1[i]);
		}
	}

	printf("Message Command: %s\n",messageCommands);
	singleDesHeader->senderHandelLen = sendHandelLen;
	singleDesHeader->senderHandelName = (uint8_t *)sendHandleName;
	//package length containing the header info + dynamic message length
	int packagesLength = sizeof(uint8_t) + sendHandelLen + sizeof(uint8_t);

	//dynamically sending the direct message to the user
	if(strcmp(messageCommands, "%M") == 0 || strcmp(messageCommands, "%m") == 0){	//sending individual message
		packagesLength += sizeof(uint8_t) + singleDesHeader->destHandelLen;
		//subtracting the destination header + command bytes from the total user length
		int sendermessageLength = sendLen - (singleDesHeader->destHandelLen + 3) -1;
		if(sendermessageLength > MAX_MESSAGE_LENGTH-1){
			for (int i = (singleDesHeader->destHandelLen + 3 + 1); i < sendLen; i += min(sendLen - i, MAX_MESSAGE_LENGTH - 1)) {
				int chunkSize = min(sendLen - i, MAX_MESSAGE_LENGTH - 1);
				memcpy(messageBuff, sendBuf + i, chunkSize);
				messageBuff[chunkSize] = '\0';
				packagesLength += chunkSize;	//dynamic packageLength
				
				uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);	//dynamic packageBuffer
				memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
				memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
				memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, &singleDesHeader->destinationHandels, sizeof(uint8_t));
				memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t), &singleDesHeader->destHandelLen, sizeof(uint8_t));
				memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t), destHandleName, singleDesHeader->destHandelLen);
				memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t) + singleDesHeader->destHandelLen, messageBuff, chunkSize);
				//null terminating the last char of the package buffer(message buffer)
				*(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t) + singleDesHeader->destHandelLen + chunkSize) = '\0';

				sent =  sendPDU(socketNum, INDIVIDUAL_PACKET_FLAG, packageBuff, packagesLength);
				packagesLength -= chunkSize;	
				if (sent < 0)
				{
					perror("send call");
					exit(-1);
				}
				printf("Amount of data sent is: %d\n", sent);
			}
		}
		else if(sendermessageLength == 0){	//if no message is provided
			messageBuff[0] = '\n';
			packagesLength += sizeof(uint8_t);
			uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);
			memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, &singleDesHeader->destinationHandels, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t), &singleDesHeader->destHandelLen, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t), destHandleName, singleDesHeader->destHandelLen);
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t) + singleDesHeader->destHandelLen, messageBuff, sizeof(uint8_t));
			sent =  sendPDU(socketNum, INDIVIDUAL_PACKET_FLAG, packageBuff, packagesLength);
			if (sent < 0)
			{
				perror("send call");
				exit(-1);
			}
			printf("Amount of data sent is: %d\n", sent);
		}
		else{	//message is within the bounds
			memcpy(messageBuff,sendBuf + singleDesHeader->destHandelLen + 3 + 1, sendermessageLength);
			packagesLength += sendermessageLength;
			uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);
			memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, &singleDesHeader->destinationHandels, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t), &singleDesHeader->destHandelLen, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t), destHandleName, singleDesHeader->destHandelLen);
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t) + singleDesHeader->destHandelLen, messageBuff, sendermessageLength);
			messageBuff[sendermessageLength] = '\0';
			sent =  sendPDU(socketNum, INDIVIDUAL_PACKET_FLAG, packageBuff, packagesLength);
			if (sent < 0)
			{
				perror("send call");
				exit(-1);
			}
			printf("Amount of data sent is: %d\n", sent);
		}
	}
	else if(strcmp(messageCommands, "%C") == 0 || strcmp(messageCommands, "%c") == 0){	//sending multicasting message
		//updating the package length for each of the client destHandelName
		for(int i=0;i<singleDesHeader->destinationHandels;i++){
			packagesLength += sizeof(uint8_t) + destHandleNameLenghts1[i];
		}

		//subtracting the destination header + command bytes from the total user length
		int sendermessageLength = sendLen - 5;	//first 5 bytes for messageCommands, 1 byte for padding
		for(int i=0;i<singleDesHeader->destinationHandels;i++){	//delete the n number of handle size from the sendLen
			sendermessageLength -=  (destHandleNameLenghts1[i] + 1);
		}

		if(sendermessageLength > MAX_MESSAGE_LENGTH-1){
			//to advace the pointer by all the handles name
			int valAdvance = 0;	
			for(int t=0;t<singleDesHeader->destinationHandels;t++){
				valAdvance += destHandleNameLenghts1[t] + 1;	//1 for each space
			}

			for (int i = (valAdvance + 5); i < sendLen; i += min(sendLen - i, MAX_MESSAGE_LENGTH - 1)) {
				int chunkSize = min(sendLen - i, MAX_MESSAGE_LENGTH - 1);
				memcpy(messageBuff, sendBuf + i, chunkSize);
				messageBuff[chunkSize] = '\0';
				packagesLength += chunkSize;	//dynamic packageLength
				
				uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);	//dynamic packageBuffer
				memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
				memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
				memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, &singleDesHeader->destinationHandels, sizeof(uint8_t));

				//need to dynamically allocate handelName, handelData to the packageBuff
				int dynamicPackageBuff = sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t);
				for(int t=0;t<singleDesHeader->destinationHandels;t++){
					memcpy(packageBuff + dynamicPackageBuff, &destHandleNameLenghts1[t], sizeof(uint8_t));
					dynamicPackageBuff += sizeof(uint8_t);
					memcpy(packageBuff + dynamicPackageBuff,destHandleName1[t], destHandleNameLenghts1[t]);
					dynamicPackageBuff += destHandleNameLenghts1[t];
				}

				// memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t), &singleDesHeader->destHandelLen, sizeof(uint8_t));
				// memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t), destHandleName, singleDesHeader->destHandelLen);
				memcpy(packageBuff + dynamicPackageBuff, messageBuff, chunkSize);
				
				sent =  sendPDU(socketNum, MULTI_CAST_FLAG, packageBuff, packagesLength);
				packagesLength -= chunkSize;	
				if (sent < 0)
				{
					perror("send call");
					exit(-1);
				}
				printf("Amount of data sent is: %d\n", sent);
			}
		}
		else if(sendermessageLength == 0){	//if no message is provided
			messageBuff[0] = '\n';
			packagesLength += sizeof(uint8_t);
			uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);
			memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, &singleDesHeader->destinationHandels, sizeof(uint8_t));
			
			//need to dynamically allocate handelName, handelData to the packageBuff
			int dynamicPackageBuff = sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t);
			for(int t=0;t<singleDesHeader->destinationHandels;t++){
				memcpy(packageBuff + dynamicPackageBuff, &destHandleNameLenghts1[t], sizeof(uint8_t));
				dynamicPackageBuff += sizeof(uint8_t);
				memcpy(packageBuff + dynamicPackageBuff,destHandleName1[t], destHandleNameLenghts1[t]);
				dynamicPackageBuff += destHandleNameLenghts1[t];
			}

			// memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t), &singleDesHeader->destHandelLen, sizeof(uint8_t));
			// memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t), destHandleName, singleDesHeader->destHandelLen);
			memcpy(packageBuff + dynamicPackageBuff, messageBuff, sizeof(uint8_t));
			sent =  sendPDU(socketNum, MULTI_CAST_FLAG, packageBuff, packagesLength);
			if (sent < 0)
			{
				perror("send call");
				exit(-1);
			}
			printf("Amount of data sent is: %d\n", sent);
		}
		else{	//message is within the bounds
			//getting the starting position for the sendBug
			int valAdvance = 0;
			for(int t=0;t<singleDesHeader->destinationHandels;t++){
				valAdvance += destHandleNameLenghts1[t] + 1;	//1 for the space
			}

			memcpy(messageBuff,sendBuf + valAdvance + 5, sendermessageLength);
			packagesLength += sendermessageLength;
			uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);
			memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, &singleDesHeader->destinationHandels, sizeof(uint8_t));

			//need to dynamically allocate handelName, handelData to the packageBuff
			int dynamicPackageBuff = sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t);
			for(int t=0;t<singleDesHeader->destinationHandels;t++){
				memcpy(packageBuff + dynamicPackageBuff, &destHandleNameLenghts1[t], sizeof(uint8_t));
				dynamicPackageBuff += sizeof(uint8_t);
				memcpy(packageBuff + dynamicPackageBuff,destHandleName1[t], destHandleNameLenghts1[t]);
				dynamicPackageBuff += destHandleNameLenghts1[t];
			}

			// memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t), &singleDesHeader->destHandelLen, sizeof(uint8_t));
			// memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t), destHandleName, singleDesHeader->destHandelLen);
			messageBuff[sendermessageLength] = '\0';
			memcpy(packageBuff + dynamicPackageBuff, messageBuff, sendermessageLength);
			sent =  sendPDU(socketNum, MULTI_CAST_FLAG, packageBuff, packagesLength);
			if (sent < 0)
			{
				perror("send call");
				exit(-1);
			}
			printf("Amount of data sent is: %d\n", sent);
		}
	}
	else if(strcmp(messageCommands, "%L") == 0 || strcmp(messageCommands, "%l") == 0){	//Requesting the list of handel from server
		uint8_t EmptyBuff[0];	//for forming the flag 10 packet
		int dataSend = sendPDU(socketNum, HANDLE_LIST_FLAG, EmptyBuff,0);	//seding the empty buffer and only the LIST_FLAG
		if (dataSend < 0)
		{
			perror("send call");
			exit(-1);
		}
		printf("Amount of data sent is: %d\n", dataSend);
	}
	else if(strcmp(messageCommands, "%B") == 0 || strcmp(messageCommands, "%b") == 0){	//Boradcasting the message:	

		packagesLength -= sizeof(uint8_t);	//since there is no destination handles, we don't need the extra byte

		//subtracting the command bytes from the total user length
		int sendermessageLength = sendLen - 3;
		if(sendermessageLength > MAX_MESSAGE_LENGTH-1){
			for (int i = 3; i < sendLen; i += min(sendLen - i, MAX_MESSAGE_LENGTH - 1)) {
				int chunkSize = min(sendLen - i, MAX_MESSAGE_LENGTH - 1);
				memcpy(messageBuff, sendBuf + i, chunkSize);
				messageBuff[chunkSize] = '\0';
				packagesLength += chunkSize;	//dynamic packageLength
				
				uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);	//dynamic packageBuffer
				memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
				memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
				memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, messageBuff, chunkSize);

				sent =  sendPDU(socketNum, BROADCAST_FLAG, packageBuff, packagesLength);
				packagesLength -= chunkSize;	
				if (sent < 0)
				{
					perror("send call");
					exit(-1);
				}
				printf("Amount of data sent is: %d\n", sent);
			}
		}
		else if(sendermessageLength == 0){	//if no message is provided
			packagesLength += sizeof(uint8_t);
			messageBuff[0] = '\n';
			uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);
			memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, messageBuff, sizeof(uint8_t));
			sent =  sendPDU(socketNum, BROADCAST_FLAG, packageBuff, packagesLength);
			if (sent < 0)
			{
				perror("send call");
				exit(-1);
			}
			printf("Amount of data sent is: %d\n", sent);
		}
		else{	//message is within the bounds
			memcpy(messageBuff,sendBuf + 3, sendermessageLength);
			packagesLength += sendermessageLength;
			uint8_t * packageBuff = (uint8_t *)malloc(packagesLength);
			memcpy(packageBuff, &singleDesHeader->senderHandelLen, sizeof(uint8_t));
			memcpy(packageBuff + sizeof(uint8_t), singleDesHeader->senderHandelName, singleDesHeader->senderHandelLen);
			memcpy(packageBuff + sizeof(uint8_t) + singleDesHeader->senderHandelLen, messageBuff, sendermessageLength);
			messageBuff[sendermessageLength] = '\0';
			sent =  sendPDU(socketNum, BROADCAST_FLAG, packageBuff, packagesLength);
			if (sent < 0)
			{
				perror("send call");
				exit(-1);
			}
			printf("Amount of data sent is: %d\n", sent);
		}
	}
}

/**
 * @brief
 * parsing the direct message from the server
 * @param databuffer
 * @param messageLen
*/
void parseDirectMessageFromServer(uint8_t * dataBuffer, int messageLen){
	struct MultiSingleDestiHeader * directMessageHead = (struct MultiSingleDestiHeader *)malloc(sizeof(struct MultiSingleDestiHeader));
	memcpy(&directMessageHead->senderHandelLen, dataBuffer, sizeof(uint8_t));
	uint8_t senderHandelName[directMessageHead->senderHandelLen + 1];
	senderHandelName[directMessageHead->senderHandelLen] = '\0';
	memcpy(senderHandelName,dataBuffer + sizeof(uint8_t), directMessageHead->senderHandelLen);
	memcpy(&directMessageHead->destinationHandels, dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen, sizeof(uint8_t));
	memcpy(&directMessageHead->destHandelLen, dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t), sizeof(uint8_t));
	uint8_t destinationHandelName[directMessageHead->destHandelLen];
	memcpy(destinationHandelName,dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t),directMessageHead->destHandelLen);
	
	int messageDataLen = messageLen - sizeof(uint8_t) - directMessageHead->senderHandelLen - 2*sizeof(uint8_t) - directMessageHead->destHandelLen;
	uint8_t messageData[messageDataLen];
	memcpy(messageData, dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t) + directMessageHead->destHandelLen, messageDataLen);
	messageData[messageDataLen] = '\0';	//add the null termination before printing the data
	printf("\n%s: %s\n",senderHandelName, messageData);
}

/**
 * @brief
 * parsing the direct message from the server
 * @param databuffer
 * @param messageLen
*/
void processMultiCastMessageFromServer(uint8_t * dataBuffer, int messageLen){
	struct MultiSingleDestiHeader * directMessageHead = (struct MultiSingleDestiHeader *)malloc(sizeof(struct MultiSingleDestiHeader));
	memcpy(&directMessageHead->senderHandelLen, dataBuffer, sizeof(uint8_t));
	uint8_t senderHandelName[directMessageHead->senderHandelLen + 1];
	senderHandelName[directMessageHead->senderHandelLen] = '\0';
	memcpy(senderHandelName,dataBuffer + sizeof(uint8_t), directMessageHead->senderHandelLen);
	memcpy(&directMessageHead->destinationHandels, dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen, sizeof(uint8_t));

	//to store the length of each handles and handle names
	char destHandleName1[9][MAX_HANDLE_SIZE];
	uint8_t destHandleNameLenghts1[9];

	int valueOffset = sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t);
	for(int i=0; i<directMessageHead->destinationHandels;i++){
		memcpy(&destHandleNameLenghts1[i], dataBuffer + valueOffset, sizeof(uint8_t));	//grab the lenght of handel
		valueOffset = valueOffset + sizeof(uint8_t);
		memcpy(destHandleName1[i], dataBuffer + valueOffset, destHandleNameLenghts1[i]);
		valueOffset += destHandleNameLenghts1[i];
	}

	
	int messageDataLen = messageLen - sizeof(uint8_t) - directMessageHead->senderHandelLen - sizeof(uint8_t);
	//subtract the handel length and handel size from the messageLen
	for(int i=0;i<directMessageHead->destinationHandels;i++){
		messageDataLen -= (sizeof(uint8_t) + destHandleNameLenghts1[i]);
	}

	uint8_t messageData[messageDataLen];
	memcpy(messageData, dataBuffer + valueOffset, messageDataLen);
	messageData[messageDataLen] = '\0';
	//output the data to the clinet
	printf("\n%s: %s\n",senderHandelName, messageData);
}

/**
 * @brief
 * server returns the error when it doesn't find the
 * handel name in the handel list
 * @param dataBuffer
 * @param messageLen
*/
void ProcessMessageError(uint8_t * dataBuffer, int messageLen){
	//first bit contains the handelLength
	int handelLen = dataBuffer[0];
	uint8_t handelName[handelLen + 1];
	memcpy(handelName,dataBuffer + sizeof(uint8_t), handelLen);
	handelName[handelLen] = '\0';

	printf("\nClient with handle %s does not exist.\n",handelName);
}

/**
 * @brief
 * client request for the handel list from the server
 * server response with multiple flags.
 * Thread is blocked and no other poll is established
 * until flag 13 is received
 * @param dataBuffer
 * @param messageLen
*/
void handelNamesListHandler(uint8_t * dataBuffer, int messageLen){
	uint32_t num_handles;
	memcpy(&num_handles,dataBuffer,sizeof(uint32_t));
	uint32_t num_handles_HO = ntohl(num_handles);

	printf("\nNumber of clients: %d",num_handles_HO);

	//block the pointer in the handle until flag 13 is received
	uint8_t flag;
	uint32_t counter = 0;	//keep track if we received the same number of handles as promised

	removeFromPollSet(STDIN_FILENO);	//remove the stdin temporary
	bool isLastFlagReceived = false;
	while(isLastFlagReceived == false){

		int pollVal = pollCall(-1);	//block the poll untit we receive something from server
		
		if ((messageLen = recvPDU(pollVal,&flag, dataBuffer, MAXBUF)) < 0)
		{
			perror("recv call");
			exit(-1);
		}
		if(flag == EACH_HANDEL_SEND_FLAG){
			uint8_t handelNamesLen;
			memcpy(&handelNamesLen,dataBuffer,sizeof(uint8_t));
			uint8_t handelNames[handelNamesLen + sizeof(uint8_t)];
			memcpy(&handelNames,dataBuffer + sizeof(uint8_t),handelNamesLen);
			handelNames[handelNamesLen] = '\0';
			printf("\n\t%s",handelNames);
			fflush(stdout);
		}
		if(flag == LIST_END_FLAG && counter == num_handles_HO){
			break;
			isLastFlagReceived = true;
		}
		counter++;
	}
	addToPollSet(STDIN_FILENO);	//add back the stdin to the poll set
}

/**
 * @brief
 * processes the broadCasr message on the client side
 * and prints it on the console for the user
 * @param dataBuffer
 * @param messageLen
*/
void processBroadCastMessage(uint8_t * dataBuffer, int messageLen){
	struct MultiSingleDestiHeader * directMessageHead = (struct MultiSingleDestiHeader *)malloc(sizeof(struct MultiSingleDestiHeader));
	memcpy(&directMessageHead->senderHandelLen, dataBuffer, sizeof(uint8_t));
	uint8_t senderHandelName[directMessageHead->senderHandelLen + sizeof(uint8_t)];
	memcpy(senderHandelName,dataBuffer + sizeof(uint8_t), directMessageHead->senderHandelLen);
	senderHandelName[directMessageHead->senderHandelLen] = '\0';

	int dataLen = messageLen - sizeof(uint8_t) - directMessageHead->senderHandelLen;

	uint8_t message[dataLen]; 
	memcpy(message,dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen,dataLen);

	printf("\n%s: %s\n",senderHandelName, message);

}

/**
 * @brief processes the messgage from the server.
 * Also close the connection if the server has ended the program
 * @param
 * takes in the serverSocket to receive the message or change the message
*/
void processMsgFromServer(int serverSocket){
	uint8_t flag;
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	//now get the data from the client_socket
	if ((messageLen = recvPDU(serverSocket, &flag ,dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0)
	{
		if(flag == INDIVIDUAL_PACKET_FLAG){
			parseDirectMessageFromServer(dataBuffer, messageLen);
		}
		else if(flag == MULTI_CAST_FLAG){
			processMultiCastMessageFromServer(dataBuffer, messageLen);
		}
		else if(flag == ERROR_MESSAGE_FLAG){
			ProcessMessageError(dataBuffer, messageLen);
		}
		else if(flag == SERVER_HANDEL_FLAG){
			handelNamesListHandler(dataBuffer, messageLen);
		}
		else if(flag == BROADCAST_FLAG){
			processBroadCastMessage(dataBuffer,messageLen);
		}
		
	}
	else
	{
		printf("Connection closed by the server\n");
		//close the socket connection b/w client and server
		close(serverSocket);
		//remove the client from the socket-set
		removeFromPollSet(serverSocket);
		exit(0);
	}
}

/**
 * @brief
 * method handles the stdin and process message from the server
 * @param socketNum
 * param carries the server socketnumber to perfrom that operation
*/
void clientControl(int socketNum, char * argv[]){
	int pollVal = pollCall(-1);
	if(pollVal == STDIN_FILENO){
		processStdin(socketNum, argv);
	}
	else if(pollVal > 0){
		processMsgFromServer(pollVal);
	}
}

/**
 * @brief
 * performs the login request by sending the flag 1
 * @param val
 * @param socketNumber
 * @param argv
*/
int loginRequest(uint8_t * val, int socketNumber, char * argv[]){
	int bytesSend = sendPDU(socketNumber, CLIENT_INIT_FLAG, val, strlen(argv[1])+1);	//one extra byte for the length of handel name
	if (bytesSend < 0)
	{
		perror("send call");
		exit(-1);
	}
	uint8_t flag = 0;	//to get the flag value from the server
	//block the polling until we receive something from the server
	int pollVal = pollCall(-1);
	printf("%d\n",pollVal);
	if(pollVal > 0){
		uint8_t dataBuffer[MAXBUF];
		int messageLen = 0;
		//now get the data from the client_socket
		if ((messageLen = recvPDU(pollVal,&flag, dataBuffer, MAXBUF)) < 0)
		{
			perror("recv call");
			exit(-1);
		}
		fflush(stdout);
		printf("Message len: %d\n",messageLen);
		printf("flag val from the server: %d\n",flag);
		fflush(stdout);
	}
	return flag;
}

/**
 * @brief 
 * send the initial pdu to the server and wait for the receive until
 * the server responds back
 * 
 * @param socketNumber, @param * argv[]
*/
int setUpConnection(int socketNumber, char * argv[]){
	//add the sockeNumber to the pollset so ack can also be considered
	addToPollSet(socketNumber);
	uint8_t val[strlen(argv[1]) + 1]; // +1 for the length byte

    // First byte contains the length of the handle without null
    val[0] = strlen(argv[1]);
	printf("%d",val[0]);

    // Copy the handle name into val starting from index 1
    memcpy(val + sizeof(uint8_t), argv[1], val[0]);

	//performing the login request
	uint8_t ServerAckFlag = loginRequest(val, socketNumber, argv);

	//bad server response:
	if(ServerAckFlag == 3){
		printf("Error not the right Handel Name to perform the Operation.");
		exit(-1);
	}

	return ServerAckFlag;
	
}

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	checkArgs(argc, argv);
	//setup the polling for the client
	setupPollSet();
	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	//log in connection with the server
	uint8_t ServerAckFlag = setUpConnection(socketNum, argv);

	if(ServerAckFlag == 2){	//ok response from the server
		//add the stdin to the poll-set
		addToPollSet(STDIN_FILENO);

		while(1){
			fflush(stdout);
			printf("$: ");	//prompt the user to enter the information
			fflush(stdout);  
			clientControl(socketNum,argv);
		}
	}
	
	return 0;
}

int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;      
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	// printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}

	if(sizeof(argv[1]) > MAX_HANDLE_SIZE){
		printf("The handle length increased the max size buffer\n Current provided name: %s\n",argv[1]);
		exit(-1);
	}
}
