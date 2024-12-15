#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
    int errorCode = 0;					// zero means no error, default

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }
    
    //init shell memory
    mem_init();
    while(1) {	
        // If interactive mode, print $ before user input
        if(isatty(0)){
            printf("%c ", prompt);
        }

        fgets(userInput, MAX_USER_INPUT-1, stdin);
        char* singleInput;
        singleInput = strtok(userInput,";");
        while(singleInput != NULL){
          errorCode = parseInput(singleInput);
          if(errorCode == -1) exit(99);
          memset(singleInput,0,sizeof(singleInput));
          
          singleInput = strtok(NULL, ";");
        }

        // In batch mode, exit at eof
        if (feof(stdin)){
            return 0;
            //freopen("/dev/tty","r",stdin);
        }
    }

    return 0;
}

int wordEnding(char c) {
    // You may want to add ';' to this at some point,
    // or you may want to find a different way to implement chains.
    return c == '\0' || c == '\n' || c == ' ';
}

int parseInput(char inp[]) {
    char tmp[200], *words[100];                            
    int ix = 0, w = 0;
    int wordlen;
    int errorCode;
    for (ix = 0; inp[ix] == ' ' && ix < 1000; ix++); // skip white spaces
    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
        // extract a word
        for (wordlen = 0; !wordEnding(inp[ix]) && ix < 1000; ix++, wordlen++) {
            tmp[wordlen] = inp[ix];                        
        }
        tmp[wordlen] = '\0';
        words[w] = strdup(tmp);
        w++;
        if (inp[ix] == '\0') break;
        ix++; 
    }
    errorCode = interpreter(words, w);
    return errorCode;
}
