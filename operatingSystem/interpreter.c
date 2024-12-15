#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "shellmemory.h"
#include "shell.h"
#include "processmanagement.h"
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>

//Global variables will allow for easier access in different functions
pthread_mutex_t lock;
pthread_t thread1,thread2;

int thread1Joined = 0;
int thread2Joined = 0;

int quitSignal = 0;

int MAX_ARGS_SIZE = 7;

int IS_VERBOSE = 0;


int str_isalnum(char* string){
    int length = strlen(string);
    int i=0;
    while(i<strlen(string)){
        if(! isalnum(string[i]) && string[i] != '_'){
            if(IS_VERBOSE) printf("Invalid char: %c",string[i]);
            return 1;
        }
        i++;
    }
    return 0;
}

int badcommand(){
    printf("Unknown Command\n");
    return 1;
}

int customBadCommand(char* message){
    if(message != NULL){
        printf("%s\n",message);
        return 1;
    }else{
        printf("Null input");
        return badcommand();
    }
}
// For run command only
int badcommandFileDoesNotExist(){
    printf("Bad command: File not found\n");
    return 3;
}

int help();
int quit();
int set(char* var, char* value);
int print(char* var);
int run(char* script);
int badcommandFileDoesNotExist();
int echo(char* token);

int my_ls();
int my_mkdir();
int my_touch();
int my_cd(char* location);
int exec(char *fileNames[], char* policy, int numFiles, int isBackground, int MT);
int executeForegroundPCB(struct pcb* foregroundPCB, int numExecutions);
//Testing stuff
int my_pwd();
int join_threads();

// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){
    int i;
    if (args_size < 1){
        return customBadCommand("No command input");

    }else if (args_size > MAX_ARGS_SIZE){
        return customBadCommand("Bad command: Too many tokens");
    }

    for (i = 0; i < args_size; i++){
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    
    if (strcmp(command_args[0], "help") == 0){
        if (args_size != 1) return badcommand();
        return help();
    
    } else if(strcmp(command_args[0],"verbose") == 0){
        IS_VERBOSE = (IS_VERBOSE+1)%2;
        if(IS_VERBOSE) printf("Verbose printing: ON\n");
        else printf("Verbose printing: OFF\n");
        return 0;

    }else if (strcmp(command_args[0], "quit") == 0){
        if (args_size != 1) return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0){
        if (args_size < 3) return badcommand();
        if (args_size > 7) return customBadCommand("Bad command: Too many tokens");
        char* value = command_args[2];

        for (int token = 3; token < args_size; token++){
            if(str_isalnum(command_args[token])==1){
                return customBadCommand("Bad command: Invalid non-alnum character");
            }
            strcat(value, " ");
            strcat(value, command_args[token]);
        }
        return set(command_args[1], command_args[2]);
    
    } else if (strcmp(command_args[0], "print") == 0){
        if (args_size != 2) return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0){
        if (args_size != 2) return badcommand();
        char* fileNames[1];
        fileNames[0] = command_args[1];
        return exec(fileNames,"RR",1,0,0);

    }else if(strcmp(command_args[0],"my_ls") == 0){
        if(args_size != 1) return badcommand();
        return my_ls();

    }else if(strcmp(command_args[0],"my_mkdir") == 0){
        if(args_size != 2) return badcommand();
        return my_mkdir(command_args[1]);

    }else if(strcmp(command_args[0],"echo") == 0){
        if(args_size != 2) return badcommand();
        return echo(command_args[1]);

    }else if(strcmp(command_args[0],"my_cd") == 0){
        if(args_size != 2) return badcommand();
        return my_cd(command_args[1]);

    }else if(strcmp(command_args[0],"my_pwd") == 0){
        return my_pwd();

    }else if(strcmp(command_args[0],"my_touch") == 0) {
        if(args_size != 2) return badcommand();
        return my_touch(command_args[1]);

    }else if(strcmp(command_args[0],"exec") == 0){
        if(args_size < 3 || args_size > 7) return badcommand();
        
        // By default, numFiles is number of arguments minus "exec" & POLICY
        int numFiles = args_size - 2;
        
        int isBackground = 0; // False by default
        int isMT = 0;
        if(strcmp(command_args[args_size-1],"#") == 0 || strcmp(command_args[args_size-2],"#")==0){
            //if the last or before-last argument is '#', then we are executing in the background
            // & one of the arguments is not a file name
            numFiles--;
            isBackground = 1;
        }

        if(strcmp(command_args[args_size-1],"MT")==0){
            // One of the arguments is MT, so not a file name
            numFiles--;
            isMT = 1;
        }

        char* fileNames[numFiles]; 
        for(int i=1;i<numFiles+1;i++){
            fileNames[i-1] = command_args[i];
        }
        return exec(fileNames, command_args[1+numFiles], numFiles, isBackground,isMT);
    }
    else return customBadCommand("My bad command");
}

int my_touch(char* filename){
    if(str_isalnum(filename) == 1){
        return customBadCommand("Bad command: my_touch");
    }
    FILE *fptr;
    fptr = fopen(filename, "w"); //opening a file that doesn't exist creates an empty file
    fclose(fptr);
}

int my_cd(char* location){
    if(str_isalnum(location) == 1){
        return customBadCommand("Bad command (my_cd): string is not alphanumeric");
    }
    if(chdir(location) != 0){
        return customBadCommand("Bad command (my_cd): error changing directory");
    }
    return 0;
}

int my_pwd(){
    char buffer[1024];

    getcwd(buffer, sizeof(buffer));
    printf("%s\n",buffer);
    return 0;
}

int my_ls(){
    struct dirent **namelist;
    int n;
    n = scandir(".",&namelist, NULL, alphasort);

    if(n == -1){//negative number of entries indicates an error
        printf("Error\n");
        return 1;
    }

    int i=0;
    while(i<n){
        if(namelist[i]->d_name[0] != '.'){
            printf("%s\n",namelist[i]->d_name);
        }
        free(namelist[i]);
        i++;
    }  
    free(namelist);
    return 0;
}

int my_mkdir(char* dirname){
    char* directory_name = dirname;

    if((char) dirname[0] == '$'){
        char* var_name = malloc(strlen(dirname));
        strncpy(var_name,dirname+1,strlen(dirname));
        directory_name = mem_get_value(var_name);
    }

    if(strcmp(directory_name,"Variable does not exist in memory") == 0 || str_isalnum(directory_name)==1){
        return customBadCommand("Bad command: my_mkdir");
    }

    int status;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;

    status = mkdir(directory_name,mode);
    return 0;
}

int echo(char* token) {
    if(token == NULL){
        return 1;
    }//error if the string is empty

    if ((char) token[0] == '$'){
        char* var_name = malloc(strlen(token));
        strncpy(var_name, token+1 ,strlen(token));
        if(str_isalnum(var_name)==1){
            return customBadCommand("Bad Command: Invalid non-alnum character");
        }
        
        char* mem_val = mem_get_value(var_name);
        
        if(strcmp(mem_val, "Variable does not exist in memory") == 0){
            printf("\n"); //print nothing if the file does not exist
            return 0;
        }else{
            printf("%s\n",mem_val); //else print the contents of the file
            return 0;
        }
    }else{
        if(str_isalnum(token)==1){
            return customBadCommand("Bad Command: Invalid non-alphanumeric character");
        }
        printf("%s\n",token);//print the filename
        return 0;
    }
}

int help() {

    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    
    // If child thread calling quit, set quitSignal to 1
    // So child thread doesn't exit process before others finish
    // After threads complete, main thread will quit if quitSignal is set
    if(readyQueue!=NULL){
        quitSignal = 1;
        pthread_exit(NULL);
        return 0;
    }
    remove_dir(BACKING_FILE);   //delete the backing store file

    exit(0);
}

int set(char *var, char *value) {
    char *link = "=";
    mem_set_value(var, value);  //call the helper function
    return 0;
}

int print(char *var) {
    printf("%s\n", mem_get_value(var)); //print the contents of the variable
    return 0;
}

int run(char *script) {
    //struct pcb* ready_queue = NULL;
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script,"rt");

    if (p == NULL){
        return badcommandFileDoesNotExist();
    }

    // Save every line in memory under process_X_command_X
    // So they can be retrieved by pcb && cannot be overriden by user's set command 
    int numLines = 0;
    int numCommands = 0;
    char* key = malloc(24*sizeof(char));
    while(1){
        if(feof(p)){
            break;
        }
        fgets(line, MAX_USER_INPUT-1,p);
        char* singleInput = strtok(line,";");
        while(singleInput != NULL){
            sprintf(key, "process_%d_command_%d",PROCESS_COUNT,numCommands);
            mem_set_value(key,singleInput);
            numCommands++;
            singleInput = strtok(NULL,";");
        }
        numLines++;
    }
    fclose(p);
    struct pcb* newPCB = createPCB(numLines,numCommands);
    PROCESS_COUNT++;
    add_to_queue(newPCB);


    // Free mallocated memory after all instructions fetched
    // To avoid re-malloc'ing for every instruction
    free(key);

    char* currentInstructionKey = malloc(24*sizeof(char));
    while(readyQueue != NULL){
        for(int i=0; i<readyQueue->numCommands; i++){
            sprintf(currentInstructionKey,"process_%d_command_%d",readyQueue->pid,i);
            char* curr_command = mem_get_value(currentInstructionKey);
            curr_command[strcspn(curr_command,"\n")] = 0;
            if(IS_VERBOSE) printf("\tExecuting command: %s\n",curr_command);
            parseInput(curr_command);        
        }
        struct pcb* nextPCB = readyQueue->nextPCB;
        clearPCB(readyQueue);
        free(readyQueue);
        readyQueue = nextPCB;
    }
    free(currentInstructionKey);
}

struct pcb* loadScript(char* script){
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script,"rt");
    if (p == NULL){
        badcommandFileDoesNotExist();
        return NULL;
    }

    // Save every line in memory under process_X_command_X 
    int numLines = 0;
    int numCommands = 0;
    char* key = malloc(24*sizeof(char));
    while(1){
        if(feof(p)){
            break;
        }
        fgets(line, MAX_USER_INPUT-1,p);
        char* singleInput = strtok(line,";");
        //Uses mem_set_value to store in instructions in memory 
        // with key that user cannot use with set command
        while(singleInput != NULL){
            sprintf(key, "process_%d_command_%d",PROCESS_COUNT,numCommands);
            mem_set_value(key,singleInput);
            numCommands++;
            singleInput = strtok(NULL,";");
        }
        numLines++;
    }
    fclose(p);
    free(key);
    // Create and initialize PCB with default and indicated values
    struct pcb* newPCB = createPCB(numLines,numCommands);
    return newPCB;
}

// This function will create a PCB from another existing PCB
// The purpose of this is to achieve instruction sharing to avoid filling frameMemory
// with copies of the same instructions from the same script.
struct pcb* loadScriptFromPCB(struct pcb* originalPCB){    
    struct pcb* newPCB = createPCB(originalPCB->numLines, originalPCB->numCommands);
    
    // Copy all important fields that may be used in PCB execution from original PCB
    newPCB->pid = originalPCB->pid; // This PID field no longer has to be unique, however another unique identifier is added in createPCB
    newPCB->firstInstructionKey = originalPCB->firstInstructionKey;
    newPCB->pagetable = originalPCB->pagetable; // Shares the pagetable pointer of original PCB
    newPCB->fileName = originalPCB->fileName;
    return newPCB;
}

//Executes readyQueue list sequentially, one PCB at a time
int executeReadyQueue(){
    char* currentInstructionKey = malloc(24*sizeof(char));
    while(readyQueue != NULL){
        //Foreground execution follows different logic
        if(readyQueue->isForeground == 'Y'){
            executeForegroundPCB(readyQueue,-1);
        }
        else{
            // Fetch and execute every command in the PCB
            for(int i=0; i<readyQueue->numCommands; i++){
                sprintf(currentInstructionKey,"process_%d_command_%d",readyQueue->pid,i);
                char* curr_command = mem_get_value(currentInstructionKey);
                curr_command[strcspn(curr_command,"\n")] = 0;
                if(IS_VERBOSE) printf("\tExecuting command: %s\n",curr_command);
                parseInput(curr_command);        
            }
        }
        //After loop/foreground exec, nothing left to execute
        // so clear the pcb and move to next process
        struct pcb* nextPCB = readyQueue->nextPCB;
        clearPCB(readyQueue);
        free(readyQueue);
        readyQueue = nextPCB;
    }
    free(currentInstructionKey);
    return 0;
}

int agingRearrange(){
    //Nothing to rearrange if <2 elements
    if((readyQueue) == NULL || (readyQueue)->nextPCB == NULL){
        return 0;
    }
    int numPCBs = 0;
    struct pcb* tracker = readyQueue;
    // Get total number of PCBs to sort
    while(tracker != NULL){
        numPCBs++;
        tracker = tracker->nextPCB;
    }
    struct pcb* allPCBs[numPCBs];
    tracker = readyQueue;
    int i=0;
    // Store all PCBs in a non sorted array for easier access
    while(tracker != NULL){
        allPCBs[i] = tracker;
        tracker = tracker->nextPCB;
        // Order of PCBs might change, so remove their next pointers
        allPCBs[i]->nextPCB = NULL;
        i++;
    }
    struct pcb* sortedPCBs[numPCBs];
    // Bubble sort by jobLengthScore field into new array
    for(int j=0;j<numPCBs;j++){
        int min=200;
        int minIdx=-1;
        for(int i=0; i<numPCBs;i++){
            if(allPCBs[i] != NULL && allPCBs[i]->jobLengthScore < min){
                min = allPCBs[i]->jobLengthScore;
                minIdx = i;
            }
        }
        sortedPCBs[j] = allPCBs[minIdx];
        // Remove newly added PCB from unsorted array so it doesn't get processed twice
        allPCBs[minIdx] = NULL;
    }

    // Set highest priority element to head of readyQueue
    readyQueue = sortedPCBs[0];
    // Add every following sorted element to readyQueue
    for(int i=1; i<numPCBs;i++){
        add_to_queue(sortedPCBs[i]);
    }
    return 0;
}

// Aging all pcbs in readyQueue except head
int ageProcesses(){
    struct pcb* curr = (readyQueue)->nextPCB;
    while(curr!=NULL){
        if(curr->jobLengthScore > 0){
            curr->jobLengthScore--;
        }
        curr = curr->nextPCB;
    }
}

int agingExecuteReadyQueue(){
    char* currentInstructionKey = malloc(24*sizeof(char));
    while(readyQueue != NULL){
        sprintf(currentInstructionKey,"process_%d_command_%d",readyQueue->pid,readyQueue->currentInstructionLine);
        char* currCommand = mem_get_value(currentInstructionKey);

        // Aging process placed before any process that could change readyQueue
        ageProcesses();

        if(IS_VERBOSE) printf("\tExecuting command: %s\n",currCommand);
        currCommand[strcspn(currCommand,"\n")] = 0;
        parseInput(currCommand);
        readyQueue->currentInstructionLine++;
        
        // Clearing instructions from memory if pcb complete
        if(readyQueue->currentInstructionLine >= readyQueue->numCommands){
            if(IS_VERBOSE) printf("\tCLEARING PCB %d",readyQueue->pid);
            clearPCB(readyQueue);
            readyQueue = readyQueue->nextPCB;
        }
        //Re-sort after each iter since other processes aged
        agingRearrange();
    }
    //free memory allocated to fetching instructions after execution complete
    free(currentInstructionKey);
}

int roundRobinExecuteReadyQueue(int numExecutions){
    
    struct pcb* tail = readyQueue;
    //Navigates to tail of readyQueue linked list
    while(tail->nextPCB != NULL){
        tail = tail->nextPCB;
    }
    while(readyQueue != NULL){
        //Foreground execution process is different from others
        //Defined separately in a different function
        if(readyQueue->isForeground=='Y'){
            executeForegroundPCB(readyQueue,numExecutions);
        }else{
           for(int i=0;i<numExecutions;i++){
                if(readyQueue->currentInstructionLine>=readyQueue->numCommands){
                    //if this condition is met, then we are done executing
                    if(IS_VERBOSE) printf("\tPROCESS %d COMPLETED, EXITING",readyQueue->pid);
                    break;
                }
                // Fetch and execute instruction in shell memory
                int currentInstruction = readyQueue->currentInstructionLine;
                int frameNumber = readyQueue->pagetable[(int)currentInstruction/3];
                // Per decided convention, frameNumber -1 indicates invalid page. 
                // This line only gets reached due to pageFault, since the currentInstructionLine is less than numInstructions
                // So only reason frameNumber can be invalid is if it is not in memory
                if(frameNumber == -1){
                    // Load next page into memory
                    int frameNumber = addFrameFromBackingStore(readyQueue->fileName, currentInstruction);
                    // Update PCB table
                    readyQueue->pagetable[(int)currentInstruction/3] = frameNumber;
                    // Leave and have the code set PCB back to queue tail
                    break;
                }
                // Else, page is in memory, so load instruction and continue
                char* curr_command = getFromFrameStore(frameNumber,currentInstruction%3);
                
                if(IS_VERBOSE) printf("\tExecuting command: %s\n",curr_command);
                
                // Treats an entire one-liner in one iteration of RR loop because it is considered 1 instruction
                char* singleInput = strtok(curr_command,";");
                while(singleInput != NULL){
                    parseInput(singleInput);        
                    singleInput = strtok(NULL, ";");
                }

                // Increment curr instruction line for following iterations
                readyQueue->currentInstructionLine++;
            }

        }
        
        // Check if PCB's lines have all been executed
        if(readyQueue->currentInstructionLine >= readyQueue->numCommands){
            if(IS_VERBOSE) printf("\tCLEARING PCB %d\n",readyQueue->pid);
            clearPCB(readyQueue);
            if(readyQueue->nextPCB == NULL){
                break;
            }
            
            // Move to next process in readyQueue
            readyQueue = readyQueue->nextPCB;
        }else{
            //Move current pcb to tail of readyQueue
            tail->nextPCB = readyQueue;
            tail = readyQueue;
            // Move to next PCB in linkedList
            readyQueue = readyQueue->nextPCB;
            // Avoid circular linkedlist
            tail->nextPCB = NULL; 
        }
    }
}

void* roundRobinExecuteMT(void* numExecutionsPtr){
    int numExecutions = *((int*) numExecutionsPtr);
    while(readyQueue != NULL){
        // Prevent simultaneous access and modification to readyQueue
        pthread_mutex_lock(&lock);
        struct pcb* myPCB = readyQueue;
        readyQueue = readyQueue->nextPCB;
        pthread_mutex_unlock(&lock);

        // Same execution logic as regular round robin
        if(myPCB->isForeground == 'Y'){
            executeForegroundPCB(myPCB,numExecutions);
        }
        else{
            char* currentInstructionKey = malloc(24*sizeof(char));
            for(int i=0;i<numExecutions;i++){
                if(myPCB->currentInstructionLine>=myPCB->numCommands){
                    if(IS_VERBOSE) printf("\tPROCESS %d COMPLETED, EXITING",myPCB->pid);
                    break;
                }
                
                sprintf(currentInstructionKey,"process_%d_command_%d",myPCB->pid,myPCB->currentInstructionLine);
                char* curr_command = mem_get_value(currentInstructionKey);
                curr_command[strcspn(curr_command,"\n")] = 0;
                if(IS_VERBOSE) printf("\tExecuting command: %s\n",curr_command);
                parseInput(curr_command); 
                myPCB->currentInstructionLine++;

            }
            free(currentInstructionKey);
        }
        if(myPCB->isForeground=='N' && myPCB->currentInstructionLine>=myPCB->numCommands){
            clearPCB(myPCB);
        }else{
            //Prevent simultaneous access to readyQueue again when adding back to queue
            pthread_mutex_lock(&lock);
            add_to_queue(myPCB);
            myPCB->nextPCB=NULL;
            pthread_mutex_unlock(&lock);
        }
    }
}

int executeForegroundPCB(struct pcb* foregroundPCB,int numExections){
    int i=0;

    //loop until end of file or until specified number of command executed
    while(1) {
        char userInput[MAX_USER_INPUT];
        fgets(userInput, MAX_USER_INPUT-1, stdin);
        char* singleInput;
        foregroundPCB->numLines++;
        singleInput = strtok(userInput,";");

        while(singleInput != NULL){
          parseInput(singleInput);          
          singleInput = strtok(NULL, ";");
        }

        // Count number of executed lines
        i++;
        if (feof(stdin)){
            return 0;
        }
        if (numExections > 0 && i >= numExections){
            return 0;
        }
    }
}

// This loadScript is called when a script is loaded into memory for the first time
// eg not using existing loaded instructions in memory
struct pcb* newLoadScript(char* fileName){
    // By default, on first load, always want to load first 2 frames into memory
    int numFramesToLoad = 2;
    // Copy file into Backing Store
    int numLines = copyToBackingStore(fileName);
    struct pcb* newPCB = createPCB(numLines,numLines);

    // Filename is used to check if a script is loaded twice
    // Since this is the first time the script gets loaded,
    // It is important to set this variable
    newPCB->fileName = fileName;

    // Load lines from backing store file into frame store
    int frameCount = 0;
    int pageIndexInFrame = 0;
    while(pageIndexInFrame != -1 && frameCount < numFramesToLoad){
        // Ensures 1 page gets added for every 3 lines in file
        pageIndexInFrame = addFrameFromBackingStore(fileName,frameCount*3);

        // Update the next pagetable entry to the frame number associated with page
        newPCB->pagetable[frameCount] = pageIndexInFrame;
        frameCount++;
    }

    return newPCB;
}

int exec(char *fileNames[], char* policy, int numFiles, int isBackground, int isMT){    
    if(strcmp(policy, "FCFS")!=0 && strcmp(policy,"RR")!=0
        && strcmp(policy,"SJF")!=0 && strcmp(policy,"AGING")!=0
        && strcmp(policy,"RR30")!=0){
            return badcommand();
        }
    // If specified to run in background
    // Creates a new PCB to run foreground commands, with priority over other processes
    if(isBackground){
        if(readyQueue == NULL){
            readyQueue = createPCB(0,0);
        }else{
            struct pcb* newPCB = createPCB(0,0);
            newPCB->nextPCB = readyQueue;
            readyQueue = newPCB;
        }
        readyQueue->isForeground = 'Y';
    }

    // Create PCBs for the specified files
    struct pcb* allPCBs[numFiles];
    for(int i=0; i<numFiles;i++){
        // Avoid double creating PCB after loop by setting & checking this 
        int codeLoaded = 0;
        
        // If same name passed multiple times, share same code
        // So check all loaded names so far, and create pcb with shared code
        for(int j=0; j<i; j++){
            if(strcmp(fileNames[i],fileNames[j])==0){
                struct pcb* newPCB = loadScriptFromPCB(allPCBs[j]);
                allPCBs[i] = newPCB;
                codeLoaded = 1;
                break;
            }
        }
        // Skips creating PCB again
        if(codeLoaded) continue;

        struct pcb* newPCB = newLoadScript(fileNames[i]);
        allPCBs[i] = newPCB;
    }
    


    if(strcmp(policy,"SJF")==0){
        // Bubble sort by job length
        struct pcb* sortedPCBs[numFiles];
        for(int j=0; j<numFiles;j++){
            int min = 200;
            int minIdx = -1;
            for(int i=0; i<numFiles; i++){
                if(allPCBs[i] != NULL && allPCBs[i]->numLines<min){
                    min = allPCBs[i]->numLines;
                    minIdx = i;
                }
            }
            sortedPCBs[j] = allPCBs[minIdx];
            // Remove element from unsorted array to avoid double treatment
            allPCBs[minIdx] = NULL;
        }
        for(int c=0; c<numFiles;c++){
            allPCBs[c] = sortedPCBs[c];
        }
    }
    
    // Add all newly created PCBs to global readyQueue list
    for(int i=0; i<numFiles; i++){
        add_to_queue(allPCBs[i]);
    }
    // Initialize global variable lock
    if(isMT){
        pthread_mutex_init(&lock,NULL);
    }

    // Execute readyQueue by policy
    if(strcmp(policy,"RR")==0){
        if(isMT){
            // Can only pass pointers to thread argument
            // Will be dereferenced in actual execution
            int* numExecPtr = malloc(sizeof(int));
            *numExecPtr = 2;
            pthread_create(&thread1,NULL,roundRobinExecuteMT,numExecPtr);
            pthread_create(&thread2,NULL,roundRobinExecuteMT,numExecPtr);
        }
        else{
            roundRobinExecuteReadyQueue(2);
        }
    }else if(strcmp(policy,"RR30")==0){
        if(isMT){
            // See case RR
            int* numExecPtr = malloc(sizeof(int));
            *numExecPtr = 30;

            pthread_create(&thread1,0,roundRobinExecuteMT,numExecPtr);
            pthread_create(&thread2,0,roundRobinExecuteMT,numExecPtr);
        }else{
            roundRobinExecuteReadyQueue(30);
        }
    }else if(strcmp(policy,"AGING")==0){
        agingExecuteReadyQueue();
    }else{
        executeReadyQueue();
    }
    
    // Join threads once they are completed
    if(isMT){
        join_threads();
    }

    //quitSignal that may have gotten set by one of the child threads
    // will execute here after all other threads have completed
    if(quitSignal){
        exit(0);
    }

}

int join_threads(){
    if(!pthread_equal(pthread_self(),thread1)){
        pthread_join(thread1,NULL);
        thread1Joined = 1;
    }
    if(!pthread_equal(pthread_self(),thread2)){
        pthread_join(thread2,NULL);
        thread2Joined = 2;
    }
}