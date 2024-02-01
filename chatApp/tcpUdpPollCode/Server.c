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
#define ERROR_MESSAGE_FLAG 7
#define HANDLE_LIST_FLAG 10
#define SERVER_HANDEL_FLAG 11
#define EACH_HANDEL_SEND_FLAG 12
#define LIST_END_FLAG 13

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
void MultimodeMessage(uint8_t * dataBuffer, int messageLen, int clientSocket){
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

	
	int messageDataLen = messageLen - sizeof(uint8_t) - directMessageHead->senderHandelLen - sizeof(uint8_t);
	//subtract the handel length and handel size from the messageLen
	for(int i=0;i<directMessageHead->destinationHandels;i++){
		messageDataLen -= (sizeof(uint8_t) + destHandleNameLenghts1[i]);
	}

	uint8_t messageData[messageDataLen];
	memcpy(messageData, dataBuffer + valueOffset, messageDataLen);

	//finding if the handles exist in the handel set and forwarding the messges based off that
	for(int i=0;i<directMessageHead->destinationHandels;i++){
		int destinationSocketNum = getSocketNumber(destHandleName1[i],destHandleNameLenghts1[i]);
		if(destinationSocketNum == -1){ //if handel name doesn't exist in the handel Set, send Error packet flag
			uint8_t handelError[sizeof(uint8_t) + destHandleNameLenghts1[i]];
			memcpy(handelError, &destHandleNameLenghts1[i], sizeof(uint8_t));
			memcpy(handelError + sizeof(uint8_t), destHandleName1[i],destHandleNameLenghts1[i]);
			int sendingFlag = sendPDU(clientSocket, ERROR_MESSAGE_FLAG, handelError, sizeof(uint8_t) + destHandleNameLenghts1[i]);
			if (sendingFlag < 0)
			{
				perror("send call");
				exit(-1);
			}
		}
		else{	//forward the message to all the clients
			printf("Destination handel Port: %d",destinationSocketNum);
			int forwardDirectMessage = sendPDU(destinationSocketNum, MULTI_CAST_FLAG, (uint8_t *)dataBuffer, messageLen);
			if (forwardDirectMessage < 0)
			{
				perror("send call");
				exit(-1);
			}
		}
	}

}

/**
 * @brief
 * parsing the direct message flag information
*/
void DirectMessage(uint8_t * dataBuffer, int messageLen, int clientSocket){
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
		uint8_t handelError[sizeof(uint8_t) + directMessageHead->destHandelLen];
		memcpy(handelError, &directMessageHead->destHandelLen, sizeof(uint8_t));
		memcpy(handelError + sizeof(uint8_t), destinationHandelName,directMessageHead->destHandelLen);
		int sendingFlag = sendPDU(clientSocket, ERROR_MESSAGE_FLAG, handelError, sizeof(uint8_t) + directMessageHead->destHandelLen);
		if (sendingFlag < 0)
		{
			perror("send call");
			exit(-1);
		}
	}
	else{
		printf("Destination handel Port: %d",destinationSocketNum);
		int forwardDirectMessage = sendPDU(destinationSocketNum, INDIVIDUAL_PACKET_FLAG, (uint8_t *)dataBuffer, messageLen);
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
 * Sends the flag 11 to the client along with the 32 bits ntos header
 * @param clientSocket
 * @param dataBuffer
*/
void ListRequest(int clientSocket, uint8_t * dataBuffer){
	uint32_t NumberOfHandels = 0;	//to store the number of the handles present
	uint8_t HandleNames[MAXBUF];	//to store the handle Names
	//get the handle names and number of handles
	NumberOfHandels = HandlesCount(HandleNames);


	//sending the number of handles back to client with flag 11
	uint32_t NumberOfHandels_NO = htonl(NumberOfHandels);
	uint8_t NumberOfHandels_NO_Buff[4];
	memcpy(&NumberOfHandels_NO_Buff[0],&NumberOfHandels_NO, sizeof(uint8_t));
	memcpy(&NumberOfHandels_NO_Buff[1],((uint8_t*)&NumberOfHandels_NO) + sizeof(uint8_t), sizeof(uint8_t));
	memcpy(&NumberOfHandels_NO_Buff[2],((uint8_t*)&NumberOfHandels_NO) + 2*sizeof(uint8_t), sizeof(uint8_t));
	memcpy(&NumberOfHandels_NO_Buff[3],((uint8_t*)&NumberOfHandels_NO) + 3*sizeof(uint8_t), sizeof(uint8_t));
	int sendResponseHandelRequest = sendPDU(clientSocket, SERVER_HANDEL_FLAG,NumberOfHandels_NO_Buff, sizeof(uint32_t));
	if (sendResponseHandelRequest < 0)
	{
		perror("send call");
		exit(-1);
	}

	//send the handel name with flag
	int valOffset = 0;
	uint8_t currHandelLen;
	for(int i=0; i<NumberOfHandels;i++){
		memcpy(&currHandelLen, HandleNames +valOffset, sizeof(uint8_t));
		uint8_t currHandleName[currHandelLen];
		memcpy(&currHandleName, HandleNames +valOffset + sizeof(uint8_t), currHandelLen);
		valOffset += (currHandelLen + sizeof(uint8_t));

		uint8_t handelNamesBuffer[currHandelLen + sizeof(uint8_t)];
		memcpy(handelNamesBuffer, &currHandelLen, sizeof(uint8_t));
		memcpy(handelNamesBuffer + sizeof(uint8_t), currHandleName, currHandelLen);

		int sendingHandlesFlag = sendPDU(clientSocket, EACH_HANDEL_SEND_FLAG, (uint8_t *)handelNamesBuffer, currHandelLen + sizeof(uint8_t));
		if (sendingHandlesFlag < 0)
		{
			perror("send call");
			exit(-1);
		}
	}
	int finishClientListFlag = sendPDU(clientSocket, LIST_END_FLAG, (uint8_t *)NumberOfHandels_NO_Buff, 0);	//sending flag 13 with only chat header
	if (finishClientListFlag < 0)
	{
		perror("send call");
		exit(-1);
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
	/**ProcessDirectMessage flag*/
	else if(flag == INDIVIDUAL_PACKET_FLAG && messageLen > 0){
		DirectMessage(dataBuffer, messageLen, clientSocket);
	}
	/**Process multicasting flag*/
	else if(flag == MULTI_CAST_FLAG && messageLen > 0){
		MultimodeMessage(dataBuffer, messageLen, clientSocket);
	}
	/**Process requesting list handel flag*/
	else if(flag == HANDLE_LIST_FLAG && messageLen > 0){	//probably won't return the MessageLen in this case
		ListRequest(clientSocket, dataBuffer);
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

