OBJ=parser.o parsecommon.o parsersolve.o parseroutput.o main.o parserrestrictfa.o timer.o tinytemplates.o tok.o tokenizer.o tokenizeroutput.o

CFLAGS=-I. -g -O0 -fno-unwind-tables -fno-asynchronous-unwind-tables
#CFLAGS=-I. -Os -s -fno-unwind-tables -fno-asynchronous-unwind-tables
DEPS=parsecommon.h parser.h tokenizer.h tinytemplates.h

all: parser

parser: $(OBJ)
	gcc -o $@ $^ $(CPPFLAGS) $(CFLAGS)

%.o: %.c $(DEPS)
	gcc -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

clean:
	-rm *.o > /dev/null 2>&1
	-rm parser > /dev/null 2>&1
