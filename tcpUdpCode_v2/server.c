/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "createPDU.h"
#include "cpe464.h"
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

struct setUpPacketInfo{
	int socketNum;				
	int portNumber;
	float errorRate;
	uint32_t serverSequenceNumber;
	uint8_t * setUpPacket;
	uint8_t * receivedSetUpPacket;
	int firstPacketdataLen;
	FILE * toFilePointer;
	struct sockaddr_in6 client;
	int clientAddrLen;
};

typedef enum State STATE;

enum State{
	START_STATE, INORDER, BUFFER, FLUSHING, DONE,
};

struct setUpPacketInfo setup;

//state functions
void processState(char *argv[]);
int createInitialPacket(int sequenceNumber, int flag);
STATE start_state(int * firstPacketSize);
STATE inorder();
STATE buffer();
STATE flushing();
STATE done();

void processClient(int socketNum, char *argv[]);
int checkArgs(int argc, char *argv[]);
FILE * getFile(char * fromFileName, int fileNameSize);

//create RR or SREJ packets method
int create_RR(uint32_t RR_sequenceNumber, int flag);
int create_SREJ(uint32_t SREJ_sequenceNumber);

int main ( int argc, char *argv[]  )
{ 
	setup.socketNum = 0;				
	setup.portNumber = 0;
	setup.errorRate = 0;

	setup.portNumber = checkArgs(argc, argv);
	sendErr_init(setup.errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
	
	setup.socketNum = udpServerSetup(setup.portNumber);

	processClient(setup.socketNum, argv);

	// close(setup.socketNum);
	
	return 0;
}

FILE * getFile(char * toFileName, int fileNameSize){
	FILE * filePointer = NULL;

	printf("fileName: %s\n",toFileName);

	// Open the file in read mode
    filePointer = fopen(toFileName, "a");

	if(filePointer == NULL){
		printf("Specified file or filename not found");
		return NULL;
	}

	return filePointer;
}

int createInitialPacket(int sequenceNumber, int flag){
	if(setup.setUpPacket != NULL){	//if there is already setUp packet created remove it
		free(setup.setUpPacket);
	}
	uint32_t payloadLength = sizeof(uint8_t);
	uint8_t * tempPDUPacket = (uint8_t *)malloc(payloadLength);
	//add initial packet information in the first packet
	tempPDUPacket[0] = '\0';
	//setting up the initial packet to send it to the server (return packet size should be payloadLength + 7 bits)
	int firstPacketSize =  createPDU(&setup.setUpPacket, sequenceNumber, flag, tempPDUPacket, payloadLength);
	free(tempPDUPacket);	//free temp PDU packet

	return firstPacketSize;
}

int create_RR(uint32_t RR_sequenceNumber, int flag){

	if(setup.setUpPacket != NULL){	//if there is already setUp packet created remove it
		free(setup.setUpPacket);
	}

	uint8_t RR_sequenceNumberData[sizeof(uint32_t)];
	uint32_t RR_sequenceNumber_NO = htonl(RR_sequenceNumber);

	memcpy(RR_sequenceNumberData, &RR_sequenceNumber_NO, sizeof(uint32_t));

	int RR_pdu_size = createPDU(&setup.setUpPacket, setup.serverSequenceNumber, flag, RR_sequenceNumberData, sizeof(uint32_t));

	return RR_pdu_size;
}

int create_SREJ(uint32_t SREJ_sequenceNumber){
	if(setup.setUpPacket != NULL){	//if there is already setUp packet created remove it
		free(setup.setUpPacket);
	}
	uint8_t SREJ_sequenceNumberData[sizeof(uint32_t)];
	uint32_t SREJ_sequenceNumber_NO = htonl(SREJ_sequenceNumber);

	memcpy(SREJ_sequenceNumberData, &SREJ_sequenceNumber_NO, sizeof(uint32_t));

	//setting up the initial packet to send it to the server (return packet size should be payloadLength + 7 bits)
	int firstPacketSize =  createPDU(&setup.setUpPacket, SREJ_sequenceNumber, SREJ_FLAG, SREJ_sequenceNumberData, sizeof(uint32_t));

	return firstPacketSize;
}

int createEOF_resp(int flag){
	if(setup.setUpPacket != NULL){	//if there is already setUp packet created remove it
		free(setup.setUpPacket);
	}

	uint8_t data = '\0';

	int createEOF_size = createPDU(&setup.setUpPacket, setup.serverSequenceNumber, flag, &data, sizeof(uint8_t));

	return createEOF_size;
}

STATE done(){
	fclose(setup.toFilePointer);
	//remove the allocated buffer from the space
	cleanServerUP();

	//clean other misc dynamically allocated memory
	if(setup.setUpPacket != NULL){
		free(setup.setUpPacket);
	}
	if(setup.receivedSetUpPacket != NULL){
		free(setup.receivedSetUpPacket);
	}

	exit(EXIT_SUCCESS);
}

STATE flushing(){
	//check for the validation in the buffer
	while(1){
		uint32_t indexVal = globalServerBuffer.expected % globalServerBuffer.serverWindowSize;
		uint32_t currSequenceNumber_HO;
		uint32_t currSequenceNumber_NO;
		printf("expected: %d\n",globalServerBuffer.expected);
		printf("highest: %d\n",globalServerBuffer.highest);
		//extracting the sequence number from the expected index val
		memcpy(&currSequenceNumber_NO, globalServerBuffer.ServerBuffer[indexVal] + sizeof(uint16_t), sizeof(uint32_t));
		currSequenceNumber_HO = ntohl(currSequenceNumber_NO);
		printf("validation, index: %d, %d\n",globalServerBuffer.validationBuffer[indexVal], indexVal);
		//when exptected reached the highest
		if(globalServerBuffer.expected < globalServerBuffer.highest && globalServerBuffer.validationBuffer[indexVal] == invalid){
			//create and send the SREJ
			int SREJ_packet_size = create_SREJ(globalServerBuffer.expected);
			//send the SREJ
			safeSendto(setup.socketNum, setup.setUpPacket, SREJ_packet_size, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);

			//return to the Buffer state
			return BUFFER;
		}
		else if(globalServerBuffer.expected == globalServerBuffer.highest){
			//extracting out the data flag
			uint8_t dataPacketFlag;
			memcpy(&dataPacketFlag, globalServerBuffer.ServerBuffer[indexVal] + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));

			//if received regular data packet, send RR
			if(dataPacketFlag == REGULAR_DATA_PACKET){
				uint8_t payloadData[globalServerBuffer.serverBufferSize];
				//write the packet to opened file
				memcpy(payloadData, globalServerBuffer.ServerBuffer[indexVal] + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), globalServerBuffer.serverBufferSize);
				size_t bytesWritten = fwrite(payloadData, 1, globalServerBuffer.serverBufferSize, setup.toFilePointer);
				if(bytesWritten != globalServerBuffer.serverBufferSize){
					printf("Error writing data in the disk.\n");
				}
				fflush(setup.toFilePointer);

				//flush that buffer location
				flushPacket(indexVal);

				//increament the expected
				globalServerBuffer.expected++;

				//create the RR extected and send RR expected
				int RR_PDU_SIZE = create_RR(globalServerBuffer.expected, RR_PACKET);
				safeSendto(setup.socketNum, setup.setUpPacket, RR_PDU_SIZE, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
				setup.serverSequenceNumber++;
				
				return INORDER;
			}
			else if(dataPacketFlag == END_OF_FILE_FLAG){

				uint8_t packetSizeBuffered;
				memcpy(&packetSizeBuffered, globalServerBuffer.ServerBuffer[indexVal], sizeof(uint16_t));

				if(packetSizeBuffered > 8){	//case when we read less bytes from the file than the standard require
					uint16_t EOF_payload_len = packetSizeBuffered - 7;
					uint8_t payloadData[EOF_payload_len];

					//write the packet to opened file
					memcpy(payloadData, globalServerBuffer.ServerBuffer[indexVal] + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), EOF_payload_len);
					size_t bytesWritten = fwrite(payloadData, 1, EOF_payload_len, setup.toFilePointer);
					if(bytesWritten != EOF_payload_len){
						printf("Error writing data in the disk.\n");
					}
					fflush(setup.toFilePointer);

					//flush that buffer location
					flushPacket(indexVal);

					//create and send the rr packet with END of FILE flag
					int EOF_PDU_SIZE = createEOF_resp(END_OF_FILE_FLAG_RESP);
					safeSendto(setup.socketNum, setup.setUpPacket, EOF_PDU_SIZE, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
					setup.serverSequenceNumber++;

						//print the receive packet in the data
				}
				else{
					//don't write it to the file

					//create and send the rr packet with END of FILE flag
					int EOF_PDU_SIZE = createEOF_resp(END_OF_FILE_FLAG_RESP);
					safeSendto(setup.socketNum, setup.setUpPacket, EOF_PDU_SIZE, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
					setup.serverSequenceNumber++;
				}

				//increament the expected
				globalServerBuffer.expected++;

				//after sending the eof flag we can just quit and end the program
				return DONE;
			}
		}
		//if the sequence number is the expected sequence number and is valid
		else if(currSequenceNumber_HO == globalServerBuffer.expected && globalServerBuffer.validationBuffer[indexVal] == valid){
			//extracting out the data flag
			uint8_t dataPacketFlag;
			memcpy(&dataPacketFlag, globalServerBuffer.ServerBuffer[indexVal] + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));

			//if received regular data packet, send RR
			if(dataPacketFlag == REGULAR_DATA_PACKET){
				uint8_t payloadData[globalServerBuffer.serverBufferSize];
				//write the packet to opened file
				memcpy(payloadData, globalServerBuffer.ServerBuffer[indexVal] + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), globalServerBuffer.serverBufferSize);
				size_t bytesWritten = fwrite(payloadData, 1, globalServerBuffer.serverBufferSize, setup.toFilePointer);
				if(bytesWritten != globalServerBuffer.serverBufferSize){
					printf("Error writing data in the disk.\n");
				}
				fflush(setup.toFilePointer);

				//flush that buffer location
				flushPacket(indexVal);

					//print the receive packet in the data
			}
			else if(dataPacketFlag == END_OF_FILE_FLAG){

				uint8_t packetSizeBuffered;
				memcpy(&packetSizeBuffered, globalServerBuffer.ServerBuffer[indexVal], sizeof(uint16_t));

				if(packetSizeBuffered > 8){	//case when we read less bytes from the file than the standard require
					uint16_t EOF_payload_len = packetSizeBuffered - 7;
					uint8_t payloadData[EOF_payload_len];

					//write the packet to opened file
					memcpy(payloadData, globalServerBuffer.ServerBuffer[indexVal] + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), EOF_payload_len);
					size_t bytesWritten = fwrite(payloadData, 1, EOF_payload_len, setup.toFilePointer);
					if(bytesWritten != EOF_payload_len){
						printf("Error writing data in the disk.\n");
					}
					fflush(setup.toFilePointer);

					//flush that buffer location
					flushPacket(indexVal);
				}
			}
			//increament the expected
			globalServerBuffer.expected++;
		}
	}
}


STATE buffer(){
	//the expected dataPacketSize is buffer size + header size
	uint16_t dataPacketExpectedSize = globalServerBuffer.serverBufferSize + 7;

	while(1){
		int polVal = pollCall(10000);
		if(polVal > -1){
			uint16_t dataPacketReceivedSize = safeRecvfrom(setup.socketNum, setup.receivedSetUpPacket, dataPacketExpectedSize, 0, (struct sockaddr *) &setup.client, &setup.clientAddrLen);
			//perform the checksum on the received data
			uint16_t checkSum = in_cksum((uint16_t*)setup.receivedSetUpPacket, dataPacketReceivedSize);
			
			if(checkSum == 0){	//if checksum is successful
				uint32_t tempReceive_HO = 0;
				memcpy(&tempReceive_HO, setup.receivedSetUpPacket, sizeof(uint32_t));

				//convert the receive network sequence number to host order
				globalServerBuffer.receive = ntohl(tempReceive_HO);

				//perform the checking between the highest and received and if there are bits missing set them to invalid
				invalidationCheck();

				//if received is greater than the expected
				if(globalServerBuffer.receive > globalServerBuffer.expected){
					//buffer the receive packet
					addServerPacket(setup.receivedSetUpPacket, dataPacketReceivedSize);
					//setting the highest to what we receive
					globalServerBuffer.highest = globalServerBuffer.receive;
				}
				//if we receive the expected packet
				else if(globalServerBuffer.receive == globalServerBuffer.expected){
					//write the packet to disk
					//extracting out the data flag
					uint8_t dataPacketFlag;
					memcpy(&dataPacketFlag, setup.receivedSetUpPacket + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));

					//if received regular data packet, send RR
					if(dataPacketFlag == REGULAR_DATA_PACKET){
						uint8_t payloadData[globalServerBuffer.serverBufferSize];
						//write the packet to opened file
						memcpy(payloadData, setup.receivedSetUpPacket + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), globalServerBuffer.serverBufferSize);
						size_t bytesWritten = fwrite(payloadData, 1, globalServerBuffer.serverBufferSize, setup.toFilePointer);
						if(bytesWritten != globalServerBuffer.serverBufferSize){
							printf("Error writing data in the disk.\n");
						}
						fflush(setup.toFilePointer);
					}
					//the packet can't be eof packet because after that it shouldn't receive any packets

					//increament the expected
					globalServerBuffer.expected++;
					return FLUSHING;
				}
				else if(globalServerBuffer.receive < globalServerBuffer.expected){	//if SREJ didn't make it and we receive lower than expected, resend SREJ
					//create and send the SREJ
					int SREJ_packet_size = create_SREJ(globalServerBuffer.expected);
					//send the SREJ
					safeSendto(setup.socketNum, setup.setUpPacket, SREJ_packet_size, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
					setup.serverSequenceNumber++;
				}
			}
		}
		//failed to receive anything after 10s
		else{
			return DONE;
		}
	}

	return DONE;
}

STATE inorder(){
	if(setup.receivedSetUpPacket != NULL){
		free(setup.receivedSetUpPacket);
	}
	//the expected dataPacketSize is buffer size + header size
	uint16_t dataPacketExpectedSize = globalServerBuffer.serverBufferSize + 7;
	//buffer to store the data packets
	setup.receivedSetUpPacket = (uint8_t *)malloc(dataPacketExpectedSize);
	uint8_t payloadData[globalServerBuffer.serverBufferSize];
	while(1){
		pollCall(10000);	//poll for 10 seconds and start 
		//the receive size for each packet has standard of user entered server Buffer size
		uint16_t dataPacketReceivedSize = safeRecvfrom(setup.socketNum, setup.receivedSetUpPacket, dataPacketExpectedSize, 0, (struct sockaddr *) &setup.client, &setup.clientAddrLen);

		//perform the checksum on the received data
		uint16_t checkSum = in_cksum((uint16_t*)setup.receivedSetUpPacket, dataPacketReceivedSize);
		uint32_t tempReceive_HO = 0;
		memcpy(&tempReceive_HO, setup.receivedSetUpPacket, sizeof(uint32_t));

		//convert the receive network sequence number to host order
		globalServerBuffer.receive = ntohl(tempReceive_HO);

		if(checkSum == 0){	//if checksum pass then perform other action 
			printf("Checksum passed!\n\n");
		
			if(globalServerBuffer.receive == globalServerBuffer.expected){
				//extracting out the data flag
				uint8_t dataPacketFlag;
				memcpy(&dataPacketFlag, setup.receivedSetUpPacket + sizeof(uint32_t) + sizeof(uint16_t), sizeof(uint8_t));

				//if received regular data packet, send RR
				if(dataPacketFlag == REGULAR_DATA_PACKET){

					//write the packet to opened file
					memcpy(payloadData, setup.receivedSetUpPacket + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), globalServerBuffer.serverBufferSize);
					size_t bytesWritten = fwrite(payloadData, 1, globalServerBuffer.serverBufferSize, setup.toFilePointer);
					if(bytesWritten != globalServerBuffer.serverBufferSize){
						printf("Error writing data in the disk.\n");
					}
					fflush(setup.toFilePointer);
					//update the highest to the expected
					globalServerBuffer.highest = globalServerBuffer.expected;
					globalServerBuffer.expected++;

					//create and send the rr packet
					int RR_PDU_SIZE = create_RR(globalServerBuffer.expected, RR_PACKET);
					safeSendto(setup.socketNum, setup.setUpPacket, RR_PDU_SIZE, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
					setup.serverSequenceNumber++;
				}
				//if received end of file packet
				else if(dataPacketFlag == END_OF_FILE_FLAG){
					if(dataPacketReceivedSize > 8){	//case when we read less bytes from the file than the standard require
						uint16_t EOF_payload_len = dataPacketReceivedSize - 7;

						//write the packet to opened file
						memcpy(payloadData, setup.receivedSetUpPacket + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), EOF_payload_len);
						size_t bytesWritten = fwrite(payloadData, 1, EOF_payload_len, setup.toFilePointer);
						if(bytesWritten != globalServerBuffer.serverBufferSize){
							printf("Error writing data in the disk.\n");
						}
						fflush(setup.toFilePointer);

						//update the highest to the expected
						globalServerBuffer.highest = globalServerBuffer.expected;
						globalServerBuffer.expected++;

						//create and send the rr packet with END of FILE flag
						int EOF_PDU_SIZE = createEOF_resp(END_OF_FILE_FLAG_RESP);
						safeSendto(setup.socketNum, setup.setUpPacket, EOF_PDU_SIZE, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
						setup.serverSequenceNumber++;

						return DONE;
					}
					else if(dataPacketReceivedSize == 8){	//case when a EOF needs to be send seperately
						//update the highest to the expected
						globalServerBuffer.highest = globalServerBuffer.expected;
						globalServerBuffer.expected++;

						//create and send the rr packet with END of FILE flag
						int EOF_PDU_SIZE = createEOF_resp(END_OF_FILE_FLAG_RESP);
						safeSendto(setup.socketNum, setup.setUpPacket, EOF_PDU_SIZE, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
						setup.serverSequenceNumber++;

						return DONE;
					}
				}
			}
			//if receive is lower than expected
			else if(globalServerBuffer.receive < globalServerBuffer.expected){
				//create and send the rr packet
				int RR_PDU_SIZE = create_RR(globalServerBuffer.expected, RR_PACKET);
				safeSendto(setup.socketNum, setup.setUpPacket, RR_PDU_SIZE, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
				setup.serverSequenceNumber++;
			}
			//if recv is greater than what we expected, need to handel that in the buffer
			else if(globalServerBuffer.receive > globalServerBuffer.expected){
				printf("Received the non-expected sequence number\n\n");

				//setting up the server buffer
				initServerbuffer();

				//create and send the SREJ
				int SREJ_packet_size = create_SREJ(globalServerBuffer.expected);
				//send the SREJ
				safeSendto(setup.socketNum, setup.setUpPacket, SREJ_packet_size, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);

				//buffer the receive packet
				addServerPacket(setup.receivedSetUpPacket, dataPacketReceivedSize);

				//setting the highest to what we receive
				globalServerBuffer.highest = globalServerBuffer.receive;

				return BUFFER;
			}
		}
	}

	return DONE;
}

STATE start_state(int * firstPacketSize){

	uint8_t tempDataPacket[setup.firstPacketdataLen];

	memcpy(tempDataPacket, setup.receivedSetUpPacket, setup.firstPacketdataLen);
	uint16_t checksum = in_cksum((uint16_t*)tempDataPacket, setup.firstPacketdataLen);
	uint8_t flag;
	memcpy(&flag, tempDataPacket + sizeof(uint32_t) + sizeof(uint16_t) ,sizeof(uint8_t));

	//extracting the rcopy sequence number
	uint32_t clientSequenceNumber = 0;
	memcpy(&clientSequenceNumber, setup.receivedSetUpPacket, sizeof(uint32_t));

	//set the expected to current sequence number + 1
	globalServerBuffer.expected = ntohl(clientSequenceNumber) + 1;

	//if the flag is not the FILE_NAME_PACKET or checksum fails we want to end this child process by going to DONE state.
	if(checksum != 0 || flag != FILE_NAME_PACKET){
		return DONE;
	}

	//extract file name, windowsize and buffer size from the function
	int firstPacketPayloadDataSize = setup.firstPacketdataLen - sizeof(uint32_t) - sizeof(uint16_t) - sizeof(uint8_t);

	//extracting the payload information and storing it in the buffer
	extractFirstFilePacket(tempDataPacket + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t), firstPacketPayloadDataSize);

	//verify first that the file is able to open up
	setup.toFilePointer = getFile(globalServerBuffer.toFileName, setup.firstPacketdataLen - 4 - 2);
	//file failed to open
	if(setup.toFilePointer == NULL){
		*firstPacketSize = createInitialPacket(setup.serverSequenceNumber, FILE_OPENING_FAIL);
		setup.serverSequenceNumber++;
		if(*firstPacketSize <= 7){
			printf("The first packet size is not correct\n");
			return DONE;
		}
		//send the failed file opening falg to the client and end this process.
		safeSendto(setup.socketNum, setup.setUpPacket, *firstPacketSize, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
		return DONE;
	}
	else{
		*firstPacketSize = createInitialPacket(setup.serverSequenceNumber, RESPONSE_FLAG_NAME);
		setup.serverSequenceNumber++;
		if(*firstPacketSize <= 7){
			printf("The first packet size is not correct\n");
			return DONE;
		}
		//send the okay file flag
		safeSendto(setup.socketNum, setup.setUpPacket, *firstPacketSize, 0, (struct sockaddr *) &setup.client, setup.clientAddrLen);
	}

	//running while loop and setting up the initial connection with polling for one sec
	//setting up the pollset
	setupPollSet();

	//adding the server socket to the pollset
	addToPollSet(setup.socketNum);

	return INORDER;	//returns the Inorder state after setting everything up
}

void processState(char *argv[]){
	//create a new socket for this connetion
	if ((setup.socketNum = socket(AF_INET6,SOCK_DGRAM,0)) < 0)
	{
		perror("socket() call error");
		return;
	}

	//creating the state functions for this child process
	setup.serverSequenceNumber = 0;
	STATE state = START_STATE;
	int firstPacketSize = 0;


	while(1){
		switch(state){
			case START_STATE:
				state = start_state(&firstPacketSize);
			break;

			case INORDER:
				state = inorder();
			break;

			case BUFFER:
				state = buffer();
			break;

			case FLUSHING:
				state = flushing();
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

void processClient(int socketNum, char *argv[])
{
	setup.firstPacketdataLen = 0;
	setup.receivedSetUpPacket = (uint8_t *) malloc(MAXBUF);	  
	setup.clientAddrLen = sizeof(setup.client);	
	

	//main process loop
	while (1)
	{
		setup.firstPacketdataLen = safeRecvfrom(socketNum, setup.receivedSetUpPacket, MAXBUF, 0, (struct sockaddr *) &setup.client, &setup.clientAddrLen);

		uint16_t checkSum = in_cksum((uint16_t*)setup.receivedSetUpPacket, setup.firstPacketdataLen);
		if(checkSum == 0){
			//fork and start the child process state machines
			pid_t pid;

			pid = fork();
			if(pid < 0){
				printf("forking for the child process failed\n");
				exit(EXIT_FAILURE);
			}
			else if(pid == 0){
				//child process
				close(setup.socketNum);	//close the original socket for the child
				processState(argv);
				exit(EXIT_SUCCESS);
			}
		}
	}
}

		//verifying the pduPacket received from the client
		// printPDU((uint8_t *)buffer, dataLen);

		// just for fun send back to client number of bytes received
		// sprintf(buffer, "bytes: %d", dataLen);
		// safeSendto(socketNum, buffer, strlen(buffer)+1, 0, (struct sockaddr *) & client, clientAddrLen);

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 3)	//need to include the error that we are passing in
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 3)
	{
		portNumber = atoi(argv[2]);
		setup.errorRate = atof(argv[1]);
	}
	
	return portNumber;
}


