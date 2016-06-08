CC = g++
CFLAGS = -Wall -std=c++11 -g
EXTRA_FILES = Makefile README


all: Server Client
    emServer
    emClient

Server: Server.cpp server.h
    ${CC} ${CFLAGS} Server.cpp -o emServer

Client: Client.cpp Client.h
	${CC} ${CFLAGS} Client.cpp -o emClient

tar:
	tar cvf ex5.tar Server.cpp server.h Client.cpp Client.h ${EXTRA_FILES}

clean:
	rm emServer emClient *.o ex5.tar

.DEFAULT_GOAL := all

.PHONY:
	clean all un