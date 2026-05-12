# get some default defines for compiling
include ../../common/Makefile.defines


CC = gcc
LINK.cc = $(CC)

CFLAGS = -Wall -Wextra -g $(CDEFINES) -O3

.PHONY: all clean

build: poll_server


poll_server: main.o ./pollfd_vector.o ../server_utils.o ../server_benchmark.o ../../common/measurements.o ../../common/time_utils.o
	$(LINK.cc) -o $@ $^

main.o: main.c

./pollfd_vector.o: ./pollfd_vector.c

../server_utils.o: ../server_utils.c

../server_benchmark.o: ../server_benchmark.c

../../common/measurements.o: ../../common/measurements.c ../../common/measurements.h
	make -C ../../common/ measurements.o

../../common/time_utils.o: ../../common/time_utils.c ../../common/time_utils.h
	make -C ../../common/ time_utils.o


poll_mem_test: poll_server
	make -C ../../client/
	sleep 1 && ../../client/client &> /dev/null &
	sleep 1 && ../../client/client run_once &> /dev/null &
	sleep 5 && vgdb -l | awk -F'--pid=| ' '{print $$3}' | xargs kill -s USR1 &
	valgrind --error-exitcode=101 --leak-check=full --show-leak-kinds=all --error-exitcode=1 -s ./poll_server


clean:
	-rm -f *~
	-rm -f *.o
	-rm -f ../server_utils.o
	-rm -f ../server_benchmark.o
	-rm -f poll_server
	-make -C ../../common/ clean
