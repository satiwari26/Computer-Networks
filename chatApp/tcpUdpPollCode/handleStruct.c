#include "handleStruct.h"

/**
 * @brief
 * node to store the handle table and sockeNumber
*/
struct handleTable{
    char * handleName;
    int socket_number;
    struct handleTable * next;
};

/**
 * @brief
 * global header pointer that would point to the starting of the header if
 * first node is present otherwise null
*/
struct handleTable * handleHeader = NULL;

/**
 * @brief
 * add new handle to the table
 * @param handleName    //need to ensure that handleName is store 
 *                      in an array somewhere before passed
 * @param sockeNumber
*/
void addNewHandle(char * handleName, int sockeNumber){
    struct handleTable * table = (struct handleTable *)malloc(sizeof(struct handleTable));
    if (table == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    table->handleName = handleName;
    table->socket_number = sockeNumber;
    table->next = NULL;

    if(handleHeader == NULL){   //if there are no nodes add the first node
        handleHeader = table;
    }
    else{
        //current pointer to traverse through the list and assigned at the end
        struct handleTable * curr = handleHeader;
        //traverse through the linkedList in O(n) to find the last inserted element
        while(curr->next != NULL){
            curr = curr->next;
        }
        curr->next = table;
    }
}

/**
 * @brief
 * removes the handle element from the list
 * @param handleName (for the comparison of the file)
*/
void removeHandle(char * handleName){
    bool existHandle = false;
    if(handleHeader == NULL){
        printf("Error there are no clients on the handle");
        return;
    }
    struct handleTable * curr = handleHeader;
    struct handleTable * prev = curr;
    do{
        if(strcmp(curr->handleName,handleName) == 0){   //if the two string is same
            if(curr == handleHeader){   //if the curr pointer is header move header to next
                handleHeader = curr->next;
                free(curr); //remove the current node from the link
                existHandle = true;
                break;
            }
            else{
                prev->next = curr->next;
                free(curr);
                existHandle = true;
                break;
            }
        }
        prev = curr;
        curr = curr->next;
    }while(curr != NULL);

    existHandle == false ? printf("The handle doesn't exist\n") : printf("The handle have been deleted\n");
}

/**
 * @brief
 * gets the socket number based of the handleName provided
 * if handleName doesn't exist returns -1
 * @param handleName
*/
int getSocketNumber(char * handleName){
    struct handleTable * curr = handleHeader;
    if(curr == NULL){
        return -1;
    }

    while(curr != NULL){
        if(strcmp(curr->handleName,handleName) == 0){
            return curr->socket_number;
        }
        curr = curr->next;
    }

    //at this point if not found the socketNumber return -1
    return -1;
}

/**
 * @brief
 * gets the socket number based of the handleName provided
 * if sockeNumber doesn't exist returns NULL
 * @param handleName
*/
char * getHandleName(int sockeNumber){
    struct handleTable * curr = handleHeader;
    if(curr == NULL){
        return NULL;
    }

    while(curr != NULL){
        if(curr->socket_number == sockeNumber){
            return curr->handleName;
        }
        curr = curr->next;
    }

    //at this point if not found the socketNumber return -1
    return NULL;
}