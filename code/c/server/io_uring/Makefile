# get some default defines for compiling
include ../../common/Makefile.defines


CC = gcc

CFLAGS = -Wall -Wextra $(CDEFINES_COMMON) $(CDEFINES_IO_URING)
LIBS = -luring

CFLAGS_DEBUG = -g -Og $(CDEFINES_DEBUG)
CFLAGS_TEST = -g -O1 $(CDEFINES_TEST)
CFLAGS_RELEASE = -O3 $(CDEFINES_RELEASE)


.PHONY: build build_debug build_test build_release clean clean_full


build: build_release

build_debug: CFLAGS += $(CFLAGS_DEBUG)
build_debug: io_uring_server

build_test: CFLAGS += $(CFLAGS_TEST)
build_test: io_uring_server

build_release: CFLAGS += $(CFLAGS_RELEASE)
build_release: io_uring_server


io_uring_server: main.c ./io_uring_server_utils.c ../server_utils.c ../server_benchmark.c ../../common/measurements.c ../../common/time_utils.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)


test_memory_no_clients: build_test
	sleep 5 && vgdb -l | awk -F'--pid=| ' '{print $$3}' | xargs kill -s USR1 &
	valgrind --error-exitcode=101 --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes -s ./io_uring_server

test_memory_with_clients: build_test
	make -C ../../client/ clean
	make -C ../../client/ build_test
	sleep 1 && ../../client/client &> /dev/null &
	sleep 1 && ../../client/client run_once &> /dev/null &
	sleep 5 && vgdb -l | awk -F'--pid=| ' '{print $$3}' | xargs kill -s USR1 &
	valgrind --error-exitcode=101 --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes -s ./io_uring_server


clean:
	-rm -f ./io_uring_server

clean_full: clean
	-rm -f ./bench_file.json
	-make -C ../../client/ clean
