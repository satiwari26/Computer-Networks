// 
// Writen by Hugh Smith, Jan. 2023
//
// Put in system calls with error checking.

#ifndef __SAFEUTIL_H__
#define __SAFEUTIL_H__

#include <stdint.h>
#include <stddef.h>

int safeRecv(int socketNum, uint8_t * buffer, int bufferLen, int flag);
int safeSend(int socketNum, uint8_t * buffer, int bufferLen, int flag);

void * srealloc(void *ptr, size_t size);
void * sCalloc(size_t nmemb, size_t size);


#endif