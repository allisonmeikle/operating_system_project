CC=gcc
FMT=indent
CFLAGS = -g -o -DVAR_MEM_SIZE=$(varmemsize) -DFRAME_MEM_SIZE=$(framesize)

mysh: shell.o interpreter.o shellmemory.o scheduler.o
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o scheduler.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h scheduler.c scheduler.h
	$(FMT) $?

clean: 
	$(RM) mysh; $(RM) *.o; $(RM) *~

