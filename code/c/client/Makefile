# get some default defines for compiling
include ../common/Makefile.defines


CC = gcc
LINK.cc = $(CC)

CFLAGS = -Wall -Wextra -g $(CDEFINES)
.PHONY: all clean

build: all

all: client

client: main.o
	$(LINK.cc) -o $@ $^

main.o: main.c

../server_utils.o: ../server_utils.c

clean:
	-rm -f *~
	-rm -f *.o
	-rm -f ../server_utils.o
	-rm -f client
