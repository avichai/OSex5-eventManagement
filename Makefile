CC = g++
CFLAGS = -Wall -std=c++11
SERVER_FLAGS = -pthread
EXTRA_FILES = Makefile README
FILES = Server.cpp server.h Client.cpp Client.h Utils.cpp


all: Server Client


Server: Server.cpp Server.h Utils.h
	${CC} ${CFLAGS} ${SERVER_FLAGS} Server.cpp Utils.cpp -o emServer

Client: Client.cpp Client.h Utils.h
	${CC} ${CFLAGS} Client.cpp Utils.cpp -o emClient

S:
	emServer

C:
	emClient yossi

tar:
	tar cvf ex5.tar ${FILES} ${EXTRA_FILES}

# remove 132.65.125.69
main: main1
	main

main1:
	${CC} ${CFLAGS} main.cpp -o main

clean:
	rm emServer emClient *.o ex5.tar

.DEFAULT_GOAL := all

.PHONY:
	clean all un
