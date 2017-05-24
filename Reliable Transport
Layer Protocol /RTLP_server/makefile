all: server_driver
	sudo ./server_driver 10.105.75.45 13000 10.146.3.13 12000 

server_driver: server_driver.cpp server
	g++ -std=c++11 -o server_driver server_driver.cpp

rtlp: ../rtlp.h
	g++ -std=c++11 -o rtlp ../rtlp.h

server: server.h rtlp
	g++ -std=c++11 -o server server.h

clean:
	rm server_driver server rtlp
