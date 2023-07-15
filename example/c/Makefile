OBJ=main.o lrparse.o tokenizer.o common.o
#CFLAGS=-I. -g -O0 -fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS=-I. -Os -fno-unwind-tables -fno-asynchronous-unwind-tables

all: example

example: $(OBJ) lrparse.h tokenizer.h exampleG.h exampleL.h
	gcc -o $@ $(OBJ) $(CFLAGS)

%.o: %.c lrparse.h tokenizer.h exampleG.h exampleL.h
	gcc -c -o $@ $< $(CFLAGS)

exampleG.h: example.G exampleL.h
	../../src/parser parser '--import=<stdlib.h>' example.G

exampleL.h: example.L
	../../src/parser tokenizer example.L

clean:
	-rm exampleG.h > /dev/null 2>&1
	-rm exampleL.h > /dev/null 2>&1
	-rm *.o > /dev/null 2>&1
	-rm example > /dev/null 2>&1

