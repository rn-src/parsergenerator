all: tokenizer
CFLAGS=-O2
#CFLAGS=-g -O0

tokenizer: tokenizer.o tokenizeroutput.o main.o tinytemplates.o
	g++ $(CPPFLAGS) $(CFLAGS) tokenizer.o tokenizeroutput.o main.o tinytemplates.o -otokenizer

tinytemplates.o: tinytemplates.c tinytemplates.h
	gcc $(CFLAGS) -c tinytemplates.c -otinytemplates.o

tokenizer.o: tokenizer.c tokenizer.h
	gcc $(CFLAGS) -c tokenizer.c -otokenizer.o

tokenizeroutput.o: tokenizeroutput.c tokenizer.h
	gcc $(CFLAGS) -c tokenizeroutput.c -otokenizeroutput.o

main.o: main.c tokenizer.h
	gcc $(CFLAGS) -c main.c -omain.o

clean:
	-rm *.o > /dev/null 2>&1
	-rm tokenizer > /dev/null 2>&1

