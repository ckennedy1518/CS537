#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include "linkedlist.h"

#define MAX_BUFFER 513  // for some reason this will stop our lines at 512 (I believe the reason is in fgets)

// Trim leading space, helpful for checking exit
// return 0 if str is all space, 1 otherwise
int TrimLeadingWS(char *str) {
    int firstAlNumChar = 0;
    int foundANC = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (isspace(str[i])) {
            continue;
        }
        else {
            firstAlNumChar = i;
            foundANC = 1;
            break;
        }
    }
    if (!foundANC) {
        return 0;
    }
    for (int i = 0; i < strlen(str) - firstAlNumChar; i++) {
        str[i] = str[i + firstAlNumChar];
        if (i == strlen(str) - firstAlNumChar - 1)
            str[i + 1] = '\0';
    }

    return 1;
}

// function trims leading white space taking in a string and returning one
void MakeWhitespaceSpace(char* sToTrim) {
    for (int i = 0; i < strlen(sToTrim); i++) { 
        if (isspace(sToTrim[i]))
            sToTrim[i] = ' ';
    }
}

// main program for shell
int main(int argc, char* argv[]) {
    char* mallocFail;  // used in both batch & interactive mode
    mallocFail = "Malloc has failed";
    aliasHead = NULL;
    aliasTail = NULL;

    if (argc == 1) {  // interactive mode
        while (1) {  // simple loop to keep shell running
            char* shellPrompt;
            shellPrompt = "mysh> ";
            int redirectionUsed = 0;
            write(1, shellPrompt, strlen(shellPrompt));  // write to stdout

            char userInput[MAX_BUFFER];  // will hold user input after fgets call
            char* check = fgets(userInput, MAX_BUFFER, stdin);

            if (feof(stdin)) {  // for ctrl+D
                break;
            } else if (check == NULL || (strlen(userInput) == MAX_BUFFER - 1 && userInput[MAX_BUFFER - 2] != '\n')) {
                write(2, "Error with fgets.\n", strlen("Error with fgets.\n"));
                while(getchar() != '\n') ;  // continue until "flushing" stdin
                continue;
            }
            
            MakeWhitespaceSpace(userInput);  // get rid of leading ws
            int ret = TrimLeadingWS(userInput);
            if (!ret)
                continue;

            if (strlen(userInput) > 4) {  // do we need to exit?
                char shouldExit[5];
                for (int i = 0; i < 4; i++) {
                    shouldExit[i] = userInput[i];
                    if (i == 3)
                        shouldExit[i + 1] = '\0';
                }
                if (strcmp(shouldExit, "exit") == 0) break;
            }

            if (strlen(userInput) > 0) {  // this portion handles special cases with redirection
                char* eMessage;
                eMessage = "Redirection misformatted.\n";
                
                if (userInput[0] == '>') {  // first character is >, not allowed
                    write(2, eMessage, strlen(eMessage));
                    continue;
                }

                int counter = 0;  // this portion checks to make sure there is at most 1 '>'
                for (int i = 1; i < strlen(userInput); i++) {  // start at 1 already checked 0
                    if (userInput[i] == '>') {
                        counter++;
                        redirectionUsed = 1;
                    }
                    if (counter > 1) {
                        write(2, eMessage, strlen(eMessage));
                        break;
                    }
                }
                if (counter > 1)
                    continue;
                if (redirectionUsed) {
                    char userInput2[strlen(userInput)];
                    strcpy(userInput2, userInput);  // make a copy to ensure we don't alter ui1
                    char* dummy = strtok(userInput2, ">");
                    dummy = strtok(NULL, " ");
                    if (dummy == NULL) {  // we don't have a file defined after '>'
                        write(2, eMessage, strlen(eMessage));
                        continue;
                    }
                    dummy = strtok(NULL, " ");
                    if (dummy != NULL) {  // we have more than one file defined after '>'
                        write(2, eMessage, strlen(eMessage));
                        continue;
                    }

                    // now we need to go through string again and replace '>' with ' ' (for parsing args later)
                    for (int i = 0; i < strlen(userInput); i++) {  
                        if (userInput[i] == '>')
                            userInput[i] = ' ';
                    }
                }
            }
            
            char** args = malloc(sizeof(int) * MAX_BUFFER);
            char* token = strtok(userInput, " ");
            int argc = 0;
            while (token != NULL) {
                args[argc] = realloc(args[argc], strlen(token) * sizeof(char));
                strcpy(args[argc], token);
                token = strtok(NULL, " ");
                argc++;
            }

            // --- ALIASING ---
            if (strcmp(args[0], "alias") == 0) {
                if (argc == 1) {
                    printList(aliasHead); // print all alias
                } else if(argc == 2) {
                    linkedNode* findMe = lookup(args[1]);
                    if (findMe != NULL) { // print the alias
                        printNode(findMe);
                    }
                } else {  // adding or replacing an alias
                    linkedNode* findMe = lookup(args[1]);
                    if (findMe == NULL) {  // alias didn't exist
                        addNode(args[1], argc, args); // add the new alias
                    } else {  // alias is in list
                        findMe->argc = argc;
                        findMe->args = args; // re-assign the alias
                    }
                }
                continue;
            }
            if (strcmp(args[0], "unalias") == 0) {
                if(argc == 2) {
                    linkedNode* findMe = lookup(args[1]);
                    if (findMe != NULL) { // print the alias
                        removeNode(findMe->key); 
                    }
                } else {
                    char* err = "unalias: Incorrect number of arguments.\n";
                    write(2, err, strlen(err));
                }
                continue;
            }

            int forkRet = fork();  // have to do this in order to make sure fork() doesn't return an error
            if (forkRet > 0) {  // parent
                wait();
            } else if (forkRet == 0) {  // child
                if (redirectionUsed) {
                    int fd = open(args[argc - 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  
                    // always create new file or truncate existing one, write only
                    if (fd == -1) {
                        char* cantOpen = "Cannot write to file "; // TODO: ask
                        write(2, cantOpen, strlen(cantOpen));
                        write(2, args[argc - 1], strlen(args[argc - 1]));
                        write(2, ".\n", strlen(".\n"));
                        exit(1);
                    }

                    int dupReturn = dup2(fd, 1);  // only send stdout to file, we still want stderr to print
                    if (dupReturn == -1) {
                        char* cantOpen = "Cannot write to file "; // TODO: ask
                        write(2, cantOpen, strlen(cantOpen));
                        write(2, args[argc - 1], strlen(args[argc - 1]));
                        write(2, ".\n", strlen(".\n"));
                        exit(1);
                    }

                    close(fd);
                    args[argc - 1] = NULL;
                    argc--;
                }

                char** finalArgs;  // this portion of the code sees if we need to get an alias
                linkedNode* possibleCmds = lookup(args[0]);
                if (possibleCmds != NULL) {
                    finalArgs = malloc(sizeof(char*) * (possibleCmds->argc - 2 + argc));
                    int i;  // need to keep track for next loop
                    for (i = 0; i + 2 < possibleCmds->argc; i++) {
                        // malloc(sizeof(char) * strlen(possibleCmds->args[i + 2]));
                        finalArgs[i] = possibleCmds->args[i + 2];  // offset by two to start at cmd
                    }
                    for (int j = 1; j < argc; j++) {  // start at one to miss args[0]
                        // malloc(sizeof(char) * strlen(args[j]));
                        finalArgs[j + i - 1] = args[j];
                    }
                    execv(finalArgs[0], finalArgs);
                } else {
                    execv(args[0], args);
                }

                char* execvErr;  // line only reached if error w execv
                if (strlen(args[0]) < 1) break;
                if (possibleCmds != NULL) {
                    write(2, finalArgs[0], strlen(finalArgs[0]));
                } else {
                    write(2, args[0], strlen(args[0]));
                }
                execvErr = ": Command not found.\n";
                write(2, execvErr, strlen(execvErr));
                exit(1);
            } else {
                write(2, "Error with fork().\n", strlen("Error with fork().\n"));
                continue;
            }
        }
    } else if (argc > 2) {  // incorrect number of cla's entered
        write(2, "Usage: mysh [batch-file]\n", strlen("Usage: mysh [batch-file]\n"));
        exit(1);
    } else {  // batch mode
        FILE *fptr = fopen(argv[1], "r");
        if (fptr == NULL) {
            write(2, "Error: Cannot open file ", strlen("Error: Cannot open file "));
            write(2, argv[1], strlen(argv[1]));
            write(2, ".\n", strlen(".\n"));
            exit(1);
        }
        char buff[MAX_BUFFER];

        while (!feof(fptr)) {
            int redirectionUsed = 0;
            char* check = fgets(buff, MAX_BUFFER, fptr);
            if (check == NULL) {
                if (feof(fptr)) {  // reached eof
                    break;
                }
                write(2, "Error with fgets.\n", strlen("Error with fgets.\n"));
                while(getchar() != '\n') ;  // continue until line is "flushed"
                continue;
            } else if (strlen(buff) == MAX_BUFFER - 1 && buff[MAX_BUFFER - 2] != '\n') {
                write(1, buff, strlen(buff));  // need to print first 512 chars
                write(2, "Error with fgets.\n", strlen("Error with fgets.\n"));
                continue;
            }

            write(1, buff, strlen(buff));
            MakeWhitespaceSpace(buff);  // get rid of leading ws
            int ret = TrimLeadingWS(buff);
            if (!ret)
                continue;

            if (strlen(buff) > 4) {  // do we need to exit?
                char shouldExit[5];
                for (int i = 0; i < 4; i++) {
                    shouldExit[i] = buff[i];
                    if (i == 3)
                        shouldExit[i + 1] = '\0';
                }
                if (strcmp(shouldExit, "exit") == 0) break;
            }

            if (strlen(buff) > 0) {  // this portion handles special cases with redirection
                char* eMessage;
                eMessage = "Redirection misformatted.\n";
                
                if (buff[0] == '>') {  // first character is >, not allowed
                    write(2, eMessage, strlen(eMessage));
                    continue;
                }

                int counter = 0;  // this portion checks to make sure there is at most 1 '>'
                for (int i = 1; i < strlen(buff); i++) {  // start at 1 already checked 0
                    if (buff[i] == '>') {
                        counter++;
                        redirectionUsed = 1;
                    }
                    if (counter > 1) {
                        write(2, eMessage, strlen(eMessage));
                        break;
                    }
                }
                if (counter > 1)
                    continue;
                if (redirectionUsed) {
                    char userInput2[strlen(buff)];
                    strcpy(userInput2, buff);  // make a copy to ensure we don't alter ui1
                    char* dummy = strtok(userInput2, ">");
                    dummy = strtok(NULL, " ");
                    if (dummy == NULL) {  // we don't have a file defined after '>'
                        write(2, eMessage, strlen(eMessage));
                        continue;
                    }
                    dummy = strtok(NULL, " ");
                    if (dummy != NULL) {  // we have more than one file defined after '>'
                        write(2, eMessage, strlen(eMessage));
                        continue;
                    }

                    // now we need to go through string again and replace '>' with ' ' (for parsing args later)
                    for (int i = 0; i < strlen(buff); i++) {  
                        if (buff[i] == '>')
                            buff[i] = ' ';
                    }
                }
            }

            char** args = malloc(sizeof(int) * MAX_BUFFER);
            if (args == NULL) {
                write(2, mallocFail, strlen(mallocFail));
                exit(1);
            }
            char* token = strtok(buff, " ");
            int argc = 0;
            while (token != NULL) {
                args[argc] = realloc(args[argc], strlen(token) * sizeof(char));
                strcpy(args[argc], token);
                token = strtok(NULL, " ");
                argc++;
            }

            // --- ALIASING ---
            if (strcmp(args[0], "alias") == 0) {
                if (argc == 1) {
                    printList(aliasHead); // print all alias
                } else if(argc == 2) {
                    linkedNode* findMe = lookup(args[1]);
                    if (findMe != NULL) { // print the alias
                        printNode(findMe);
                    }
                } else {  // adding or replacing an alias
                    linkedNode* findMe = lookup(args[1]);
                    if (findMe == NULL) {  // alias didn't exist
                        addNode(args[1], argc, args); // add the new alias
                    } else {  // alias is in list
                        findMe->argc = argc;
                        findMe->args = args; // re-assign the alias
                    }
                }
                continue;
            }
            if (strcmp(args[0], "unalias") == 0) {
                if(argc == 2) {
                    linkedNode* findMe = lookup(args[1]);
                    if (findMe != NULL) { // print the alias
                        removeNode(findMe->key); 
                    }
                } else {
                    char* err = "unalias: Incorrect number of arguments.\n";
                    write(2, err, strlen(err));
                }
                continue;
            }

            int forkRet = fork();  // have to do this in order to make sure fork() doesn't return an error
            if (forkRet > 0) {  // parent
                wait();
            } else if (forkRet == 0) {  // child
                if (redirectionUsed) {
                    int fd = open(args[argc - 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);  
                    // always create new file or truncate existing one, write only
                    if (fd == -1) {
                        char* cantOpen = "Cannot write to file "; // TODO: ask
                        write(2, cantOpen, strlen(cantOpen));
                        write(2, args[argc - 1], strlen(args[argc - 1]));
                        write(2, ".\n", strlen(".\n"));
                        exit(1);
                    }

                    int dupReturn = dup2(fd, 1);  // only send stdout to file, we still want stderr to print
                    if (dupReturn == -1) {
                        char* cantOpen = "Cannot write to file "; // TODO: ask
                        write(2, cantOpen, strlen(cantOpen));
                        write(2, args[argc - 1], strlen(args[argc - 1]));
                        write(2, ".\n", strlen(".\n"));
                        exit(1);
                    }

                    close(fd);
                    args[argc - 1] = NULL;
                    argc--;
                }

                char** finalArgs;  // this portion of the code sees if we need to get an alias
                linkedNode* possibleCmds = lookup(args[0]);
                if (possibleCmds != NULL) {
                    finalArgs = malloc(sizeof(char*) * (possibleCmds->argc - 2 + argc));
                    int i;  // need to keep track for next loop
                    for (i = 0; i + 2 < possibleCmds->argc; i++) {
                        // malloc(sizeof(char) * strlen(possibleCmds->args[i + 2]));
                        finalArgs[i] = possibleCmds->args[i + 2];  // offset by two to start at cmd
                    }
                    for (int j = 1; j < argc; j++) {  // start at one to miss args[0]
                        // malloc(sizeof(char) * strlen(args[j]));
                        finalArgs[j + i - 1] = args[j];
                    }
                    execv(finalArgs[0], finalArgs);
                } else {
                    execv(args[0], args);
                }

                char* execvErr;  // line only reached if error w execv
                if (strlen(args[0]) < 1) break;
                if (possibleCmds != NULL) {
                    write(2, finalArgs[0], strlen(finalArgs[0]));
                } else {
                    write(2, args[0], strlen(args[0]));
                }
                execvErr = ": Command not found.\n";
                write(2, execvErr, strlen(execvErr));
                exit(1);
            } else {
                write(2, "Error with fork().\n", strlen("Error with fork().\n"));
                continue;
            }
        }
        fclose(fptr);
    }
    exit(0);
}
