# TXMLPARSER MAKEFILE
OUTFILE=txml_parser

CFLAGS=-Wall -Werror -Os 

CC=gcc

all: $(OUTFILE)

debug: $(OUTFILE)

clean:
	rm -f *.o $(OUTFILE)

$(OUTFILE): txml_parser.o main.o
	$(CC) *.o -o $(OUTFILE) $(LFLAGS)

.c.o:
	$(CC) -c $*.c -o $*.o $(CFLAGS)

