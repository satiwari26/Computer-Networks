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

void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);

/**
 * @brief
 * process the stdin, get the data from the stdin and send it to the server
 * @param
 * takes in the server socketNum
*/
void processStdin(int socketNum){
	uint8_t sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	
	sendLen = readFromStdin(sendBuf);
	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
	sent =  sendPDU(socketNum, sendBuf, sendLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("Amount of data sent is: %d\n", sent);
}

/**
 * @brief processes the messgage from the server.
 * Also close the connection if the server has ended the program
 * @param
 * takes in the serverSocket to receive the message or change the message
*/
void processMsgFromServer(int serverSocket){
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	//now get the data from the client_socket
	if ((messageLen = recvPDU(serverSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0)
	{
		printf("Message received on socket: %u, Message Length: %d, Data: %s\n", serverSocket, messageLen, dataBuffer);
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
void clientControl(int socketNum){
	int pollVal = pollCall(-1);
	printf("Enter data: ");
	fflush(stdout);
	if(pollVal == STDIN_FILENO){
		processStdin(socketNum);
	}
	else if(pollVal > 0){
		processMsgFromServer(pollVal);
	}
}

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	
	checkArgs(argc, argv);

	//setup the polling for the client
	setupPollSet();

	//add the stdin to the poll-set
	addToPollSet(STDIN_FILENO);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);
	addToPollSet(socketNum);

	while(1){
		clientControl(socketNum);
	}
	
	return 0;
}

void sendToServer(int socketNum)
{
	uint8_t sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	
	sendLen = readFromStdin(sendBuf);
	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
	sent =  sendPDU(socketNum, sendBuf, sendLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("Amount of data sent is: %d\n", sent);
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
	if (argc != 3)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
}
