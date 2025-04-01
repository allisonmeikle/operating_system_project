#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"
#include "scheduler.h"

int parseInput (char ui[]);
int backgroundMode = 0;
int firstBackground = 0;
char *batchScript = "batchScript";      // filename to store user input after # in exec
char *execArgs[100];            // arguments for exec with #
int execArgsCount = 0;          // num argument for exec with #

// Start of everything
int main (int argc, char *argv[]) {
    //printf ("Shell version 1.4 created December 2024\n\n");
    printf ("Frame Store Size = %d; Variable Store Size = %d\n",
            FRAME_MEM_SIZE, VAR_MEM_SIZE);
    char prompt = '$';          // Shell prompt
    char userInput[MAX_USER_INPUT];     // user's input stored here
    int errorCode = 0;          // zero means no error, default

    //init user inputa
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }

    // init variable memory
    varMemInit ();
    // init frame memory
    frameMemInit ();

    while (1) {
        if (isatty (STDIN_FILENO)) {
            printf ("%c ", prompt);     // Show prompt only if interactive
        }

        if (fgets (userInput, MAX_USER_INPUT - 1, stdin) == NULL) {
            if (backgroundMode) {
                initReadyQueue ();
                int errCode = exec (execArgs, execArgsCount);
                for (int i = 0; i < execArgsCount; i++) {
                    free (execArgs[i]);
                }
                execArgsCount = 0;
                remove (batchScript);
                free (batchScript);
            }
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

    // if writeToFile == 1, want to write commands to file
    if (firstBackground) {
        FILE *batchFile = fopen (batchScript, "a");
        if (!batchFile) {
            //printf("Error: Could not open batch script file.\n");
            return 1;
        }
        fputs (inp, batchFile);
        fclose (batchFile);
        return 0;
    }
    // This function probably isn't the best place to handle chains.
    // That is, if we really wanted to implement relatively complex
    // syntax like that of bash, we should really just tokenize everything,
    // send the ';' as a separate word to the interpreter, and let the
    // interpreter sort it out later.
    // But for this simple shell, the interpreter's job is really only to be
    // command dispatch, and this function is really acting as a complete
    // parser rather than just a tokenizer. So we'll handle it here.

    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
        // skip white spaces
        for (; isspace (inp[ix]) && inp[ix] != '\n' && ix < 1000; ix++);

        // If the next character is a semicolon,
        // we should run what we have so far.
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
    // Ignore commands that contain no (meaningful) input by only calling the
    // interpreter if actually found words.
    if (w > 0) {
        // check here if command was exec w # and change background
        if (strcmp (words[0], "exec") == 0 && strcmp (words[w - 1], "#") == 0) {
            backgroundMode = 1;
            firstBackground = 1;
            words[w - 1] = NULL;
            w--;
            for (int i = 1; i < w; i++) {
                execArgs[i - 1] = strdup (words[i]);
                execArgsCount++;
            }
            FILE *batchFile = fopen (batchScript, "w");
            if (!batchFile) {
                printf ("Error: Could not create batch script file.\n");
                return 1;
            }
            fclose (batchFile);
            for (size_t i = 0; i < w; ++i) {
                free (words[i]);
            }
            return 0;
        }

        errorCode = interpreter (words, w);
        for (size_t i = 0; i < w; ++i) {
            free (words[i]);
        }
    }
    if (inp[ix] == ';') {
        // handle the next command in the chain by recursing
        // the parser. We could equivalently wrap all of the work above
        // in a while loop, but this makes it clearer what's going on.
        // Additionally, a modern compiler is more than smart enough to
        // turn this into a loop for us! Try adding -O2 to the CFLAGS in
        // the Makefile and then read the assembly we get.
        // Or you might be interested in godbolt.org.
        return parseInput (&inp[ix + 1]);
    }
    return errorCode;
}
