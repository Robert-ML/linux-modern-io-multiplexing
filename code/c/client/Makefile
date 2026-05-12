# get some default defines for compiling
include ../common/Makefile.defines


CC = gcc
LINK.cc = $(CC)

CFLAGS = -Wall -Wextra -g $(CDEFINES)
.PHONY: all clean

build: client

client: main.o
	$(LINK.cc) -o $@ $^

main.o: main.c


clean:
	-rm -f *~
	-rm -f *.o
	-rm -f client
