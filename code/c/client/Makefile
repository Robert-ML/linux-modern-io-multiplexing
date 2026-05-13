# get some default defines for compiling
include ../common/Makefile.defines


CC = gcc

CFLAGS = -Wall -Wextra $(CDEFINES_COMMON)

CFLAGS_DEBUG = -g -Og $(CDEFINES_DEBUG)
CFLAGS_TEST = -g -O1 $(CDEFINES_TEST)
CFLAGS_RELEASE = -O3 $(CDEFINES_RELEASE)


.PHONY: build build_debug build_test build_release clean


build: build_release

build_debug: CFLAGS += $(CFLAGS_DEBUG)
build_debug: client

build_test: CFLAGS += $(CFLAGS_TEST)
build_test: client

build_release: CFLAGS += $(CFLAGS_RELEASE)
build_release: client


client: main.c
	$(CC) $(CFLAGS) $^ -o $@


clean:
	-rm -f client
