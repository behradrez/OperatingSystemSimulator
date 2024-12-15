#define MEM_SIZE 1000
#define BACKING_FILE "backing_store"
void mem_init();
int remove_dir(char *dirname);
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int mem_clear_value(char *var);
int copyToBackingStore(char* fileName);
int addFrameFromBackingStore(char* fileName, int lineIndex);
char* getFromFrameStore(int frameNumber, int offset);
