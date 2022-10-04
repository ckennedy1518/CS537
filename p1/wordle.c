// Copyright 2022 Christopher Kennedy
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER 255
#define DESIRED_WORD_LENGTH 5

int main(int argc, char* argv[]) {
    int numArgsWanted = 3;
    if (argc != numArgsWanted) {
        printf("wordle: invalid number of args\n");
        exit(1);
    }

    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("wordle: cannot open file\n");
        exit(1);
    }

    char* cancelledChars = &argv[2][0];
    char fileLine[MAX_BUFFER];
    while (fgets(fileLine, MAX_BUFFER, fp)) {
        int breakSearch = 0;
        if (strlen(fileLine) == DESIRED_WORD_LENGTH + 1) {
            // need to account for newline char
            for (int i = 0; i < strlen(cancelledChars); i++) {
                // double for loop iterates through
                // fileLine and cancelledChars to make sure no matches
                for (int j = 0; j < DESIRED_WORD_LENGTH; j++) {
                    if (cancelledChars[i] == fileLine[j]) {
                        breakSearch = 1;  // we have a match
                        break;
                    }
                }
                if (breakSearch == 1)
                    break;
            }
            if (breakSearch == 0)  // no chars were the same
                printf("%s", fileLine);
        }
    }

    fclose(fp);
    return 0;
}
