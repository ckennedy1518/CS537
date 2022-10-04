// Copyright 2022 Christopher Kennedy
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define MAX_BUFFER 255


int main(int argc, char* argv[]) {
    int fflag = 0;  // for file
    char* cvalue = NULL;
    char* stringToSearch = NULL;
    int flag1 = 0;  // for string
    int c;
    FILE* fp = stdin;

    while ((c = getopt(argc, argv, "-Vhf:")) != -1) {  // CLA functionality
        switch (c) {
        case 'V':
            if (flag1 == 1) {
                printf("my-look: invalid command line\n");
                exit(1);
            }
            printf("my-look from CS537 Spring 2022\n");
            exit(0);
        case 'h':
            if (flag1 == 1) {
                printf("my-look: invalid command line\n");
                exit(1);
            }
            printf("Usage: -f (filename) (line search token)\n");
            exit(0);
        case 'f':  // flag for file
            if (flag1 == 1) {
                printf("my-look: invalid command line\n");
                exit(1);
            }
            fflag = 1;
            cvalue = optarg;
            break;
        case 1:  // get string we're searching for
            if (flag1 == 1) {
                printf("my-look: invalid command line\n");
                exit(1);
            }
            stringToSearch = optarg;
            flag1 = 1;
            break;

        default:  // something went wrong
            printf("my-look: invalid command line\n");
            exit(1);
        }
    }

    if (flag1 == 0) {
        printf("No search key entered.\n");
        exit(1);
    }

    if (fflag == 1 && cvalue != NULL) {
        fp = fopen(cvalue, "r");
        if (fp == NULL) {
            printf("my-look: cannot open file\n");
            exit(1);
        }
    }

    char fileLine[MAX_BUFFER];
    while (fgets(fileLine, MAX_BUFFER, fp)) {  // check all lines of file
        char* stringComp = (char*) calloc(strlen(fileLine) + 1, sizeof(char));
        // plus 1 for null char
        if (stringComp == NULL) {
            printf("Error with calloc\n");
            exit(1);
        }
        int stringCompLen = 0;

        for (int i = 0; i < strlen(fileLine); i++) {
            if (isalnum(fileLine[i])) {
                stringComp[stringCompLen] = fileLine[i];
                stringCompLen++;
            }
        }
        stringComp[stringCompLen] = '\0';

        int STSLen = strlen(stringToSearch);
        if (strlen(stringComp) >= STSLen &&
        strncasecmp(stringComp, stringToSearch, STSLen) == 0) {
            // we have a match in what we're searching for and the line in file
            printf("%s", fileLine);
        }
        free(stringComp);
    }

    if (fflag == 1) {  // need to close file
        fclose(fp);
    }

    return 0;
}
