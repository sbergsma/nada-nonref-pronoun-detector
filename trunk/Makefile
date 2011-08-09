.SUFFIXES: .c .cpp

CC=g++
GO = -O3

CFLAGS = $(GO) -Wall
EXECS = nadaIt

%.o:	%.cpp
	$(CC) -c -o $@ $(CFLAGS) $<

%.o:	%.c
	$(CC) -c -o $@ $(CFLAGS) $<

all: $(EXECS)

nadaIt:	nadaIt.o nadaCommon.o
	$(CC) -o $@ $(CFLAGS) nadaIt.o nadaCommon.o

depend:
	$(CC) -MM $(CFLAGS) *.cpp >.dep

clean:
	rm -rf *.o core temp $(EXECS) *~

include .dep
