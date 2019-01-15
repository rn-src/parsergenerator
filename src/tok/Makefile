all: tokenizer

tokenizer: tokenizer.o main.o
	g++ tokenizer.o main.o -otokenizer

tokenizer.o: tokenizer.cpp tokenizer.h
	g++ -g -c tokenizer.cpp -otokenizer.o

main.o: main.cpp tokenizer.h
	g++ -g -c main.cpp -omain.o

clean:
	rm *.o
	rm tokenizer

