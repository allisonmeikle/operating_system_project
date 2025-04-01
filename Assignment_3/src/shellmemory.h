#include <stdio.h>
#include <sys/types.h>
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 101     // extra space for null terminator
#define FRAME_SIZE 3            // 3 lines of code per frame

#ifndef FRAME_MEM_SIZE
#   define FRAME_MEM_SIZE 18    // default, 6 frames (0-5)
#endif

#ifndef VAR_MEM_SIZE
#   define VAR_MEM_SIZE 10
#endif

#define NUM_FRAMES (FRAME_MEM_SIZE / FRAME_SIZE)


void varMemInit ();
void frameMemInit ();
char *mem_get_value (char *var);
void mem_set_value (char *var, char *value);

extern int globalLRUCounter;

struct PCB {
    pid_t pid;
    int curLine;
    struct PCB *next;
    char *scriptName;
    FILE *scriptFile;
    struct PageTable *pageTable;
};

struct Frame {
    char *code[FRAME_SIZE];
    char *filename;
    int pageNum;
    int validBit;
    int lastAccessTime;
    // Dynamically allocated array of pointers to Page structs (page table entries) that currently reference this frame (used for reverse mapping)
    struct Page **references;
    // number of pages that currently reference this frame 
    int refCount;
    // current capacity for number of references
    int refCapacity;
};

struct Page {
    int frameNum;
    int validBit;               // is page currently loaded in memory
};

struct PageTable {
    // dynamically allocated array of Page entries
    struct Page *pages;
    // current allocated size 
    int capacity;
    // number of pages currently used          
    int numPages;
};

// loads first two pages of code for this process into frameStore and initializes it's PageTable
struct PageTable *initPageTable (struct PCB *pcb);
// returns current line of code for process
char *getLineForProcess (struct PCB *pcb);
// frees all memory used by process (excluding frames)
int freeProcess (struct PCB *pcb);
