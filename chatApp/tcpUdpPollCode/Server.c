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
#include "pollLib.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);


/**
 * @brief
 * process the client's message that is sent to the server
 * @param clientSocket
*/
void processClient(int clientSocket){
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
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

	// recvFromClient(clientSocket);
	
	/* close the sockets */
	close(clientSocket);
	close(mainServerSocket);

	
	return 0;
}

void recvFromClient(int clientSocket)
{
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0)
	{
		printf("Message received on socket: %u, Message Length: %d, Data: %s\n", clientSocket, messageLen, dataBuffer);
	}
	else
	{
		printf("Connection closed by other side\n");
	}
}

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

