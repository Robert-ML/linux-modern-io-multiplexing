# get some default defines for compiling
include ../../common/Makefile.defines


CC = gcc

CFLAGS = -Wall -Wextra $(CDEFINES_COMMON)

CFLAGS_DEBUG = -g -Og $(CDEFINES_DEBUG)
CFLAGS_TEST = -g -O1 $(CDEFINES_TEST)
CFLAGS_RELEASE = -O3 $(CDEFINES_RELEASE)


.PHONY: build build_debug build_test build_release clean


build: build_release

build_debug: CFLAGS += $(CFLAGS_DEBUG)
build_debug: poll_server

build_test: CFLAGS += $(CFLAGS_TEST)
build_test: poll_server

build_release: CFLAGS += $(CFLAGS_RELEASE)
build_release: poll_server


poll_server: main.c ./pollfd_vector.c ../server_utils.c ../server_benchmark.c ../../common/measurements.c ../../common/time_utils.c
	$(CC) $(CFLAGS) $^ -o $@


test_memory_poll: build_test
	make -C ../../client/ clean
	make -C ../../client/ build_test
	sleep 1 && ../../client/client &> /dev/null &
	sleep 1 && ../../client/client run_once &> /dev/null &
	sleep 5 && vgdb -l | awk -F'--pid=| ' '{print $$3}' | xargs kill -s USR1 &
	valgrind --error-exitcode=101 --leak-check=full --show-leak-kinds=all -s ./poll_server


clean:
	-rm -f ./bench_file.json
	-rm -f ./poll_server
	-make -C ../../client/ clean
