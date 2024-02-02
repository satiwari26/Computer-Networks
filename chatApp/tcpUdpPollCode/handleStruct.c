#include "handleStruct.h"

/**
 * @brief
 * node to store the handle table and sockeNumber
*/
struct handleTable{
    char * handleName;
    int handelLen;
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
 * to perform the comparison between the existing handel and provided handel
 * 
*/
int cmpHandel(struct handleTable * table, char * handleN, uint8_t handelL){
    if(table->handelLen != handelL){
        return -1;
    }
    printf("%s",table->handleName);
    char existing[table->handelLen + 1];
    char newString[handelL + 1];
    memcpy(existing,table->handleName,table->handelLen);
    existing[table->handelLen] = '\0';

    memcpy(newString,handleN,handelL);
    newString[handelL] = '\0';

    // Print or debug to verify the content of the strings
    printf("Existing: %s\n", existing);
    printf("New String: %s\n", newString);

    if(strncmp(existing,newString, handelL) == 0){
        return 0;
    }
    else{
        return -1;
    }
}

/**
 * @brief
 * add new handle to the table
 * @param handleName    //need to ensure that handleName is store 
 *                      in an array somewhere before passed
 * @param sockeNumber
*/
void addNewHandle(char * handleName, int sockeNumber, uint8_t handelLen){
    char *handelArray = malloc(handelLen + 1);
    memcpy(handelArray,handleName,handelLen);
    handelArray[handelLen] = '\0';

    struct handleTable * table = (struct handleTable *)malloc(sizeof(struct handleTable));
    if (table == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    table->handleName = handelArray;
    table->socket_number = sockeNumber;
    table->handelLen = handelLen;
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
void removeHandle(char * handleName, uint8_t handelLen){
    char *handelArray = malloc(handelLen + 1);
    memcpy(handelArray,handleName,handelLen);
    handelArray[handelLen] = '\0';

    bool existHandle = false;
    if(handleHeader == NULL){
        printf("Error there are no clients on the handle");
        return;
    }
    struct handleTable * curr = handleHeader;
    struct handleTable * prev = curr;
    do{
        if(cmpHandel(curr,handelArray, handelLen) == 0){   //if the two string is same
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
int getSocketNumber(char * handleName, uint8_t handelLen){
    char *handelArray = malloc(handelLen + 1);
    memcpy(handelArray,handleName,handelLen);
    handelArray[handelLen] = '\0';

    struct handleTable * curr = handleHeader;
    if(curr == NULL){
        return -1;
    }

    while(curr != NULL){
        printf("%s\n",curr->handleName);
        if(cmpHandel(curr,handelArray, handelLen) == 0){
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

/**
 * @brief
 * provides the number of the handleCounts and Handle Names
 * with 1 bytes padded in front of each handle names
*/
uint32_t HandlesCount(uint8_t * handelNames){
    struct handleTable * curr = handleHeader;
    uint32_t HandlesCounts = 0;
    if(curr == NULL){
        handelNames = NULL;
        return 0;
    }

    int offSetPointer = 0;
    while(curr != NULL){
        HandlesCounts++;
        memcpy(handelNames + offSetPointer,&curr->handelLen, sizeof(uint8_t));
        memcpy(handelNames + offSetPointer + sizeof(uint8_t), (uint8_t *)curr->handleName, curr->handelLen);
        offSetPointer += (curr->handelLen + sizeof(uint8_t));
        curr = curr->next;
    }

    return HandlesCounts;
}

/**
 * @brief
 * function only returns the number of the handle counts
 * present in the handle list
 * @return HandlesCounts
*/
uint32_t onlyHandelCount(){
    struct handleTable * curr = handleHeader;
    uint32_t HandlesCounts = 0;
    if(curr == NULL){
        return 0;
    }

    while(curr != NULL){
        HandlesCounts++;
        curr = curr->next;
    }

    return HandlesCounts;
}

/**
 * @brief
 * provides the socket number for all the handles present
 * @param HandlesSocket
 * @return HandlesCounts
*/
void HandlesSocketNum(int * HandelsSocket){
    struct handleTable * curr = handleHeader;
    uint32_t HandlesCounts = 0;
    if(curr == NULL){
        HandelsSocket = NULL;
        return;
    }

    while(curr != NULL){
        memcpy(&HandelsSocket[HandlesCounts],&curr->socket_number, sizeof(int));
        curr = curr->next;
        HandlesCounts++;
    }
}
