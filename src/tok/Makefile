all: tokenizer
CFLAGS=-O2
#CFLAGS=-g -O0

tokenizer: tokenizer.o main.o tinytemplates.o
	g++ $(CPPFLAGS) $(CFLAGS) tokenizer.o main.o tinytemplates.o -otokenizer

tinytemplates.o: tinytemplates.c tinytemplates.h
	gcc $(CFLAGS) -c tinytemplates.c -otinytemplates.o

tokenizer.o: tokenizer.c tokenizer.h
	gcc $(CFLAGS) -c tokenizer.c -otokenizer.o

main.o: main.c tokenizer.h
	gcc $(CFLAGS) -c main.c -omain.o

clean:
	-rm *.o > /dev/null 2>&1
	-rm tokenizer > /dev/null 2>&1

