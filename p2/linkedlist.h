typedef struct linkedNode {
    char* key;  // name of alias
    int argc;  // num args w alias
    char** args;  // actual arguments
    struct linkedNode* next;  // pointer to the next node in LL
    struct linkedNode* previous;  // pointer to prev node in LL
} linkedNode;

linkedNode* aliasHead;
linkedNode* aliasTail;

linkedNode* lookup(char* key);
void printNode(linkedNode* toPrint);
void printList();
void addNode(char* keyAdd, int argcAdd, char** argsAdd);
linkedNode* removeNode(char* key);