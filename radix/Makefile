M4=m4
CC=gcc
CFLAGS:=-Werror -O3 -g -std=c89 $(CFLAGS)
LDFLAGS:=$(LDFLAGS) -lm

MACROS = c.m4.null

all: radix

radix: radix.o

%.c: %.C
	$(M4) $(MACROS) $*.C > $*.c

# Disable the built-in C++ rule
%.o: %.C

clean:	
	rm -f radix *.o
