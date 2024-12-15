struct pcb {
    int pid;
    int actualId;
    int numLines;
    int currentInstructionLine;
    int numCommands;
    int jobLengthScore;
    char* firstInstructionKey;
    char* fileName;
    char isForeground;
    int* pagetable;
    struct pcb* nextPCB;
};
int clearPCB(struct pcb* pcb_in);
void add_to_queue(struct pcb* newPCB);
struct pcb* createPCB(int numLines, int numCommands);
extern int PROCESS_COUNT;
extern struct pcb* readyQueue;

