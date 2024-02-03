# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019

CC= gcc
CFLAGS= -g -Wall
LIBS = 

OBJS = networks.o gethostbyname.o pollLib.o safeUtil.o sendPDU.o recvPDU.o handleStruct.o

all:   cClient Server

cClient: cClient.c $(OBJS)
	$(CC) $(CFLAGS) -o cclient cClient.c  $(OBJS) $(LIBS)

Server: Server.c $(OBJS)
	$(CC) $(CFLAGS) -o server Server.c $(OBJS) $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f Server cClient *.o




