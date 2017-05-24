auto_enter: client_main
	./client_main 10.147.148.49 6500 7000  < input.txt

run: client_main
	./client_main 10.147.148.49 6500 7000

client_main: client_main.cpp client record
	g++ -o client_main client_main.cpp

record: record.h
	g++ -o record record.h

client: client.h
	g++ -o client client.h

clean:
	\rm client_main client record
