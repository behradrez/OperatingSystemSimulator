#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ftw.h>
#include "shellmemory.h"
#include <time.h>

#ifndef varmemsize
#define varmemsize 1000
#endif

#ifndef framesize
#define framesize 1000
#endif

struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[varmemsize];
char* framememory[framesize];

int frameRecency[framesize/3];
int frameTimeVariable = 0;

// Helper functions
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i]) matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else return 0;
}

// Used to differentiate which part of shell memory to use
int isFrameKey(char *key){
    if(strlen(key)<8) return 0;
    return (strncmp(key, "process_",8)==0) ? 1 : 0;
}

// Memory functions
void evictFrame(int frameNumber){
    printf("Page fault! Victim page contents:\n\n");
    
    // Print the max 3 instructions that get evicted
    for(int i=0;i<3;i++){
        // Avoid indexing past the framesize
        if(frameNumber*3+i >= framesize) break;
        
        char* content = framememory[frameNumber*3+i];
        if(strcmp(content,"NULL")!=0){
            content[strcspn(content,"\n")] = 0;
            printf("%s\n",content);
        }
        framememory[frameNumber*3+i] = "none";
    }
    printf("\nEnd of victim page contents.\n");
}

void evictLRU(){
    // Min-finding algorithm to find least recently accessed framed to evict
    int LRUFrame = 0;
    for(int i=0;i<framesize/3;i++){
        if(frameRecency[i]<frameRecency[LRUFrame]) LRUFrame = i;
    }
    evictFrame(LRUFrame);
}

int addToFrameStore(char* instructions[], int isPageFault){
    // Takes an array of 3 strings and places them in the
    // first available frame. "none" ensures the availability
    // of 3 slots. If no slots available, will empty one via 
    // a policy and calls itself again. Returns the frame number
    // where the given instruction are stored.
    
    for (int i = 0; i<framesize; i++){
        if(strcmp(framememory[i],"none")==0){
            if(i+2>=framesize) break;

            for(int j=0; j<3; j++){
                framememory[i+j] = instructions[j];
            }
            if(isPageFault) printf("Page fault!\n");
            // Divide by 3 since each frame takes 3 indices
            int frameNumber = i/3;
            frameRecency[frameNumber] = frameTimeVariable++;
            return frameNumber; 
        }
    }
    evictLRU(); 
    addToFrameStore(instructions,0);
}


int copyToBackingStore(char* fileName){
    // This method takes a fileName from the default directory as input
    // And copies its content into a file of the same name in a designated backing store
    
    FILE *source = fopen(fileName, "r");
    char* destName = malloc(strlen(BACKING_FILE)+strlen(fileName)+2);
    
    // Set file destination in backing_store
    snprintf(destName, strlen(BACKING_FILE)+strlen(fileName)+2, "%s/%s", BACKING_FILE, fileName); 
    FILE *dest = fopen(destName,"w");
    
    char buffer[1024];
    int linesCopied = 0;
    
    while(fgets(buffer,sizeof(buffer), source)){
        fputs(buffer,dest);
        linesCopied++;
    }

    free(destName);
    fclose(dest);
    fclose(source);

    // Want to return # of lines copied for PCB instruction tracking purposes
    return linesCopied;
}

int addFrameFromBackingStore(char* fileName, int lineIndex){
    // This method takes a fileName and a lineIndex integer as input
    // The fileName is the name of the process, and lineIndex is the
    // first line of the page that is requested. The function takes
    // the first 3 lines starting at the lineIndex and loads them into
    // the frame store. Returns the frame number the new page is stored in.
    
    // Any case where a request to add a frame starting at lines 6 or higher is made
    // implies the requested frame is not of the first 2 frames that get loaded when PCB is first created
    // Meaning a page fault has happened for this request to have been made.
    int isPageFault = lineIndex>5 ? 1 : 0;

    char* instructions[3] = {"NULL", "NULL", "NULL"};
    char* sourceName = malloc(strlen(BACKING_FILE)+strlen(fileName)+2);
    snprintf(sourceName, strlen(BACKING_FILE)+strlen(fileName)+2, "%s/%s", BACKING_FILE, fileName);
    
    FILE *sourceFile = fopen(sourceName,"r");
    if (!sourceFile){
        free(sourceName);
        perror("Error opening file");
        return -1;
    }
    free(sourceName);
    
    char buffer[1024];
    int currLine = 0;
    int instructionIndex = 0;

    // Reads 3 instructions from backing store and save them
    while(fgets(buffer,sizeof(buffer),sourceFile) && instructionIndex < 3){
        if(currLine >= lineIndex){
            instructions[instructionIndex] = malloc(strlen(buffer)+1);
            strcpy(instructions[instructionIndex],buffer);
            instructionIndex++;
        }
        currLine++;
    }

    // If the first instruction is NULL, then there were no new lines to be read
    // This will happen due to the lineIndex being a larger value than the number of
    // lines in the file. Returning -1 indicates there is nothing left to add to memory
    if(strcmp(instructions[0],"NULL")==0){
        return -1;
    }
    fclose(sourceFile);

    return addToFrameStore(instructions,isPageFault);
}

char* getFromFrameStore(int frameNumber, int offset){
    // The virtual "time" variable serves to track the how recently a frame was accessed.
    // Larger value indicates more recent use, so set curr time in recency tracker and
    // increment it.
    frameRecency[frameNumber] = frameTimeVariable;
    frameTimeVariable++;
    
    // The input parser that executes the instruction held in the below
    // line will split the pointer's string value with strtok for one-liners,
    // modifying what is kept in the framestore. Therefore, its necessary to return a
    // copy of the line, and not the pointer to the framestore memory element.
    char* lineFromFrameStore = malloc(strlen(framememory[frameNumber*3+offset]));
    strcpy(lineFromFrameStore, framememory[frameNumber*3+offset]);
    
    return lineFromFrameStore;
}

void mem_init(){
    printf("Frame Store Size = %d; Variable Store Size = %d\n", framesize, varmemsize);
    
    // Helper library that checks if a directory exists
    struct stat st = {0};

    // Since backing store is only deleted on quit, it may still exist, so delete it
    if (stat(BACKING_FILE, &st) != -1){
        remove_dir(BACKING_FILE);
    }
    
    // Creates the backing store with the necessary permissions
    mkdir(BACKING_FILE, 0700);
    
    int i;
    // Initializing var and frame memories to default values indicating empty slots
    for (i = 0; i < varmemsize; i++){		
        shellmemory[i].var   = "none";
        shellmemory[i].value = "none";
    }
    for (i = 0; i < framesize; i++){		
        framememory[i] = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < varmemsize; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < varmemsize; i++){
        if (strcmp(shellmemory[i].var, "none") == 0){
            shellmemory[i].var   = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    return;
}

//Sets key and value in memory back to their default values
int mem_clear_value(char *var_in){
    int i;
    for(i=0; i<varmemsize; i++){
        if (strcmp(shellmemory[i].var,var_in) == 0){
            shellmemory[i].var = "none";
            shellmemory[i].value = "none";
            return 0;
        }
    }
    return 1;
}

//get value based on input key
char *mem_get_value(char *var_in) {
    int i;

    for (i = 0; i < varmemsize; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            return strdup(shellmemory[i].value);
        } 
    }
    return "Variable does not exist in memory";
}

int rm(const char *filepath, const struct stat *statbuffer, int flag, struct FTW *ftwbuffer){
    int removevalue = remove(filepath);//remove the file at filepath

    if(removevalue) perror(filepath);//if remove fails return the filename
    return removevalue;//otherwise return 0
}

int remove_dir(char *dirname){
    return nftw(dirname, rm, 64, FTW_DEPTH | FTW_PHYS);//depth-first traversal of the directory, calling rm on everything
}
