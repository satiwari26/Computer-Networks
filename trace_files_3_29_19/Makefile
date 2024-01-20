# Example makefile for CPE464 program 1
#
# gcc -o trace trace.cpp -lstdc++ -lpcap

CC = gcc
CFLAGS = -g -Wall -std=gnu11
CPPFLAGS = -g -Wall -std=c++11
#CFLAGS = -g

all:  trace

trace: trace.cpp
	$(CC) $(CFLAGS) -c checksum.c -o checksum
	$(CC) $(CPPFLAGS) -o trace trace.cpp checksum -lstdc++ -lpcap

clean:
	rm -f trace checksum output.txt
