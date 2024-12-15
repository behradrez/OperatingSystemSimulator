#include <stdio.h>
#include <stdlib.h>
#include "shellmemory.h"
#include "shell.h"
#include "processmanagement.h"

int PROCESS_COUNT = 0;

// Head of linkedList pcb's
struct pcb* readyQueue = NULL;

int clearPCB(struct pcb* pcb_in){
    // Foreground PCB does not require memory clearing
    if(pcb_in->isForeground == 'N'){
        return 0;
    }
    char* instructionKey = malloc(24*sizeof(char));
    //Fetch instruction key value and reset its associated value in shell memory
    for(int i=0; i<pcb_in->numCommands; i++){
        sprintf(instructionKey, "proccess_%d_command_%d",pcb_in->pid,i);
        //Custom shell memory clearing function
        mem_clear_value(instructionKey);
    }
    free(instructionKey);
    return 0;
}

// Adds to global linked list readyQueue 
void add_to_queue(struct pcb* newPCB){
    if(readyQueue == NULL){
        readyQueue = newPCB;
        return;
    }
    struct pcb* currPCB = readyQueue;
    // Fetch tail of linked list
    while(currPCB->nextPCB != NULL){
        currPCB = currPCB->nextPCB;
    }
    currPCB->nextPCB = newPCB;
    return;
}

struct pcb* createPCB(int numLines, int numCommands){
    struct pcb* newPCB = (struct pcb*) malloc(sizeof(struct pcb));
    // Initialize all fields to a default value
    newPCB->numLines = numLines;
    newPCB->jobLengthScore = numLines;
    newPCB->numCommands = numCommands;
    newPCB->currentInstructionLine = 0;
    
    // No longer unique, may get re-set if PCB is created from existing PCB
    newPCB->pid = PROCESS_COUNT;
    
    // This is the new unique PCB identifier.
    newPCB->actualId = PROCESS_COUNT;

    newPCB->nextPCB = NULL;
    newPCB->isForeground = 'N';

    // -1 indicates invalid/unloaded pagetable value 
    newPCB->pagetable = malloc(40*sizeof(int));
    for(int i=0;i<40;i++){
        newPCB->pagetable[i] = -1;
    }

    // Fields needed for execution for non-RR execution policies
    char* firstKey = malloc(24*sizeof(char));
    sprintf(firstKey, "process_%d_command_%d",newPCB->pid,0);
    newPCB->firstInstructionKey = firstKey;
    //Increment number of created processes to keep unique PID for following PCBs
    PROCESS_COUNT++;
    return newPCB;
}