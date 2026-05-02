# get some default defines for compiling
include ./Makefile.defines

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O3 -g $(CDEFINES)

.PHONY: clean

time_utils.o: time_utils.c time_utils.h
	$(CC) $(CFLAGS) -o $@ -c $<

measurements.o: measurements.c measurements.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o
