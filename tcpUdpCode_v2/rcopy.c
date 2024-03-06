// // Client side - UDP Code				    
// // By Hugh Smith	4/1/2017		

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
#include <stdbool.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "createPDU.h"
#include "cpe464.h"
#include "window.h"
#include "pollLib.h"

#define MAXBUF 1400

//all defined flags used for this problem
#define REGULAR_DATA_PACKET 3
#define RESEND_DATA_PACKET 4
#define RR_PACKET 5
#define SREJ_FLAG 6
#define TIMEOUT_RESENT_DATA_PACKET 7
#define FILE_NAME_PACKET 8
#define RESPONSE_FLAG_NAME 9
#define FILE_OPENING_FAIL 10
#define END_OF_FILE_FLAG 11
#define END_OF_FILE_FLAG_RESP 12

//data_packet_size_received
#define data_packet_size_received 11

typedef enum State STATE;

enum State{
	START_STATE, FILENAME, FILE_OK, END_OF_FILE, DONE,
};

struct setUpStruct{
	int socketNum;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	int portNumber;
	float errorRate;
	char * from_fileName;
	char * to_fileName;
	FILE * filePointer;
	uint8_t * setUpPacket;
	uint32_t clientSequenceNumber;
	uint32_t packetCounter;
	bool sendEOF;
};
struct setUpStruct setup;

// void talkToServer(int socketNum, struct sockaddr_in6 * server);
int readFromStdin(char * buffer);
int checkArgs(int argc, char * argv[]);

//state functions
STATE start_state(int * firstPacketSize);
STATE fileName(int firstPacketSize, char *argv[]);
STATE file_ok();
STATE endOfFile();
STATE done();
void processState(char *argv[]);
int createInitialPacket(int sequenceNumber, int flag);
uint32_t SREJ_RR_SEQUENCE(uint8_t * data, uint8_t * flag);


int main (int argc, char *argv[])
 {
	
	setup.portNumber = checkArgs(argc, argv);
	sendErr_init(setup.errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
	
	setup.socketNum = setupUdpClientToServer(&setup.server, argv[6], setup.portNumber);

	processState(argv);
	
	// talkToServer(socketNum, &server);
	
	// close(socketNum);

	return 0;
}

STATE done(){
	fclose(setup.filePointer);
	//clean up the client window api dynamic buffer
	freeWindowBuffer();

	//freeing other misc pointers used in the program
	free(setup.from_fileName);
	free(setup.to_fileName);
	free(setup.setUpPacket);

	exit(EXIT_SUCCESS);
}

void processState(char *argv[]){
	setup.clientSequenceNumber = 0;
	STATE state = START_STATE;
	int firstPacketSize = 0;


	while(1){
		switch(state){
			case START_STATE:
				state = start_state(&firstPacketSize);
			break;

			case FILENAME:
				state = fileName(firstPacketSize, argv);
			break;

			case FILE_OK:
				state = file_ok();
			break;

			case END_OF_FILE:
				state = endOfFile();
			break;

			case DONE:
				state = done();
			break;

			default:
				printf("some unhandel error happened\n");
			break;
		}
	}
}

FILE * getFile(char * fromFileName){
	FILE * filePointer = NULL;

	// Open the file in read mode
    filePointer = fopen(fromFileName, "rb");

	if(filePointer == NULL){
		printf("Specified file or filename not found");
		return NULL;
	}

	return filePointer;
}

uint32_t SREJ_RR_SEQUENCE(uint8_t * data, uint8_t * flag){

	uint32_t passedSequenceNumber;
	memcpy(&passedSequenceNumber, data + 7, sizeof(uint32_t));	//get the RR or SREJ sequence number from the payload after header

	//type of flag SREJ or RR
	memcpy(flag, data + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint32_t));

	return ntohl(passedSequenceNumber);
}

STATE endOfFile(){
	int serverAddrLen = sizeof(struct sockaddr_in6);
	uint8_t bufferData[11];	// 7 bytes for header and 4 bytes for the payload pdu
	setup.packetCounter = 0;	//initialize the packet counter to 0

	if(setup.sendEOF == false){	//if we didn't send the last packet, send an extra packet to notify the server
		while(1){
			//if window is open send immediately
			if(globalWindow.current != globalWindow.upper){
				//for this need to perform the window check see if it open and close and based of that send this last packet
				uint8_t updatedTempBuffer[1];	//to store one null char as part of the pdu
				updatedTempBuffer[0] = '\0';
				if(setup.setUpPacket != NULL){
					//if there exist a packet free it
					free(setup.setUpPacket);
				}
				uint16_t lastPacketSize = createPDU(&setup.setUpPacket, setup.clientSequenceNumber, END_OF_FILE_FLAG, updatedTempBuffer, sizeof(uint8_t));
				//send this packet
				
				safeSendto(setup.socketNum, setup.setUpPacket, lastPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);

				//add the packet to the buffer
				addPacket(setup.setUpPacket, lastPacketSize);	//7 bytes for the header size
				setup.clientSequenceNumber++;
				//set sendEOF to true since we send it aready and no need to send extra packet
				setup.sendEOF = true;
				break;
			}
			else{
				//if window is close
				while(globalWindow.current == globalWindow.upper){
					//if 10 tries fail return to done state
					if(setup.packetCounter == 10){
						return DONE;
					}
					//poll for 1s
					int socketVal = pollCall(1000);
					if(socketVal > -1){
						int dataLen = safeRecvfrom(socketVal, bufferData, 11, 0, (struct sockaddr *) &setup.server, &serverAddrLen);
						//reset the counter to  0 if we receive anything from the server
						setup.packetCounter = 0;

						//perfrom the checksum on the receive data
						setup.packetCounter = 0;

						uint16_t checkSum = in_cksum((uint16_t*)bufferData, dataLen);
						if(checkSum == 0){
							uint8_t flagReceived;
							memcpy(&flagReceived, bufferData + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));
							//if the last flag received end
							if(flagReceived == END_OF_FILE_FLAG_RESP){
								return DONE;
							}

							uint8_t SREJ_RR_flag;
							uint32_t received_RR_SREJ_sequenceNumber = SREJ_RR_SEQUENCE(bufferData, &SREJ_RR_flag);

							if(SREJ_RR_flag == RR_PACKET){
								//update the lower and upper window based of the received RR packet
								updateWindow(received_RR_SREJ_sequenceNumber);
								printf("received data packet from the server %d\n",dataLen);
							}
							else if(SREJ_RR_flag == SREJ_FLAG){
								uint16_t pduPacketSize;
								memcpy(&pduPacketSize, getPacket(received_RR_SREJ_sequenceNumber), sizeof(uint16_t));
								safeSendto(setup.socketNum, getPacket(received_RR_SREJ_sequenceNumber) + sizeof(uint16_t), pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);
							}
						}
					}
					else{	//pollCall 1 second time out, need to resend the lowest packet

						//get the lowest packet from the buffer and send it to server again
						uint16_t pduPacketSize;
						memcpy(&pduPacketSize, getPacket(globalWindow.lower), sizeof(uint16_t));
						safeSendto(setup.socketNum, getPacket(globalWindow.lower) + sizeof(uint16_t), pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);

						//increament the sequence number
						// setup.clientSequenceNumber++;

						//incremeant the packet-counter
						setup.packetCounter++;
					}
				}
			}
		}
	}
	//the last packet have been sent, waiting for the ack back
	setup.packetCounter = 0;
	while(1){
		//if 10 tries fail return to done state
		if(setup.packetCounter == 10){
			return DONE;
		}
		//poll for 1s
		int socketVal = pollCall(1000);
		if(socketVal > -1){
			int dataLen = safeRecvfrom(socketVal, bufferData, 11, 0, (struct sockaddr *) &setup.server, &serverAddrLen);
			//reset the counter to  0 if we receive anything from the server
			setup.packetCounter = 0;

			//perform the checksum
			uint16_t checkSum = in_cksum((uint16_t*)bufferData, dataLen);
			if(checkSum == 0){
				uint8_t flagReceived;
				memcpy(&flagReceived, bufferData + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));
				//if the last flag received end
				if(flagReceived == END_OF_FILE_FLAG_RESP){
					return DONE;
				}

				uint8_t SREJ_RR_flag;
				uint32_t received_RR_SREJ_sequenceNumber = SREJ_RR_SEQUENCE(bufferData, &SREJ_RR_flag);

				if(SREJ_RR_flag == RR_PACKET){
					//update the lower and upper window based of the received RR packet
					updateWindow(received_RR_SREJ_sequenceNumber);
					printf("received data packet from the server %d\n",dataLen);
				}
				else if(SREJ_RR_flag == SREJ_FLAG){
					uint16_t pduPacketSize;
					memcpy(&pduPacketSize, getPacket(received_RR_SREJ_sequenceNumber), sizeof(uint16_t));
					safeSendto(setup.socketNum, getPacket(received_RR_SREJ_sequenceNumber) + sizeof(uint16_t), pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);
				}
			}
		}
		else{	//pollCall 1 second time out, need to resend the lowest packet

			//get the lowest packet from the buffer and send it to server again
			uint16_t pduPacketSize;
			memcpy(&pduPacketSize, getPacket(globalWindow.lower), sizeof(uint16_t));
			safeSendto(setup.socketNum, getPacket(globalWindow.lower) + sizeof(uint16_t), pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);

			//increament the sequence number
			setup.clientSequenceNumber++;

			//incremeant the packet-counter
			setup.packetCounter++;
		}
	}

	return DONE;
}

STATE file_ok(){
	//initialize my window api
	init(setup.clientSequenceNumber);

	uint8_t tempBuffer[globalWindow.packetSize];
	uint16_t bytesRead;
	setup.sendEOF = false;
	int serverAddrLen = sizeof(struct sockaddr_in6);
	uint8_t bufferData[11];	// 7 bytes for header and 4 bytes for the payload pdu

	//initialize the packet counter to 0
	setup.packetCounter = 0;

	while (1) {
		//while window is open
		while(globalWindow.current != globalWindow.upper){
			printf("current: %d\n",globalWindow.current);
			printf("upper: %d\n",globalWindow.upper);
			printf("lower: %d\n",globalWindow.lower);

			//read from the file
			bytesRead = fread(tempBuffer, 1, globalWindow.packetSize, setup.filePointer);
			//0 bytes read that means we have send the EOF packet already
			if(bytesRead == 0){
				return END_OF_FILE;
			}
			//create and send the packet, add it to buffer
				if(setup.setUpPacket != NULL){
					//if there exist a packet free it
					free(setup.setUpPacket);
				}
				//not the last packet
				if(bytesRead == globalWindow.packetSize){
					uint16_t pduPacketSize = createPDU(&setup.setUpPacket, setup.clientSequenceNumber, REGULAR_DATA_PACKET, tempBuffer, globalWindow.packetSize);
					//send this packet
					safeSendto(setup.socketNum, setup.setUpPacket, pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);

					//add the packet to the buffer
					addPacket(setup.setUpPacket, 7 + globalWindow.packetSize);	//7 bytes for the header size
					setup.clientSequenceNumber++;
					//update the current pointer
					globalWindow.current++;
				}
				//last packet with less than expected val
				else if(bytesRead < globalWindow.packetSize){
					uint16_t pduPacketSize = createPDU(&setup.setUpPacket, setup.clientSequenceNumber, END_OF_FILE_FLAG, tempBuffer, bytesRead);
					//send this packet
					safeSendto(setup.socketNum, setup.setUpPacket, pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);

					//add the packet to the buffer
					addPacket(setup.setUpPacket, 7 + bytesRead);	//7 bytes for the header size
					setup.clientSequenceNumber++;
					//set sendEOF to true since we send it aready and no need to send extra packet
					setup.sendEOF = true;
					//update the current pointer
					globalWindow.current++;

					//Enter the EOF state
					return END_OF_FILE;
				}

			//receive the RR's or SREJ
			int socketVal = pollCall(0);
			if(socketVal > -1){
				int dataLen = safeRecvfrom(socketVal, bufferData, 11, 0, (struct sockaddr *) &setup.server, &serverAddrLen);
				//reset the counter to  0 if we receive anything from the server
				setup.packetCounter = 0;

				uint16_t checkSum = in_cksum((uint16_t*)bufferData, dataLen);
				if(checkSum == 0){
					uint8_t flagReceived;
					memcpy(&flagReceived, bufferData + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));
					//if the last flag received end
					if(flagReceived == END_OF_FILE_FLAG_RESP){
						return DONE;
					}
					
					uint8_t SREJ_RR_flag;
					uint32_t received_RR_SREJ_sequenceNumber = SREJ_RR_SEQUENCE(bufferData, &SREJ_RR_flag);

					if(SREJ_RR_flag == RR_PACKET){
						//update the lower and upper window based of the received RR packet
						updateWindow(received_RR_SREJ_sequenceNumber);
						printf("received data packet from the server %d\n",dataLen);
					}
					else if(SREJ_RR_flag == SREJ_FLAG){
						uint16_t pduPacketSize;
						memcpy(&pduPacketSize, getPacket(received_RR_SREJ_sequenceNumber), sizeof(uint16_t));
						safeSendto(setup.socketNum, getPacket(received_RR_SREJ_sequenceNumber) + sizeof(uint16_t), pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);
					}
				}
			}
		}

		//if window is closed
		while(globalWindow.current == globalWindow.upper){
			//if 10 tries fail return to done state
			if(setup.packetCounter == 10){
				return DONE;
			}
			//poll for 1s
			int socketVal = pollCall(1000);
			if(socketVal > -1){
				int dataLen = safeRecvfrom(socketVal, bufferData, 11, 0, (struct sockaddr *) &setup.server, &serverAddrLen);

				//reset the counter to  0 if we receive anything from the server
				setup.packetCounter = 0;

				uint16_t checkSum = in_cksum((uint16_t*)bufferData, dataLen);
				if(checkSum == 0){
					uint8_t flagReceived;
					memcpy(&flagReceived, bufferData + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));
					//if the last flag received end
					if(flagReceived == END_OF_FILE_FLAG_RESP){
						return DONE;
					}

					uint8_t SREJ_RR_flag;
					uint32_t received_RR_SREJ_sequenceNumber = SREJ_RR_SEQUENCE(bufferData, &SREJ_RR_flag);

					if(SREJ_RR_flag == RR_PACKET){
						//update the lower and upper window based of the received RR packet
						updateWindow(received_RR_SREJ_sequenceNumber);
						printf("received data packet from the server %d\n",dataLen);
					}
					else if(SREJ_RR_flag == SREJ_FLAG){
						uint16_t pduPacketSize;
						memcpy(&pduPacketSize, getPacket(received_RR_SREJ_sequenceNumber), sizeof(uint16_t));
						safeSendto(setup.socketNum, getPacket(received_RR_SREJ_sequenceNumber) + sizeof(uint16_t), pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);
					}
				}
			}
			else{	//pollCall 1 second time out, need to resend the lowest packet

				//get the lowest packet from the buffer and send it to server again
				uint16_t pduPacketSize;
				memcpy(&pduPacketSize, getPacket(globalWindow.lower), sizeof(uint16_t));
				safeSendto(setup.socketNum, getPacket(globalWindow.lower) + sizeof(uint16_t), pduPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);

				//increament the sequence number
				// setup.clientSequenceNumber++;

				//incremeant the packet-counter
				setup.packetCounter++;
			}
		}

		//reset the counter to 0
		setup.packetCounter = 0;
    }

	return END_OF_FILE;
}

STATE fileName(int firstPacketSize, char *argv[]){
	//initializing the packet counter to 0 (keeps track of how many times to send the packet before quitting)
	setup.packetCounter = 0;
	uint8_t * buffer = (uint8_t *)malloc(MAXBUF);

	while(1){
		if(setup.packetCounter == 10){	//if packetCounter reaches 10 end
			fclose(setup.filePointer);
			return DONE;
		}
		//send first packet and waiting for the ack before data transfer
		int serverAddrLen = sizeof(struct sockaddr_in6);
		safeSendto(setup.socketNum, setup.setUpPacket, firstPacketSize, 0, (struct sockaddr *) &setup.server, serverAddrLen);

		//after sending the setup packet increament the sequnce numbe each time
		setup.clientSequenceNumber++;

		//poll for 1 second and if the pollcall fail f the original socket and send a new one
		int socketVal = -1;
		socketVal = pollCall(1000);

		if(socketVal > -1){
			int dataLen = safeRecvfrom(socketVal, buffer, MAXBUF, 0, (struct sockaddr *) &setup.server, &serverAddrLen);
			//reset the counter to  0 if we receive anything from the server
			setup.packetCounter = 0;

			uint8_t tempCheckBuffer[dataLen];

			//copy the received amount of data from the server
			memcpy(tempCheckBuffer, buffer, dataLen);

			//perform the checksum on the received data
			uint16_t checkSum = in_cksum((uint16_t*)tempCheckBuffer, dataLen);
    		checkSum == 0 ? printf("checksum verification successful\n") : printf("checksum verification failed.\n");
			//get the flag to check if it is okay flag or fileOpening flag
			uint8_t flag;
			memcpy(&flag,tempCheckBuffer + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));

			if(checkSum == 0 && flag == RESPONSE_FLAG_NAME){
				setup.packetCounter = 0;
				break;
			}
			else{
				setup.packetCounter++;
				//create new packet with the updated sequence number
				firstPacketSize = createInitialPacket(setup.clientSequenceNumber, FILE_NAME_PACKET);

				if(flag == FILE_OPENING_FAIL){
					printf("\n\nServer failed to open the file on their end.\n\n");
					fflush(stdout);
				}
			}
		}
		else{
			//update the packet counter
			setup.packetCounter++;
			//poll call failed we need close the socket, remove from the poll set
			removeFromPollSet(setup.socketNum);
			close(setup.socketNum);
			
			//create new socket and add it to the pollset
			setup.socketNum = setupUdpClientToServer(&setup.server, argv[6], setup.portNumber);
			addToPollSet(setup.socketNum);

			//create new packet with the updated sequence number
			firstPacketSize = createInitialPacket(setup.clientSequenceNumber, FILE_NAME_PACKET);
		}
	}

	setup.packetCounter = 0;
	return FILE_OK;
}

int createInitialPacket(int sequenceNumber, int flag){
	if(setup.setUpPacket != NULL){	//if there is already setUp packet created remove it
		free(setup.setUpPacket);
	}
	uint32_t payloadLength = sizeof(uint32_t) + sizeof(uint16_t) + strlen(setup.to_fileName);
	uint8_t * tempPDUPacket = (uint8_t *)malloc(payloadLength);
	//add initial packet information in the first packet
	memcpy(tempPDUPacket, &globalWindow.windowSize, sizeof(uint32_t));
	memcpy(tempPDUPacket + sizeof(uint32_t), &globalWindow.packetSize, sizeof(uint16_t));
	memcpy(tempPDUPacket + sizeof(uint32_t) + sizeof(uint16_t), (uint8_t *)setup.to_fileName, strlen(setup.to_fileName));

	//setting up the initial packet to send it to the server (return packet size should be payloadLength + 7 bits)
	uint16_t firstPacketSize =  createPDU(&setup.setUpPacket, sequenceNumber, flag, tempPDUPacket, payloadLength);
	free(tempPDUPacket);	//free temp PDU packet

	return firstPacketSize;
}

/**
 * @brief
 * setting up the connection with the server and sending the intial packet information
*/
STATE start_state(int * firstPacketSize){

	//verify first that the file name does exist
	setup.filePointer = getFile(setup.from_fileName);
	//file failed to open or not found
	if(setup.filePointer == NULL){
		return DONE;
	}

	*firstPacketSize = createInitialPacket(setup.clientSequenceNumber, FILE_NAME_PACKET);

	if(*firstPacketSize <= 7){
		printf("The first packet size is not correct\n");
		return DONE;
	}

	//running while loop and setting up the initial connection with polling for one sec
	//setting up the pollset
	setupPollSet();

	//adding the server socket to the pollset
	addToPollSet(setup.socketNum);

	return FILENAME;	//returns the file name state after setting everything up
}

int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
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

int checkArgs(int argc, char * argv[])
{
	int portNumber = 0;
	
	/* check command line arguments  */
	if (argc != 8)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}

	if(strlen(argv[1]) <= 100 && strlen(argv[2]) <= 100){
		setup.from_fileName = malloc(strlen(argv[1]) + 1);
		setup.to_fileName = malloc(strlen(argv[2]) + 1);
	}
	else{
		printf("The file name length is greater than 100 chars\n");
		exit(EXIT_FAILURE);
	}

	//set the last char to null
	setup.from_fileName[strlen(argv[1])] = '\0';
	setup.to_fileName[strlen(argv[2])] = '\0';

	//get the file name from the user
    strcpy(setup.from_fileName, argv[1]);
    strcpy(setup.to_fileName, argv[2]);

	//get port number and error Rate from the user
	portNumber = atoi(argv[7]);
	setup.errorRate = atof(argv[5]);

	//getting the window size and buffer size
	globalWindow.windowSize = atoi(argv[3]);

	if(globalWindow.windowSize > 1073741824){
		printf("\nWindow size specified is too big!!\n");
		exit(EXIT_FAILURE);
	}
	globalWindow.packetSize = atoi(argv[4]);
	if(globalWindow.packetSize > 1400){
		printf("\nBuffer size specified is too big!!\n");
		exit(EXIT_FAILURE);
	}
		
	return portNumber;
}