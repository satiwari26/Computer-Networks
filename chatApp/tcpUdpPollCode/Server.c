/******************************************************************************
* myServer.c
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
#include "recvPDU.h"
#include "sendPDU.h"
#include "pollLib.h"
#include "handleStruct.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1
#define GOOD_HANDEL_FLAG 2
#define INIT_PACK_ERROR 3
#define INDIVIDUAL_PACKET_FLAG 5
#define CLIENT_INIT_FLAG 1
#define MULTI_CAST_FLAG 6
#define MAX_HANDLE_SIZE 100

/**
 * @brief
 * struct for direct Message(individual message)
*/
struct SingleDestiHeader{
	uint8_t senderHandelLen;
	uint8_t * senderHandelName;
	uint8_t destinationHandels;
	uint8_t destHandelLen;
	uint8_t * destHandelName;
};

// void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);

void printString(char * dataBuff, int sizeOfBuff){
	for(int i=0;i<sizeOfBuff;i++){
		printf("%c",dataBuff[i]);
	}
	printf("\n");
}

/**
 * @brief
 * parsing the direct message flag information
*/
void MultimodeMessage(uint8_t * dataBuffer, int messageLen){
	struct SingleDestiHeader * directMessageHead = (struct SingleDestiHeader *)malloc(sizeof(struct SingleDestiHeader));
	memcpy(&directMessageHead->senderHandelLen, dataBuffer, sizeof(uint8_t));
	uint8_t senderHandelName[directMessageHead->senderHandelLen];
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


	// memcpy(&directMessageHead->destHandelLen, dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t), sizeof(uint8_t));
	// uint8_t destinationHandelName[directMessageHead->destHandelLen];
	// memcpy(destinationHandelName,dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t),directMessageHead->destHandelLen);
	
	int messageDataLen = messageLen - sizeof(uint8_t) - directMessageHead->senderHandelLen - sizeof(uint8_t);
	//subtract the handel length and handel size from the messageLen
	for(int i=0;i<directMessageHead->destinationHandels;i++){
		messageDataLen -= (sizeof(uint8_t) + destHandleNameLenghts1[i]);
	}

	uint8_t messageData[messageDataLen];
	memcpy(messageData, dataBuffer + valueOffset, messageDataLen);
	printf("%s",messageData);
	fflush(stdout);
}

/**
 * @brief
 * parsing the direct message flag information
*/
void DirectMessage(uint8_t * dataBuffer, int messageLen){
	struct SingleDestiHeader * directMessageHead = (struct SingleDestiHeader *)malloc(sizeof(struct SingleDestiHeader));
	memcpy(&directMessageHead->senderHandelLen, dataBuffer, sizeof(uint8_t));
	uint8_t senderHandelName[directMessageHead->senderHandelLen];
	memcpy(senderHandelName,dataBuffer + sizeof(uint8_t), directMessageHead->senderHandelLen);
	memcpy(&directMessageHead->destinationHandels, dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen, sizeof(uint8_t));
	memcpy(&directMessageHead->destHandelLen, dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t), sizeof(uint8_t));
	uint8_t destinationHandelName[directMessageHead->destHandelLen];
	memcpy(destinationHandelName,dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t),directMessageHead->destHandelLen);
	
	int messageDataLen = messageLen - sizeof(uint8_t) - directMessageHead->senderHandelLen - 2*sizeof(uint8_t) - directMessageHead->destHandelLen;
	uint8_t messageData[messageDataLen];
	memcpy(messageData, dataBuffer + sizeof(uint8_t) + directMessageHead->senderHandelLen + sizeof(uint8_t) + sizeof(uint8_t) + directMessageHead->destHandelLen, messageDataLen);

	int destinationSocketNum = getSocketNumber((char *)destinationHandelName, directMessageHead->destHandelLen);
	if(destinationSocketNum == -1){
		printf("The handel doesn't exist.");
	}
	else{
		printf("Destination handel Port: %d",destinationSocketNum);
		int forwardDirectMessage = sendPDU(destinationSocketNum, GOOD_HANDEL_FLAG, (uint8_t *)dataBuffer, messageLen);
		if (forwardDirectMessage < 0)
		{
			perror("send call");
			exit(-1);
		}
	}

}

/**
 * @brief
 * confirms the handleName by checking in the handelSet
 * @param clientSocket
 * @param dataBuffer
*/
void confirmHandelName(int clientSocket, uint8_t * dataBuffer){
	//get the size of the data buffer
	uint8_t sizeOfHandel = dataBuffer[0];
	//store the data in the handelName
	char handelNameProvided[sizeOfHandel];
	memcpy(handelNameProvided,dataBuffer + sizeof(uint8_t), sizeOfHandel);
	printString(handelNameProvided,sizeOfHandel);
	
	//look for the handel name and send the ack
	int handelSocketCheck;
	handelSocketCheck = getSocketNumber(handelNameProvided, sizeOfHandel);
	if(handelSocketCheck == -1){
		printf("can't find the handel in the list\n");
		addNewHandle(handelNameProvided,clientSocket,sizeOfHandel);
		int sendingFlag = sendPDU(clientSocket, GOOD_HANDEL_FLAG, (uint8_t *)handelNameProvided, 0);
		if (sendingFlag < 0)
		{
			perror("send call");
			exit(-1);
		}
	}
	else{
		printf("the handel already exist please try something else\n");
		int sendingFlag = sendPDU(clientSocket, INIT_PACK_ERROR, (uint8_t *)handelNameProvided, 0);
		if (sendingFlag < 0)
		{
			perror("send call");
			exit(-1);
		}
		//close the socket connection b/w client and server
		close(clientSocket);
		//remove the client from the socket-set
		removeFromPollSet(clientSocket);
	}
}

/**
 * @brief
 * process the client's message that is sent to the server
 * @param clientSocket
*/
void processClient(int clientSocket){
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;

	uint8_t flag = 0;	//processing the flags send from the client
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, &flag,  dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	/**performs the login for the client*/
	if(flag == CLIENT_INIT_FLAG && messageLen > 0){
		confirmHandelName(clientSocket, dataBuffer);
	}
	else if(flag == INDIVIDUAL_PACKET_FLAG && messageLen > 0){
		DirectMessage(dataBuffer, messageLen);
	}
	else if(flag == MULTI_CAST_FLAG && messageLen > 0){
		MultimodeMessage(dataBuffer, messageLen);
	}

	if (messageLen > 0)
	{
		printf("Message received on socket: %u, Message Length: %d, Data: %s\n", clientSocket, messageLen, dataBuffer);
	}
	else
	{
		printf("Connection closed by other side\n");
		//close the socket connection b/w client and server
		close(clientSocket);
		//remove the client from the socket-set
		removeFromPollSet(clientSocket);
	}
}

/**
 * @brief
 * to add new socket to pollset
 * @param mainServerSocket clientSocket
*/
void addNewSocket(int mainServerSocket, int clientSocket){
	// wait for client to connect
	clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	addToPollSet(clientSocket);
}

void serverControl(int mainServerSocket, int clientSocket){
	int pollVal = pollCall(-1);
	if(pollVal == mainServerSocket){	//new client is trying to connect
		addNewSocket(mainServerSocket, clientSocket);
	}
	else if(pollVal > 0){	//client is trying to send a message to the server
		processClient(pollVal);
	}
}

int main(int argc, char *argv[])
{

	int clientSocket = 0;   //socket descriptor for the client socket
	int mainServerSocket = 0;   //socket descriptor for the server socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	//setup the poll for the sockets
	setupPollSet();
	//adding the main server socket to the pollset:
	addToPollSet(mainServerSocket);

	while(1){
		serverControl(mainServerSocket,clientSocket);
	}
	
	return 0;
}

// void recvFromClient(int clientSocket)
// {
// 	uint8_t dataBuffer[MAXBUF];
// 	int messageLen = 0;
	
// 	//now get the data from the client_socket
// 	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
// 	{
// 		perror("recv call");
// 		exit(-1);
// 	}

// 	if (messageLen > 0)
// 	{
// 		printf("Message received on socket: %u, Message Length: %d, Data: %s\n", clientSocket, messageLen, dataBuffer);
// 	}
// 	else
// 	{
// 		printf("Connection closed by other side\n");
// 	}
// }

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}

