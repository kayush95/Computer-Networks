all: server_driver
	 ./server_driver

server_driver: server_driver.cpp server request train
	g++ -std=c++11 -S server_driver.cpp
	g++ -std=c++11 -o server_driver server_driver.s 

request: ../request.h
	g++ -std=c++11 -o request ../request.h

train: ../train.h
	g++ -std=c++11 -o train ../train.h

server: server.h
	g++ -std=c++11 -o server server.h

clean:
	\rm server_driver server request train
