# Makefile for CPE464 tcp and udp test code
# updated by Hugh Smith - April 2023

CC = gcc
CFLAGS = -g -Wall

# List of object files
OBJS = networks.o gethostbyname.o pollLib.o safeUtil.o createPDU.o window.o

# Uncomment next two lines if you're using sendtoErr() library
LIBS += libcpe464.2.21.a -lstdc++ -ldl
CFLAGS += -D__LIBCPE464_

all: udpAll

udpAll: rcopy server
tcpAll: myClient myServer

# Compile createPDU.c to create createPDU.o
createPDU.o: createPDU.c createPDU.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile window.c to create window.o
window.o: window.c window.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build rcopy executable, linking with createPDU.o
rcopy: rcopy.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

server: server.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

myClient: myClient.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

myServer: myServer.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f myServer myClient rcopy server *.o
