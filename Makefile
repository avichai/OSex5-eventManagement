CC = g++
CFLAGS = -Wall -std=c++11
SERVER_FLAGS = -pthread
EXTRA_FILES = Makefile README
FILES = emServer.cpp emClient.cpp Utils.h Utils.cpp


all: emServer emClient

emServer: emServer.cpp Utils.h
	${CC} ${CFLAGS} ${SERVER_FLAGS} emServer.cpp Utils.cpp -o emServer

emClient: emClient.cpp Utils.h
	${CC} ${CFLAGS} emClient.cpp Utils.cpp -o emClient

tar:
	tar cvf ex5.tar ${FILES} ${EXTRA_FILES}

clean:
	rm emServer emClient *.o ex5.tar

.DEFAULT_GOAL := all

.PHONY:
	clean all
