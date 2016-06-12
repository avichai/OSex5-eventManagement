CC = g++
CFLAGS = -Wall -std=c++11 -g
EXTRA_FILES = Makefile README
FILES = Server.cpp server.h Client.cpp Client.h Utils.cpp


all: Server Client
	emServer 8889


Server: Server.cpp Server.h Utils.cpp
	${CC} ${CFLAGS} Server.cpp -o emServer

Client: Client.cpp Client.h Utils.cpp
	${CC} ${CFLAGS} Client.cpp -o emClient

tar:
	tar cvf ex5.tar ${FILES} ${EXTRA_FILES}

# remove
main: main1
	main

main1:
	${CC} ${CFLAGS} main.cpp -o main

clean:
	rm emServer emClient *.o ex5.tar

.DEFAULT_GOAL := all

.PHONY:
	clean all un
