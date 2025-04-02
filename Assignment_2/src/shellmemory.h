#define MEM_SIZE 1000

void mem_init ();
char *mem_get_value (char *var);
void mem_set_value (char *var, char *value);

// Script storage
#define FILENAME_LENGTH 100
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 101     // extra space for null terminator
#define MAX_SCRIPTS 5
#include <stdio.h>

struct ScriptFile {
    char filename[FILENAME_LENGTH];
    char **lines;
    int numLines;
    struct ScriptFile *prev;
    struct ScriptFile *next;
};

// Function prototypes
struct ScriptFile *getScript (const char *filename);
int freeScript (const char *filename);

extern int totalStoredLines;
