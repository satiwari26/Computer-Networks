#include "safeUtil.h"   //provided by the prof Smith
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
#include <errno.h>
#include <stddef.h>

int sendPDU(int clientSocket, uint8_t flag, uint8_t * dataBuffer, int lengthOfData);