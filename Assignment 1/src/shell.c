#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int parseInput (char ui[]);

// Start of everything
int main (int argc, char *argv[]) {
    printf ("Shell version 1.4 created December 2024\n");

    char prompt = '$';          // Shell prompt
    char userInput[MAX_USER_INPUT];     // user's input stored here
    int errorCode = 0;          // zero means no error, default

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }

    //init shell memory
    mem_init ();
    while (1) {
        if (isatty (STDIN_FILENO)) {
            printf ("%c ", prompt);     // Show prompt only if interactive
        }

        if (fgets (userInput, MAX_USER_INPUT - 1, stdin) == NULL) {
            break;
        }

        errorCode = parseInput (userInput);
        if (errorCode == -1) {
            exit (99);          // ignore all other errors
        }
        memset (userInput, 0, sizeof (userInput));
    }

    return 0;
}

int wordEnding (char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ' || c == ';';
}

int parseInput (char inp[]) {
    char tmp[1000], *words[300];
    int ix = 0, w, totalWords = 0, tmpIx = 0;
    int wordlen;
    int errorCode;
    int cmdCount = 0;
    for (ix = 0; inp[ix] == ' ' && ix < 1000; ix++);    // skip white spaces
    // extract line
    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000 && tmpIx < 1000) {
        // reset number of words for this command
        w = 0;
        // extract command 
        while (inp[ix] != '\n' && inp[ix] != '\0' && inp[ix] != ';'
               && ix < 1000 && tmpIx < 1000) {
            // Skip spaces
            while (inp[ix] == ' ' && ix < 1000)
                ix++;
            // starting index of word
            int start = tmpIx;

            // extract a word
            for (wordlen = 0;
                 !wordEnding (inp[ix]) && ix < 1000 && tmpIx < 1000;
                 ix++, wordlen++, tmpIx++) {
                tmp[tmpIx] = inp[ix];
            }
            tmp[tmpIx] = '\0';
            words[totalWords] = strdup (&tmp[start]);
            w++;
            totalWords++;
        }
        ix++;
        cmdCount++;

        // execute command
        if (w > 0) {
            words[totalWords] = NULL;
            errorCode = interpreter (words + (totalWords - w), w);
            if (errorCode != 0)
                return errorCode;
        }
    }
    return errorCode;
}
