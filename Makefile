CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wswitch-enum -std=c2x -pedantic -I$(IDIR)
LIBS =
IDIR = ./headers
CDIR = ./src

_HEADERS = gbvm_memory.h gbvm_errors.h gbvm_strings.h gbvm_fileops.h gbvm_instructions.h gbvm_assembler.h gbvm.h external_includes.h
HEADERS = $(patsubst %,$(IDIR)/%,$(_HEADERS))

_CODE = gbvm_errors.c gbvm_strings.c gbvm_fileops.c gbvm_instructions.c gbvm_assembler.c gbvm.c
CODE = $(patsubst %,$(CDIR)/%,$(_CODE))

gbvm: main.c $(CODE)
	  $(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(CDIR)/*.o main.o gbvm
