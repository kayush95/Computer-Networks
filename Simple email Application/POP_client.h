#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fstream>

#include <termios.h>
#include <stdio.h>
#include <ctype.h>

#include "email.h"
#include "packet.h"
#include "SMTP_client.h"

// POP_client.h
#ifndef POP_CLIENT_H
#define POP_CLIENT_H

#define MAX_SIZE 1024

using namespace std;

class POP_Client
{
private:

	char sender_domain[40];

	int sockfd,portno;
	char* serverIP;
	struct sockaddr_in serv_addr;

	void error(const char* error_mssg)
    {
    	perror(error_mssg);
    	exit(1);
    }

public:
	POP_Client(char* serverIP,int portno)
	{
		
		this->serverIP=strdup(serverIP);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if(sockfd<0)
			error("Error in opening POP_Client Socket");

		this->portno=portno;

		bzero((char *) &serv_addr, sizeof(serv_addr));
 		serv_addr.sin_family = AF_INET;
 		serv_addr.sin_port = htons(portno);

 		/* either this
		struct hostent* server;
 		server = gethostbyname(serverIP);	// if serverIP is a DNS name // need to insert a new mapping in /etc/hosts file
  		 if (server == NULL) {
   		    fprintf(stderr,"ERROR, no such host\n");
   		    exit(0);
    	}
 	    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    	*/


 	    // OR this 
	//	if serverIP is an IP address in dot notation
		inet_pton(AF_INET, serverIP, &(serv_addr.sin_addr));
		
	}

	
	void run()
	{

		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error in connecting to POP Server !");
		else cout<<" Connected to the POP Server "<< serverIP<<endl;

		int retv = authorize();

		if(retv==-1)
		{
			cout << "Authorization failed\n\n";
			return;
		}

		getStats_Mails();

		terminateConnection();

		close(sockfd);
	}

	int authorize()
	{
		int write_count,read_count;
		struct packet pkt;

		cout<<"Please enter full username : <user@domain.com> "<<endl;
		cin.getline(sender_domain,40);

		if(strchr(sender_domain,'@') == NULL)
		{
			cout << "Invalid email address !" << endl;
			return -1;
		}

		/*char* buf = strtok(sender_domain," @\n");
		
		strcpy(sender,buf);
		buf = strtok(NULL," @\n");
		strcpy(domain,buf);*/

		char pwd[10];

		cout<<" Please enter password "<<endl;
	//	cin.getline(pwd,10);

		  string pass ="";
                      char pass_ch;
                      pass_ch = getch();
                          while(pass_ch != '\n')
                          {//character 13 is enter
                             pass.push_back(pass_ch);
                             cout << '*';
                             pass_ch = getch();
                          }

        strcpy(pwd,pass.c_str());

        cout << "\n\nStarting authorization..." <<endl;

		// Now, we have sender[10] and pwd[10]

		pkt = packet(packet::VRFY_USER,sender_domain);
		write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
		if(write_count<0) error("Error in writing VRFY_USER to Server !");
		else cout << "Sent VRFY_USER username to Server" << endl;


		read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
		if(read_count<0) error("Error in reading OK from Server !");
		else if(pkt.command_code != packet::OK)
		{
			cout << "Error : didn't receive username ACK from Server" << endl;
			return -1;
		}

		printf("POP Sever : OK %s\n",pkt.mssg);

		pkt = packet(packet::VRFY_PWD,pwd);
		write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
		if(write_count<0) error("Error in writing VRFY_PWD to Server !");
		else cout << "Sent VRFY_PWD password to Server" << endl;


		read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
		if(read_count<0) error("Error in reading OK from Server !");
		else if(pkt.command_code != packet::OK)
		{
			printf("POP Sever : %s\n",pkt.mssg);
			return -1;
		}

		printf("POP Sever : OK %s\n",pkt.mssg);

		cout << "username and password verified\n";


	}

	void getStats_Mails()
	{
		int write_count,read_count;
		struct packet pkt;

		int established = 0;

		do
		{
			pkt = packet(packet::STAT,"show Mailbox Stats");
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing STAT to Server !");
			else cout << "Requested Server for Stats\n";


			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading Stats from Server !");
			else if(pkt.command_code != packet::OK)
			{
				printf("POP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					printf("\nSorry ! Empty Mailbox : %s\n\n\n",pkt.mssg);
					established=0;
					return;
				}
				else
					error("Fatal Error !");
				
			}

			printf("POP Server : OK %s\n",pkt.mssg);

			if(pkt.command_code == packet::OK)
			{
				printf("\nYour Mailbox Statistics : %s\n\n\n",pkt.mssg);
				//established=1;
			}

			pkt = packet(packet::LIST,"list all emails");
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing LIST to Server !");
			else cout << "Requested Server for List of mails\n";

			receive_Email_List();

			int _choice_;

			cout << "\nEnter the no. of Email you want to read\n";
			cin >> _choice_;
			char choice_str[15];
			sprintf(choice_str,"Retrieve %d",_choice_);

			pkt = packet(packet::RETR,choice_str);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing LIST to Server !");
			else cout << "Requested Server for retrieval of Email no. " << _choice_ << "\n";

			receive_Display_Chosen_Email();

			established = 1;

		}while(established==0);

		getch();
	}

	void receive_Email_List()
	{
		int write_count,read_count;
		struct packet pkt;

		do
		{

			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading list from Server !");
			else if(pkt.command_code != packet::DATA)
			{
				printf("POP Server : %s\n",pkt.mssg);
				error("Fatal Error !");	
			}	

			if(strcmp(pkt.mssg,".")!=0)
				cout << "POP Server : " << pkt.mssg << endl;	

		}while(strcmp(pkt.mssg,".")!=0);
	}

	void receive_Display_Chosen_Email()
	{
		struct email _display_;

		int read_count=read(sockfd,(char*)&_display_,sizeof(_display_));
		if(read_count<0) error("Error in reading Email from Server !");
		
		if(strcmp(_display_.to,"-1")==0)
			cout << "Error : Email not found on Server\n";
		else
		{
			cout << "POP Server has sent the following email : " << endl;
			_display_.print();
		}


	}

	void terminateConnection()
	{
		int write_count,read_count;
		struct packet pkt;

		int terminated = 0;

		do
		{
			pkt = packet(packet::QUIT,"quit");
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing QUIT to Server !");
			else cout << "Sent QUIT to Server\n";


			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading service_closed from Server !");
			else if(pkt.command_code != packet::service_closed)
			{
				printf("POP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					terminated=0;
					continue;
				}
				else
					error("Fatal Error !");
				
			}

			printf("POP Server : service_closed %s\n",pkt.mssg);

			if(pkt.command_code == packet::service_closed)
			{
				printf("Connection Terminated !\n\n\n\n");
				terminated=1;
			}


		}while(terminated==0);
	}


};


#endif