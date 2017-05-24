all: client_driver
	sudo ./client_driver 10.146.3.13 12000 10.105.75.45 13000 
	
client_driver: client_driver.cpp client
	g++ -std=c++11 -o client_driver client_driver.cpp

client: client.h rtlp
	g++ -std=c++11 -o client client.h

rtlp: ../rtlp.h
	g++ -std=c++11 -o rtlp ../rtlp.h

clean:
	rm client_driver client rtlp