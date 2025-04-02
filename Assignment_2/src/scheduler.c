#include "scheduler.h"
#include "shellmemory.h"
#include "shell.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>

struct PCB {
    pid_t pid;
    struct ScriptFile *script;
    int curLine;
    struct PCB *next;
    int jobLengthScore;
};

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
    return 1;
}

static void enqueueFCFS (struct PCB *pcb) {
    pcb->next = NULL;
    if (readyQueue.tail == NULL) {
        readyQueue.head = pcb;
        readyQueue.tail = pcb;
    } else {
        readyQueue.tail->next = pcb;
        readyQueue.tail = pcb;
    }
    readyQueue.length++;
}

static void enqueueSJF (struct PCB *pcb) {
    if (readyQueue.head == NULL) {
        readyQueue.head = pcb;
        readyQueue.tail = pcb;
        pcb->next = NULL;
    } else {
        struct PCB *current = readyQueue.head;
        // if the new PCB should be the head
        if (current->script->numLines > pcb->script->numLines) {
            pcb->next = current;
            readyQueue.head = pcb;
        } else {                // not the head, loop through to find it's place
            while (current->next != NULL
                   && current->next->script->numLines <=
                   pcb->script->numLines) {
                current = current->next;
            }
            pcb->next = current->next;
            current->next = pcb;
        }
    }
    readyQueue.length++;
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
static void firstEnqueueAging (struct PCB *pcb) {
    // insert current pcb back into proper slot
    if (readyQueue.head == NULL) {
        readyQueue.head = pcb;
        readyQueue.tail = pcb;
        pcb->next = NULL;
    } else {
        struct PCB *current = readyQueue.head;
        // if the new PCB should be the head
        if (pcb->jobLengthScore < current->jobLengthScore) {
            pcb->next = current;
            readyQueue.head = pcb;
        } else {                // not the head, loop through to find it's place
            while (current->next != NULL
                   && pcb->jobLengthScore >= current->next->jobLengthScore) {
                current = current->next;
            }
            pcb->next = current->next;
            current->next = pcb;
        }
    }
    readyQueue.length++;
}

// Enqueue a PCB to the ready queue
static void enqueueAging (struct PCB *pcb) {
    struct PCB *current = readyQueue.head;
    while (current != NULL) {
        if (current->jobLengthScore > 0) {
            current->jobLengthScore--;
        }
        current = current->next;
    }
    // insert current pcb back into proper slot
    if (readyQueue.head == NULL) {
        readyQueue.head = pcb;
        readyQueue.tail = pcb;
        pcb->next = NULL;
    } else {
        struct PCB *current = readyQueue.head;
        // if the new PCB should be the head
        if (pcb->jobLengthScore <= current->jobLengthScore) {
            pcb->next = current;
            readyQueue.head = pcb;
        } else {                // not the head, loop through to find it's place
            while (current->next != NULL
                   && pcb->jobLengthScore > current->jobLengthScore) {
                current = current->next;
            }
            pcb->next = current->next;
            current->next = pcb;
        }
    }
    readyQueue.length++;
}

// Initialize the scheduling policy
static int initSchedulingPolicy (const char *policy) {
    //printf("policy: %s\n", policy);
    if (strcmp (policy, "FCFS") == 0) {
        enqueueFunc = enqueueFCFS;
    } else if (strcmp (policy, "SJF") == 0) {
        enqueueFunc = enqueueSJF;
    } else if (strcmp (policy, "RR") == 0 || strcmp (policy, "RR30") == 0) {
        enqueueFunc = enqueueRR;
    } else if (strcmp (policy, "AGING") == 0) {
        enqueueFunc = firstEnqueueAging;
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

int addProcess (const char *filename, const char *policy) {
    int errCode = initSchedulingPolicy (policy);
    if (errCode != 0) {
        return errCode;
    }

    struct ScriptFile *script = getScript (filename);
    if (script == NULL) {
        return 1;
    }

    struct PCB *pcb = (struct PCB *) malloc (sizeof (struct PCB));
    if (pcb == NULL) {
        return errorGeneral ("addProcess could not allocate memory for PCB");
    }

    pcb->pid = getNextPID ();
    pcb->script = script;
    pcb->curLine = 0;
    pcb->jobLengthScore = script->numLines;
    pcb->next = NULL;


    if (strcmp (filename, batchScript) == 0) {
        struct PCB *oldHead = readyQueue.head;
        readyQueue.head = pcb;
        pcb->next = oldHead;
        readyQueue.length++;
    } else {
        enqueueFunc (pcb);
    }

    return 0;
}

int runScheduler (char *policy) {
    // 0 means non-preemptive, 1 means preemptive
    int preEmp = 0;
    int timeSlice = 0;
    if (strcmp (policy, "FCFS") == 0) {
        enqueueFunc = enqueueFCFS;
        preEmp = 0;
    } else if (strcmp (policy, "SJF") == 0) {
        enqueueFunc = enqueueSJF;
        preEmp = 0;
    } else if (strcmp (policy, "RR") == 0) {
        enqueueFunc = enqueueRR;
        preEmp = 1;
        timeSlice = 2;
    } else if (strcmp (policy, "RR30") == 0) {
        enqueueFunc = enqueueRR;
        preEmp = 1;
        timeSlice = 30;
    } else if (strcmp (policy, "AGING") == 0) {
        enqueueFunc = enqueueAging;
        preEmp = 1;
        timeSlice = 1;
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
        // since numLines is a count and curLine is an index, terminate one early
        while (curPCB->curLine < curPCB->script->numLines) {
            int errCode = parseInput (curPCB->script->lines[curPCB->curLine]);
            curPCB->curLine++;
            instructionCount++;

            if (errCode != 0) {
                break;
            }

            if (preEmp && (instructionCount >= timeSlice)
                && (curPCB->curLine < curPCB->script->numLines)) {
                // Preempt the process and enqueue it back to the ready queue
                enqueueFunc (curPCB);
                break;
            }
        }
        if (curPCB != NULL && curPCB->curLine >= curPCB->script->numLines) {
            // Process has finished execution
            freeScript (curPCB->script->filename);
            free (curPCB);
            curPCB = NULL;
        }
    }
    return 0;
}
