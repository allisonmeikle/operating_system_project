#include "scheduler.h"
#include "shellmemory.h"
#include "shell.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct ReadyQueue {
    struct PCB *head;
    struct PCB *tail;
    int length;
};

// ready queue and next pid should only be accessible within this class
static struct ReadyQueue readyQueue = { NULL, NULL, 0 };

static pid_t next_pid = 0;

// Define function pointers for enqueue operation
void (*enqueueFunc) (struct PCB * pcb) = NULL;

void initReadyQueue () {
    readyQueue.head = NULL;
    readyQueue.tail = NULL;
    readyQueue.length = 0;
    next_pid = 0;
}

static int errorGeneral (char *message) {
    printf ("Bad command: %s\n", message);
    exit (1);
}

static void enqueueRR (struct PCB *pcb) {
    if (readyQueue.tail == NULL) {
        readyQueue.head = pcb;
        readyQueue.tail = pcb;
        pcb->next = NULL;
    } else {
        readyQueue.tail->next = pcb;
        readyQueue.tail = pcb;
        pcb->next = NULL;
    }
    readyQueue.length++;
}

// Initialize the scheduling policy
static int initSchedulingPolicy (const char *policy) {
    if (strcmp (policy, "RR") == 0) {
        enqueueFunc = enqueueRR;
    } else {
        return errorGeneral ("initSchedulingPolicy given an invalid policy");
    }
    return 0;
}

// Dequeue a PCB from the ready queue
static struct PCB *dequeue () {
    if (readyQueue.head == NULL) {
        errorGeneral ("dequeue expected a nonempty queue");
        return NULL;
    }
    struct PCB *pcb = readyQueue.head;
    readyQueue.head = pcb->next;
    if (readyQueue.head == NULL) {
        readyQueue.tail = NULL;
    }
    pcb->next = NULL;
    readyQueue.length--;
    return pcb;
}

static pid_t getNextPID () {
    return next_pid++;
}

int addProcess (char *filename, const char *policy) {
    int errCode = initSchedulingPolicy (policy);
    if (errCode != 0) {
        return errCode;
    }

    struct PCB *pcb = (struct PCB *) malloc (sizeof (struct PCB));
    if (pcb == NULL) {
        errorGeneral ("addProcess could not allocate memory for PCB");
    }
    pcb->scriptName = strdup (filename);
    if (!pcb->scriptName) {
        freeProcess(pcb);
        errorGeneral ("addProcess could not allocate memory for script name");
    }
    FILE *file = fopen (pcb->scriptName, "rt");
    if (file == NULL) {
        char errorMessage[128];
        snprintf (errorMessage, sizeof (errorMessage),
                  "addProcess could not open file with name: %s",
                  pcb->scriptName);
        errorGeneral (errorMessage);
    }
    pcb->scriptFile = file;
    pcb->pid = getNextPID ();
    pcb->curLine = 0;
    pcb->next = NULL;
    pcb->pageTable = initPageTable (pcb);
    pcb->curLine = 0;
    enqueueFunc (pcb);
    return 0;
}

int runScheduler (char *policy) {
    // 0 means non-preemptive, 1 means preemptive
    int preEmp = 0;
    int timeSlice = 0;
    if (strcmp (policy, "RR") == 0) {
        enqueueFunc = enqueueRR;
        preEmp = 1;
        timeSlice = 2;
    } else {
        return errorGeneral ("runScheduler was given an invalid policy");
    }

    struct PCB *curPCB;
    int instructionCount = 0;

    while (readyQueue.length > 0) {
        curPCB = dequeue ();
        if (curPCB == NULL) {
            continue;
        }
        instructionCount = 0;
        while (1) {
            char *line = getLineForProcess (curPCB);

            if (line == NULL || strcmp (line, "__DONE__") == 0) {
                freeProcess (curPCB);
                break;
            } else if (strcmp (line, "__PAGE_FAULT__") == 0) {         
                enqueueFunc (curPCB);
                break;
            }

            int errCode = parseInput (line);
            curPCB->curLine++;
            instructionCount++;

            if (errCode != 0) {
                break;
            }

            if (preEmp && (instructionCount >= timeSlice)) {
                // Preempt the process and enqueue it back to the ready queue
                enqueueFunc (curPCB);
                break;
            }
        }
    }
    return 0;
}
