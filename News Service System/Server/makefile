all: run

run: server_main
	./server_main 6500 7000

server_main: server_main.cpp server record
	g++ -o server_main server_main.cpp

record: record.h
	g++ -o record record.h

server: server.h
	g++ -o server server.h

clean:
	\rm server_main server record
