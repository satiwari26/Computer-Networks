
// 
// Writen by Hugh Smith, April 2020
//
// Put in system calls with error checking
// and and an s to the name: srealloc()
// keep the function paramaters same as system call

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

#include "networks.h"
#include "safeUtil.h"

int safeRecv(int socketNum, uint8_t * buffer, int bufferLen, int flag)
{
    int bytesReceived = recv(socketNum, buffer, bufferLen, flag);
    if (bytesReceived < 0)
    {
        if (errno == ECONNRESET)
        {
            bytesReceived = 0;
        }
        else
        {
            perror("recv call");
            exit(-1);
        }
    }
    return bytesReceived;
}

int safeSend(int socketNum, uint8_t * buffer, int bufferLen, int flag)
{
	int bytesSent = 0;
	if ((bytesSent = send(socketNum, buffer, bufferLen, flag)) < 0)
	{
        perror("recv call");
       exit(-1);
     }
	 
    return bytesSent;
}


void * srealloc(void *ptr, size_t size)
{
	void * returnValue = NULL;
	
	if ((returnValue = realloc(ptr, size)) == NULL)
	{
		printf("Error on realloc (tried for size: %d\n", (int) size);
		exit(-1);
	}
	
	return returnValue;
} 

void * sCalloc(size_t nmemb, size_t size)
{
	void * returnValue = NULL;
	if ((returnValue = calloc(nmemb, size)) == NULL)
	{
		perror("calloc");
		exit(-1);
	}
	return returnValue;
}

