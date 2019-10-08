OBJ=parser.o parsersolve.o parseroutput.o main.o parserforbidauto.o
CFLAGS=-I. -g -O0
DEPS=parser.h ../tok/tinytemplates.h

all: parser

parser: $(OBJ)
	g++ -o $@ $^ $(CPPFLAGS) $(CFLAGS)

%.o: %.cpp $(DEPS)
	g++ -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

clean:
	-rm *.o > /dev/null 2>&1
	-rm parser > /dev/null 2>&1

