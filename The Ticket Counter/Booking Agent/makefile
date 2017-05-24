all: agent_driver
	./agent_driver 10.146.3.13

agent_driver: agent_driver.cpp booking_agent request train
	g++ -std=c++11 -o agent_driver agent_driver.cpp

request: ../request.h
	g++ -std=c++11 -o request ../request.h

train: ../train.h
	g++ -std=c++11 -o train ../train.h

booking_agent: booking_agent.h
	g++ -std=c++11 -o booking_agent booking_agent.h

clean:
	\rm agent_driver booking_agent request train
