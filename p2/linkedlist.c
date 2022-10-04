#include <string.h>
#include "linkedlist.h"
#include <stdlib.h>

linkedNode* lookup(char* key) {
    if (aliasHead == NULL) return NULL;

    linkedNode* itr = aliasHead;
    while (itr != NULL) {  // go through list until exhausted or found key
        if (strcmp(key, itr->key) == 0)
            return itr;
        itr = itr->next;
    }

    return NULL;
}

void printNode(linkedNode* toPrint) {
    if (toPrint == NULL) return;  // should never be but we can never be too sure

    for (int i = 1; i < toPrint->argc; i++) {  // start at 1 to skip alias
        write(1, toPrint->args[i], strlen(toPrint->args[i]));
        if (i != toPrint->argc - 1) {
            write(1, " ", strlen(" "));
        } else {
            write(1, "\n", strlen("\n"));
        }
    }
}

void printList() {
    if (aliasHead == NULL) return;
    
    linkedNode* itr = aliasHead;
    while (itr != NULL) {
        printNode(itr);
        itr = itr->next;
    }
}

void addNode(char* keyAdd, int argcAdd, char** argsAdd) {
    linkedNode* addMe = malloc(sizeof(linkedNode));  // create the new node
    addMe->key = keyAdd;
    addMe->argc = argcAdd;
    addMe->args = argsAdd;
    addMe->next = NULL;

    if (aliasHead == NULL) {
        aliasHead = addMe;
        aliasTail = addMe;
        return;
    }

    aliasTail->next = addMe;
    addMe->previous = aliasTail;
    aliasTail = addMe;
}

linkedNode* removeNode(char* key) {
    if (aliasHead == NULL) return NULL;

    linkedNode* toRemove = lookup(key);
    if (toRemove != NULL) {
        if (strcmp(toRemove->key, aliasHead->key) == 0 && strcmp(toRemove->key, aliasTail->key) == 0) {  
            // we're removing aliasHead as well as aliasTail
            aliasHead = aliasTail = NULL;
        } else if (strcmp(toRemove->key, aliasHead->key) == 0) {
            // we're removing aliasHead
            aliasHead = aliasHead->next;
            aliasHead->previous = NULL;
        } else if (strcmp(toRemove->key, aliasTail->key) == 0) {
            aliasTail = aliasTail->previous;
            aliasTail->next = NULL;
        } else {  // removing regular node
            toRemove->previous->next = toRemove->next;
            toRemove->next->previous = toRemove->previous;
        }
        return toRemove;
    }
    return NULL;  // no node found in lookup
}