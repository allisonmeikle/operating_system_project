#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
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
    char tmp[200], *words[100];
    int ix = 0, w = 0;
    int wordlen;
    int errorCode = 0;

    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
        // skip white spaces
        for (; isspace (inp[ix]) && inp[ix] != '\n' && ix < 1000; ix++);

        // If the next character is a semicolon, run what we have so far
        if (inp[ix] == ';')
            break;

        // extract a word
        for (wordlen = 0; !wordEnding (inp[ix]) && ix < 1000; ix++, wordlen++) {
            tmp[wordlen] = inp[ix];
        }

        if (wordlen > 0) {
            tmp[wordlen] = '\0';
            words[w] = strdup (tmp);
            w++;
            if (inp[ix] == '\0')
                break;
        } else {
            break;
        }
    }
    // Ignore commands that contain no (meaningful) input
    if (w > 0) {
        errorCode = interpreter (words, w);
        for (size_t i = 0; i < w; ++i) {
            free (words[i]);
        }
    }
    if (inp[ix] == ';') {
        // handle the next command in the chain by recursing the parser
        return parseInput (&inp[ix + 1]);
    }
    return errorCode;
}


