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

void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);

/**
 * @brief
 * process the stdin, get the data from the stdin and send it to the server
 * @param
 * takes in the server socketNum
*/
// void processStdin(int socketNum){
// 	uint8_t sendBuf[MAXBUF];   //data buffer
// 	int sendLen = 0;        //amount of data to send
// 	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	
// 	sendLen = readFromStdin(sendBuf);
// 	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
// 	sent =  sendPDU(socketNum, sendBuf, sendLen);
// 	if (sent < 0)
// 	{
// 		perror("send call");
// 		exit(-1);
// 	}

// 	printf("Amount of data sent is: %d\n", sent);
// }

/**
 * @brief processes the messgage from the server.
 * Also close the connection if the server has ended the program
 * @param
 * takes in the serverSocket to receive the message or change the message
*/
// void processMsgFromServer(int serverSocket){
// 	uint8_t dataBuffer[MAXBUF];
// 	int messageLen = 0;
// 	//now get the data from the client_socket
// 	if ((messageLen = recvPDU(serverSocket, dataBuffer, MAXBUF)) < 0)
// 	{
// 		perror("recv call");
// 		exit(-1);
// 	}

// 	if (messageLen > 0)
// 	{
// 		printf("Message received on socket: %u, Message Length: %d, Data: %s\n", serverSocket, messageLen, dataBuffer);
// 	}
// 	else
// 	{
// 		printf("Connection closed by the server\n");
// 		//close the socket connection b/w client and server
// 		close(serverSocket);
// 		//remove the client from the socket-set
// 		removeFromPollSet(serverSocket);
// 		exit(0);
// 	}
// }

/**
 * @brief
 * method handles the stdin and process message from the server
 * @param socketNum
 * param carries the server socketnumber to perfrom that operation
*/
// void clientControl(int socketNum){
// 	int pollVal = pollCall(-1);
// 	if(pollVal == STDIN_FILENO){
// 		processStdin(socketNum);
// 	}
// 	else if(pollVal > 0){
// 		processMsgFromServer(pollVal);
// 	}
// }

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

	//add the stdin to the poll-set
	// addToPollSet(STDIN_FILENO);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	//log in connection with the server
	uint8_t ServerAckFlag = setUpConnection(socketNum, argv);

	// while(1){
	// 	fflush(stdout);
	// 	printf("Enter data: ");
	// 	fflush(stdout);
	// 	clientControl(socketNum);
	// }
	
	return 0;
}

// void sendToServer(int socketNum)
// {
// 	uint8_t sendBuf[MAXBUF];   //data buffer
// 	int sendLen = 0;        //amount of data to send
// 	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	
// 	sendLen = readFromStdin(sendBuf);
// 	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
// 	sent =  sendPDU(socketNum, sendBuf, sendLen);
// 	if (sent < 0)
// 	{
// 		perror("send call");
// 		exit(-1);
// 	}

// 	printf("Amount of data sent is: %d\n", sent);
// }

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
	}
}
