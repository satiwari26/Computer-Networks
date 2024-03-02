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

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "checksum.h"


/**
 * @brief
 * function creates the pdu packet
 * @param pduBuffer
 * @param sequenceNumber
 * @param flag
 * @param payload
 * @param payLoadLen
*/
int createPDU(uint8_t **pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t * payload, int payLoadLen);

/**
 * @brief
 * prints the content of packet.
 * Verifies that the packet data is not corrupted and checksum is also correct
*/
void printPDU(uint8_t *aPDU, int pduLength);

/**
 * @brief
 * extracting the file-name, window-size, buffer-size
*/
void extractFirstFilePacket(uint8_t * buffer, int payloadLen);