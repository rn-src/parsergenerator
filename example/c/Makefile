OBJ=main.o lrparse.o tokenizer.o
CFLAGS=-I. -g

all: example

example: $(OBJ) lrparse.h tokenizer.h exampleG.h exampleL.h
	gcc -o $@ $(OBJ) $(CFLAGS)

%.o: %.c lrparse.h tokenizer.h exampleG.h exampleL.h
	gcc -c -o $@ $< $(CFLAGS)

exampleG.h: example.G
	parser parser '--import=<stdlib.h>' example.G

exampleL.h: example.L
	parser tokenizer example.L

clean:
	-rm exampleG.h > /dev/null 2>&1
	-rm exampleL.h > /dev/null 2>&1
	-rm *.o > /dev/null 2>&1
	-rm example > /dev/null 2>&1

