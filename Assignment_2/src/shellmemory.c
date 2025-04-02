#include <stdlib.h>
#include <string.h>
#include "shellmemory.h"
#include "shell.h"

struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

// Helper functions
int match (char *model, char *var) {
    int i, len = strlen (var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i])
            matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else
        return 0;
}

// Shell memory functions
void mem_init () {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value (char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp (shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup (value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp (shellmemory[i].var, "none") == 0) {
            shellmemory[i].var = strdup (var_in);
            shellmemory[i].value = strdup (value_in);
            return;
        }
    }

    return;
}

//get value based on input key
char *mem_get_value (char *var_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp (shellmemory[i].var, var_in) == 0) {
            return strdup (shellmemory[i].value);
        }
    }
    return "Variable does not exist";
}

// Script processing
struct ScriptFile *scriptListHead = NULL;
int scriptCount = 0;
int totalStoredLines = 0;

static int errorGeneral (char *message) {
    printf ("Error: %s\n", message);
    return 2;
}

int loadScript (const char *filename) {
    FILE *file = fopen (filename, "rt");        // the program is in a file

    if (file == NULL) {
        char errorMessage[256];
        sprintf (errorMessage,
                 "loadScript could not open file with this name %s", filename);
        return errorGeneral (errorMessage);
    }

    if (totalStoredLines >= MAX_LINES) {
        return
            errorGeneral
            ("loadScript, too many lines loaded, please unload some scripts and try again");
    }

    struct ScriptFile *script = malloc (sizeof (struct ScriptFile));
    if (script == NULL) {
        return
            errorGeneral ("loadScript could not allocate memory for script");
    }

    strncpy (script->filename, filename, FILENAME_LENGTH - 1);
    script->filename[FILENAME_LENGTH - 1] = '\0';
    script->numLines = 0;

    int capacity = 10;
    script->lines = malloc (capacity * sizeof (char *));
    if (script->lines == NULL) {
        free (script);
        return
            errorGeneral
            ("loadScript could not allocate memory for script lines");
    }

    char line[MAX_LINE_LENGTH];
    while (fgets (line, MAX_LINE_LENGTH - 1, file) != NULL) {
        if (totalStoredLines >= MAX_LINES) {
            freeScript (script->filename);
            return
                errorGeneral
                ("loadScript too many lines loaded, please unload some scripts and try again");
        }
        line[MAX_LINE_LENGTH - 1] = '\0';

        char *newLine = strdup (line);
        if (newLine == NULL) {
            freeScript (script->filename);
            return
                errorGeneral
                ("loadScript could not allocate memory for script line");
        }

        if (script->numLines >= capacity) {
            capacity *= 2;      // Double the capacity
            char **newLines =
                realloc (script->lines, capacity * sizeof (char *));
            if (newLines == NULL) {
                free (newLine);
                freeScript (script->filename);
                return
                    errorGeneral
                    ("loadScript could not allocate memory for script lines");
            }
            script->lines = newLines;
        }

        script->lines[script->numLines] = newLine;
        script->numLines++;
        totalStoredLines++;
    }

    fclose (file);
    script->next = scriptListHead;
    script->prev = NULL;
    if (scriptListHead != NULL) {
        scriptListHead->prev = script;
    }
    scriptListHead = script;
    scriptCount++;
    return 0;
}

struct ScriptFile *getScript (const char *filename) {
    struct ScriptFile *current = scriptListHead;
    while (current != NULL) {
        if (strcmp (current->filename, filename) == 0) {
            return current;
        }
        current = current->next;
    }
    int errCode = loadScript (filename);
    if (errCode != 0) {
        return NULL;
    }
    // Re-check the script list after loading
    current = scriptListHead;
    while (current != NULL) {
        if (strcmp (current->filename, filename) == 0) {
            return current;
        }
        current = current->next;
    }

    // If we reach here, something went wrong
    errorGeneral ("getScript could not find or load the script");
    return NULL;
}

int freeScript (const char *filename) {
    struct ScriptFile *script = getScript (filename);
    if (script == NULL) {
        return
            errorGeneral
            ("freeScript could not find a script with specified filename");
    }
    if (script->prev != NULL) {
        struct ScriptFile *prev = script->prev;
        prev->next = script->next;
    }
    if (script->next != NULL) {
        struct ScriptFile *next = script->next;
        next->prev = script->prev;
    }
    if (script == scriptListHead) {
        scriptListHead = script->next;
    }
    for (int i = 0; i < script->numLines; i++) {
        free (script->lines[i]);
    }
    free (script->lines);
    totalStoredLines -= script->numLines;
    free (script);
    scriptCount--;
    return 0;
}
