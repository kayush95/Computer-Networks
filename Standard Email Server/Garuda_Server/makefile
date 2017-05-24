run: garuda_server
	./garuda_server xyz.com 10.42.0.22

garuda_server: garuda_server.cpp packet email SMTP_server POP_server
	g++ -o garuda_server garuda_server.cpp

email: ../email.h
	g++ -o email ../email.h

packet: ../packet.h
	g++ -o packet ../packet.h

SMTP_server: ../SMTP_server.h  SMTP_client email packet
	g++ -o SMTP_server ../SMTP_server.h

SMTP_client: ../SMTP_client.h email packet
	g++ -o SMTP_client ../SMTP_client.h

POP_server: ../POP_server.h email packet
	g++ -o POP_server ../POP_server.h

clean:
	rm garuda_server packet email SMTP_server SMTP_client POP_server
