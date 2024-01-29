#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief
 * add new handle to the table
 * @param handleName    //need to ensure that handleName is store 
 *                      in an array somewhere before passed
 * @param sockeNumber
*/
void addNewHandle(char * handleName, int sockeNumber);

/**
 * @brief
 * removes the handle element from the list
 * @param handleName (for the comparison of the file)
*/
void removeHandle(char * handleName);

/**
 * @brief
 * gets the socket number based of the handleName provided
 * if handleName doesn't exist returns -1
 * @param handleName
*/
int getSocketNumber(char * handleName);

/**
 * @brief
 * gets the socket number based of the handleName provided
 * if sockeNumber doesn't exist returns NULL
 * @param handleName
*/
char * getHandleName(int sockeNumber);
