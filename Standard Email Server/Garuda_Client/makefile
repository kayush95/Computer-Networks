run: garuda_client
	./garuda_client 127.0.0.1 xyz.com

garuda_client: garuda_client.cpp packet email SMTP_client POP_client
	g++ -o garuda_client garuda_client.cpp

email: ../email.h
	g++ -o email ../email.h

packet: ../packet.h
	g++ -o packet ../packet.h

SMTP_client: ../SMTP_client.h email packet
	g++ -o SMTP_client ../SMTP_client.h

POP_client: ../POP_client.h SMTP_client email packet
	g++ -o POP_client ../POP_client.h

clean:
	rm garuda_client packet email SMTP_client POP_client
