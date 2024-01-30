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


void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);

/**
 * @brief
 * struct defined for the %M and %C commands
*/
struct SingleDestiHeader{
	uint8_t senderHandelLen;
	uint8_t * senderHandelName;
	uint8_t destinationHandels;
	uint8_t destHandelLen;
	uint8_t * destHandelName;
};

int min(int x, int y) {
    return (x < y) ? x : y;
}

void parseUserInfo(uint8_t * sendBuf, char * messageCommands, struct SingleDestiHeader * singleDesHeader){
	//parsing the commands
	memcpy(messageCommands, sendBuf, sizeof(uint8_t));	//copy the first two chars
	memcpy(messageCommands + sizeof(uint8_t), sendBuf + sizeof(uint8_t), sizeof(uint8_t));

	char destHandleName[MAX_HANDLE_SIZE];
	sscanf((char *)sendBuf, "%s %s",messageCommands,destHandleName);
	
	if(strcmp(messageCommands, "%M") == 0 || strcmp(messageCommands, "%m") == 0){	//sending individual message
		singleDesHeader->destHandelLen = strlen(destHandleName);	//destination handel name length
		char destHandleNameData[strlen(destHandleName)];	//create dynamic handelName
		memcpy(destHandleNameData,destHandleName,strlen(destHandleName));	//copy the handel name
		singleDesHeader->destHandelName = (uint8_t *)destHandleNameData;	//point to the new allocated memory

		singleDesHeader->destinationHandels = 1;	//hard set to one in single message sending
	}
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
	struct SingleDestiHeader *singleDesHeader = (struct SingleDestiHeader *)malloc(sizeof(struct SingleDestiHeader));
	
	sendLen = readFromStdin(sendBuf);
	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);


	//perform the parsing of the data from the user input
	// parseUserInfo(sendBuf, messageCommands, singleDesHeader);
	//parsing the commands
	memcpy(messageCommands, sendBuf, sizeof(uint8_t));	//copy the first two chars
	memcpy(messageCommands + sizeof(uint8_t), sendBuf + sizeof(uint8_t), sizeof(uint8_t));

	char destHandleName[MAX_HANDLE_SIZE];
	sscanf((char *)sendBuf, "%s %s",messageCommands,destHandleName);
	
	//
	if(strcmp(messageCommands, "%M") == 0 || strcmp(messageCommands, "%m") == 0){	//sending individual message
		singleDesHeader->destHandelLen = strlen(destHandleName);	//destination handel name length

		singleDesHeader->destinationHandels = 1;	//hard set to one in single message sending
	}

	printf("Message Command: %s\n",messageCommands);
	singleDesHeader->senderHandelLen = sendHandelLen;
	singleDesHeader->senderHandelName = (uint8_t *)sendHandleName;
	//package length containing the header info + dynamic message length
	int packagesLength = sizeof(uint8_t) + sendHandelLen + sizeof(uint8_t) + sizeof(uint8_t) + singleDesHeader->destHandelLen;

	//dynamically sending the direct message to the user
	if(strcmp(messageCommands, "%M") == 0 || strcmp(messageCommands, "%m") == 0){	//sending individual message
		//subtracting the destination header + command bytes from the total user length
		int sendermessageLength = sendLen - (singleDesHeader->destHandelLen + 3) -1;
		if(sendermessageLength > MAX_MESSAGE_LENGTH-1){
			for (int i = (singleDesHeader->destHandelLen + 3); i < sendLen; i += MAX_MESSAGE_LENGTH - 1) {
				int chunkSize = min(sendermessageLength - i, MAX_MESSAGE_LENGTH - 1);
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
}

void parseDataFromServer(uint8_t * dataBuffer, int messageLen){
	struct SingleDestiHeader * directMessageHead = (struct SingleDestiHeader *)malloc(sizeof(struct SingleDestiHeader));
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
		// printf("Message received on socket: %u, Message Length: %d, Data: %s\n", serverSocket, messageLen, dataBuffer);
		if(flag == GOOD_HANDEL_FLAG){
			parseDataFromServer(dataBuffer, messageLen);
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
