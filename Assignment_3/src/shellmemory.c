#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "shellmemory.h"
#include "shell.h"

struct varMemoryStruct {
    char *var;
    char *value;
};

struct varMemoryStruct varStore[VAR_MEM_SIZE];

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

// Variable memory functions
void varMemInit () {
    int i;
    for (i = 0; i < VAR_MEM_SIZE; i++) {
        varStore[i].var = "none";
        varStore[i].value = "none";
    }
}

// Set key value pair
void mem_set_value (char *var_in, char *value_in) {
    int i;

    for (i = 0; i < VAR_MEM_SIZE; i++) {
        if (strcmp (varStore[i].var, var_in) == 0) {
            varStore[i].value = strdup (value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < VAR_MEM_SIZE; i++) {
        if (strcmp (varStore[i].var, "none") == 0) {
            varStore[i].var = strdup (var_in);
            varStore[i].value = strdup (value_in);
            return;
        }
    }

    return;
}

//get value based on input key
char *mem_get_value (char *var_in) {
    int i;
    for (i = 0; i < VAR_MEM_SIZE; i++) {
        if (strcmp (varStore[i].var, var_in) == 0) {
            return strdup (varStore[i].value);
        }
    }
    return "Variable does not exist";
}

// A3
int globalLRUCounter = 0;
struct Frame frameStore[NUM_FRAMES];

static void errorGeneral (char *message) {
    printf ("Error: %s\n", message);
    exit (1);
}

// Shell memory functions
void frameMemInit () {
    globalLRUCounter = 0;
    for (int i = 0; i < NUM_FRAMES; i++) {
        for (int j = 0; j < FRAME_SIZE; j++) {
            frameStore[i].code[j] = NULL;
        }
        // reset file name
        frameStore[i].filename = NULL;
        // frame not in use
        frameStore[i].validBit = 0;
        // frame never accessed
        frameStore[i].lastAccessTime = -1;
        // frame has no (Page) references initially
        frameStore[i].refCapacity = 0;
        frameStore[i].refCount = 0;
        frameStore[i].references = NULL;
    }
}

// called by pageTableInit to see if another program has loaded the same code into a frame already
int findExistingFrameForPage (char *filename, int pageNum) {
    if (filename == NULL) {
        errorGeneral ("findExistingFrameForPage was passed a NULL filename\n");
    }

    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frameStore[i].validBit &&
            frameStore[i].filename != NULL &&
            strcmp (frameStore[i].filename, filename) == 0 &&
            frameStore[i].pageNum == pageNum) {
            frameStore[i].lastAccessTime = globalLRUCounter++;
            return i;
        }
    }
    // Not found 
    return -1;                  
}

int findFirstFrameHole () {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (!frameStore[i].validBit) {
            return i;
        }
    }
    // no free frames, will need to evict
    return -1; 
}

void addReferenceToFrame (int frameNum, struct Page *pagePtr) {
    struct Frame *frame = &frameStore[frameNum];

    // Allocate or grow the references array if needed
    if (frame->refCount >= frame->refCapacity) {
        int newCapacity =
            (frame->refCapacity == 0) ? 2 : frame->refCapacity * 2;
        struct Page **newRefs =
            realloc (frame->references, sizeof (struct Page *) * newCapacity);
        if (!newRefs) {
            errorGeneral ("Could not realloc new frame references array.");
        }
        frame->references = newRefs;
        frame->refCapacity = newCapacity;
    }

    frameStore[frameNum].lastAccessTime = globalLRUCounter++;
    // Add the page pointer to the references array
    frame->references[frame->refCount] = pagePtr;
    frame->refCount++;
}

int findLRUFrame () {
    // dummy values to start
    int LRUFrameIndex = 0;
    int minLRUValue = frameStore[0].lastAccessTime;
    for (int i = 1; i < FRAME_MEM_SIZE / FRAME_SIZE; i++) {
        if (frameStore[i].lastAccessTime < minLRUValue) {
            LRUFrameIndex = i;
            minLRUValue = frameStore[i].lastAccessTime;
        }
    }
    return LRUFrameIndex;
}

// returns index from which frame was evicted
int evictFrame () {
    int frameNum = findLRUFrame ();
    //printf("evicting frame %d\n", frameNum);
    struct Frame frame = frameStore[frameNum];

    printf ("Page fault! Victim page contents:\n \n");
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (frame.code[i] != NULL) {
            printf ("%s", frame.code[i]);
        }
    }
    printf ("\nEnd of victim page contents.\n");

    return frameNum;
}

// returns index of frameStore at which new frame was added
int addFrame (struct PCB *pcb, int pageNum) {

    char *pageCode[FRAME_SIZE] = { NULL };
    char line[MAX_LINE_LENGTH];
    int linesRead = 0;
    int fileLine = 0;
    fseek (pcb->scriptFile, 0, SEEK_SET);
    while (fileLine < pageNum * FRAME_SIZE
           && fgets (line, MAX_LINE_LENGTH - 1, pcb->scriptFile) != NULL) {
        fileLine++;
    }
    // Now read up to 3 lines into the new page
    while (fgets (line, MAX_LINE_LENGTH - 1, pcb->scriptFile) != NULL
           && linesRead < FRAME_SIZE) {
        pageCode[linesRead] = strdup (line);

        if (!pageCode[linesRead]) {
            fclose (pcb->scriptFile);
            errorGeneral ("addFrame could not allocate memory for code line");
        }
        linesRead++;
    }

    // If no lines were read, there is nothing to store
    if (linesRead == 0) {
        return -1;              // signal: no more lines left to read
    }

    int frameNum = findFirstFrameHole ();

    if (frameNum == -1) {
        frameNum = evictFrame ();
    }

    // Store lines in frame
    for (int i = 0; i < FRAME_SIZE; i++) {
        frameStore[frameNum].code[i] = pageCode[i];
    }

    if (pcb->scriptName == NULL) {
        errorGeneral ("addFrame: pcb->scriptName is NULL");
    }

    frameStore[frameNum].filename = strdup (pcb->scriptName);
    if (!frameStore[frameNum].filename) {
        errorGeneral ("addFrame could not allocate memory for filename\n");
    }

    frameStore[frameNum].pageNum = pageNum;
    frameStore[frameNum].validBit = 1;
    frameStore[frameNum].lastAccessTime = globalLRUCounter++;
    frameStore[frameNum].refCount = 0;
    frameStore[frameNum].refCapacity = 0;
    frameStore[frameNum].references = NULL;
    return frameNum;
}

// returns error code
int addPage (struct PCB *pcb, int pageNum, bool suppressPrint) {
    struct PageTable *pageTable = pcb->pageTable;
    // grow page table if not large enough
    if (pageTable->numPages >= pageTable->capacity) {
        int oldCap = pageTable->capacity;
        pageTable->capacity *= 2;
        struct Page *newPages =
            realloc (pageTable->pages,
                     sizeof (struct Page) * (pageTable->capacity));
        if (!newPages) {
            errorGeneral ("Could not realloc page table");
        }
        pageTable->pages = newPages;

        for (int i = oldCap; i < pageTable->capacity; i++) {
            pageTable->pages[i].frameNum = -1;
            pageTable->pages[i].validBit = 0;
        }
    }
    // Look for existing frame w this code
    int frameNum = findExistingFrameForPage (pcb->scriptName, pageNum);
    if (frameNum == -1) {
        int freeFrameNum = findFirstFrameHole ();
        bool needToEvict = (freeFrameNum == -1);

        // no existing frame, have to load code in from file
        frameNum = addFrame (pcb, pageNum);
        if (frameNum == -1) {
            // no more code to add, file is done executing
            return -1;
        }
        if (!needToEvict && !suppressPrint) {
            printf ("Page fault!\n");
        }
    }
    pageTable->numPages++;
    pageTable->pages[pageNum].frameNum = frameNum;
    pageTable->pages[pageNum].validBit = 1;
    addReferenceToFrame (pageTable->pages[pageNum].frameNum,
                         &pageTable->pages[pageNum]);
    return 0;
}

char *getLineForProcess (struct PCB *pcb) {
    int pageNum = pcb->curLine / FRAME_SIZE;
    int offset = pcb->curLine % FRAME_SIZE;

    if (pageNum >= pcb->pageTable->numPages
        || !pcb->pageTable->pages[pageNum].validBit) {
        int errorCode = addPage (pcb, pcb->curLine / FRAME_SIZE, false);
        if (errorCode == -1) {
            // signal no more process code to load
            return "__DONE__";
        }
        // signal page fault with successful code loading
        return "__PAGE_FAULT__";
    }
    struct Page page = pcb->pageTable->pages[pageNum];
    int frameNum = page.frameNum;
    frameStore[frameNum].lastAccessTime = globalLRUCounter++;   // LRU update
    return frameStore[frameNum].code[offset];
}

struct PageTable *initPageTable (struct PCB *pcb) {
    // set aside memory for page table
    struct PageTable *pageTable = malloc (sizeof (struct PageTable));
    if (!pageTable) {
        fclose (pcb->scriptFile);
        errorGeneral ("Could not allocate page table");
    }
    // initially only loading 2 pages per process
    pcb->pageTable = pageTable;
    pageTable->capacity = 2;
    pageTable->numPages = 0;

    pageTable->pages = malloc (sizeof (struct Page) * pageTable->capacity);
    if (!pageTable->pages) {
        fclose (pcb->scriptFile);
        free (pageTable);
        errorGeneral ("Could not allocate Page array inside PageTable");
    }
    // initialize page table entries to be empty
    for (int i = 0; i < pageTable->capacity; i++) {
        pageTable->pages[i].frameNum = -1;
        pageTable->pages[i].validBit = 0;
    }
    if (addPage (pcb, 0, true) == -1) {
        return pageTable;
    }
    addPage (pcb, 1, true);
    return pageTable;
}

int freeProcess (struct PCB *pcb) {
    if (pcb == NULL)
        return -1;

    // Free all page table memory
    if (pcb->pageTable) {
        // Free the dynamically allocated array of Page structs
        if (pcb->pageTable->pages) {
            free (pcb->pageTable->pages);
            pcb->pageTable->pages = NULL;
        }
        // Free the PageTable struct itself
        free (pcb->pageTable);
        pcb->pageTable = NULL;
    }
    // Free the dynamically allocated script name (if allocated)
    if (pcb->scriptName) {
        free (pcb->scriptName);
        pcb->scriptName = NULL;
    }
    fclose (pcb->scriptFile);

    // Free the PCB itself
    free (pcb);
    pcb = NULL;

    return 0;
}
