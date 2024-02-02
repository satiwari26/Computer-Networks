#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

/**
 * @brief
 * add new handle to the table
 * @param handleName    //need to ensure that handleName is store 
 *                      in an array somewhere before passed
 * @param sockeNumber
*/
void addNewHandle(char * handleName, int sockeNumber, uint8_t handelLen);

/**
 * @brief
 * removes the handle element from the list
 * @param handleName (for the comparison of the file)
*/
void removeHandle(char * handleName, uint8_t handelLen);

/**
 * @brief
 * gets the socket number based of the handleName provided
 * if handleName doesn't exist returns -1
 * @param handleName
*/
int getSocketNumber(char * handleName, uint8_t handelLen);

/**
 * @brief
 * gets the socket number based of the handleName provided
 * if sockeNumber doesn't exist returns NULL
 * @param handleName
*/
char * getHandleName(int sockeNumber);

/**
 * @brief
 * provides the number of the handleCounts and Handle Names
 * with 1 bytes padded in front of each handle names
*/
uint32_t HandlesCount(uint8_t * handelNames);

/**
 * @brief
 * function only returns the number of the handle counts
 * present in the handle list
 * @return HandlesCounts
*/
uint32_t onlyHandelCount();

/**
 * @brief
 * provides the socket number for all the handles present
 * @param HandlesSocket
 * @return HandlesCounts
*/
void HandlesSocketNum(int * HandelsSocket);
