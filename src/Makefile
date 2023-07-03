OBJ=parser.o parsersolve.o parseroutput.o main.o parserrestrictfa.o timer.o tinytemplates.o tok.o tokenizer.o tokenizeroutput.o

#CFLAGS=-I. -g -O0
CFLAGS=-I. -O2
DEPS=parser.h tokenizer.h tinytemplates.h

all: parser

parser: $(OBJ)
	gcc -o $@ $^ $(CPPFLAGS) $(CFLAGS)

%.o: %.c $(DEPS)
	gcc -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

clean:
	-rm *.o > /dev/null 2>&1
	-rm parser > /dev/null 2>&1

