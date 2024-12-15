CC=gcc
CFLAGS=

mysh: shell.c processmanagement.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -Dframesize=$(framesize) -Dvarmemsize=$(varmemsize) -c shell.c processmanagement.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -o mysh shell.o processmanagement.o interpreter.o shellmemory.o

clean: 
	rm mysh; rm *.o
