all: tokenizer
CFLAGS=-g -O0

tokenizer: tokenizer.o main.o
	g++ $(CPPFLAGS) $(CFLAGS) tokenizer.o main.o -otokenizer

tokenizer.o: tokenizer.cpp tokenizer.h
	g++ -g -c tokenizer.cpp -otokenizer.o

main.o: main.cpp tokenizer.h
	g++ -g -c main.cpp -omain.o

clean:
	rm *.o
	rm tokenizer

