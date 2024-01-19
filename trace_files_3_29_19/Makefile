# Example makefile for CPE464 program 1
#
# 

CC = gcc
CFLAGS = -g -Wall 
#CFLAGS = -g

all:  trace

trace: trace.cpp
	$(CC) $(CFLAGS) -o trace trace.cpp -lpcap 

trace.o: trace.cpp
	$(CC) $(CFLAGS) -c trace.cpp

clean:
	rm -f trace
