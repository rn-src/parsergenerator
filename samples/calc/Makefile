OBJ=calculator.o
CLAGS=-I. -g -Od
DEPS=parser.h ../tok/tinytemplates.h

all: calculator

calculator: $(OBJ)
	g++ -o $@ $^ $(CPPFLAGS) $(CFLAGS)

%.o: %.cpp $(DEPS)
	g++ -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

clean:
	rm *.o
	rm calculator

