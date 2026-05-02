# get some default defines for compiling
include ../../common/Makefile.defines


CC = gcc
LINK.cc = $(CC)

CFLAGS = -Wall -Wextra -g $(CDEFINES) -O3

.PHONY: all clean

build: all

all: poll_server

poll_server: main.o ../server_utils.o ../server_benchmark.o ../../common/measurements.o ../../common/time_utils.o
	$(LINK.cc) -o $@ $^

main.o: main.c

../server_utils.o: ../server_utils.c

../server_benchmark.o: ../server_benchmark.c

../../common/measurements.o: ../../common/measurements.c ../../common/measurements.h
	make -C ../../common/ measurements.o

../../common/time_utils.o: ../../common/time_utils.c ../../common/time_utils.h
	make -C ../../common/ time_utils.o

clean:
	-rm -f *~
	-rm -f *.o
	-rm -f ../server_utils.o
	-rm -f ../server_benchmark.o
	-rm -f poll_server
	-make -C ../../common/ clean
