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

// SMTP_client.h
#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

#define MAX_SIZE 1024

using namespace std;

static struct termios old;
static struct termios _new;

/* Initialize _new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  _new = old; /* make _new settings same as old settings */
  _new.c_lflag &= ~ICANON; /* disable buffered i/o */
  _new.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &_new); /* use these _new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void) 
{
  return getch_(1);
}


class SMTP_Client
{
private:
	bool redirect_forwarding_mirroring_mode;
	struct email mail_to_redirect;

	char sender_domain[40];
	char sender[10];
	char domain[20]; // to be used in HELO

	int sockfd,portno;
	char* serverIP;
	struct sockaddr_in serv_addr;

	void error(const char* error_mssg)
    {
    	perror(error_mssg);
    	exit(1);
    }

public:
	SMTP_Client(char* serverIP,int portno)
	{
		redirect_forwarding_mirroring_mode = false;

		this->serverIP=strdup(serverIP);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if(sockfd<0)
			error("Error in opening SMTP_Client Socket");

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

	SMTP_Client(char* serverIP,int portno, struct email mail_to_redirect)
	{
		redirect_forwarding_mirroring_mode = true;
		this->mail_to_redirect = mail_to_redirect;

		char copy_domain_to[40];
		strcpy(copy_domain_to,mail_to_redirect.to);
		strtok(copy_domain_to," @");
		char* redirect_domain = strtok(NULL," @");
		strcpy(this->domain,redirect_domain);

		this->serverIP=strdup(serverIP);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if(sockfd<0)
			error("Error in opening SMTP_Client Socket");

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

		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error in connecting to SMTP Server !");
		else cout<<" Connected to the SMTP Server "<< serverIP<<endl;

		int retv = authorize();

		if(retv==-1)
		{
			cout << "Authorization failed\n\n";
			return;
		}

		establishConnection();

		if(!redirect_forwarding_mirroring_mode)
			createANDsendMailContent();
		else if(redirect_forwarding_mirroring_mode)
			sendMailContent();

		terminateConnection();

		close(sockfd);
	}


	void createANDsendMailContent()
	{
		int write_count,read_count;
		struct packet pkt;

		//system("clear");

		struct email input;

		strcpy(input.from,sender);
		strcat(input.from,"@");
		strcat(input.from,domain);

		do{
			cout << "\nEnter a valid Recepient mail address\n";
			cin.getline(input.to,40);
		}while(strchr(input.to,'@') == NULL);

		input.timestamp = time(0);

		cout << "\nEnter subject : ";
		cin.getline(input.subject,50);

		int sent_header=0;
		cout << "Sending envelope and header SMTP packets ...\n\n";

		do
		{
			// Sending MAIL_FROM
			pkt = packet(packet::MAIL_FROM,input.from);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing MAIL_FROM to Server !");
			else cout << "Sent MAIL_FROM to Server\n";

			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading OK from Server !");
			else if(pkt.command_code != packet::OK)
			{
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					sent_header=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}
			printf("SMTP Sever : OK %s\n",pkt.mssg);

			// Sending RCPT_TO
			pkt = packet(packet::RCPT_TO,input.to);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing RCPT_TO to Server !");
			else cout << "Sent RCPT_TO to Server\n";

			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading OK from Server !");
			else if(pkt.command_code != packet::OK)
			{
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					sent_header=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}
			printf("SMTP Sever : OK %s\n",pkt.mssg);



			// Sending DATA
			pkt = packet(packet::DATA,"data");
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing DATA to Server !");
			else cout << "Sent DATA to Server\n";

			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading start_mail_input from Server !");
			else if(pkt.command_code != packet::start_mail_input)
			{
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					sent_header=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}
			printf("SMTP Sever : start_mail_input %s\n",pkt.mssg);



			// Sending HEADER_FROM
			pkt = packet(packet::HEADER_FROM,input.from);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HEADER_FROM to Server !");
			else cout << "Sent HEADER_FROM to Server\n";
			

			// Sending HEADER_TO
			pkt = packet(packet::HEADER_TO,input.to);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HEADER_TO to Server !");
			else cout << "Sent HEADER_TO to Server\n";
			

			// Sending HEADER_DATE
			char input_date_str[100];
			sprintf(input_date_str,"%d",(int)input.timestamp);
			pkt = packet(packet::HEADER_DATE,input_date_str);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HEADER_DATE to Server !");
			else cout << "Sent HEADER_DATE to Server\n";
			

			// Sending HEADER_SUBJECT
			pkt = packet(packet::HEADER_SUBJECT,input.subject);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HEADER_SUBJECT to Server !");
			else cout << "Sent HEADER_SUBJECT to Server\n";
			
			// Sending BLANK
			pkt = packet(packet::BLANK,"blank line");
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing BLANK to Server !");
			else cout << "Sent BLANK to Server\n";
			
			sent_header=1;
			cout<<"\n\nEnvelope and header packets sent succesfully \n";

		}while(sent_header==0);

		cout << "Enter the email body (one sentence in each line (End line with a dot)\n\n";

		char body_line[100];
		do
		{
			cin.getline(body_line,100);

			// Sending BODY
			pkt = packet(packet::BODY,body_line);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing BODY to Server !");
			else cout << "Sent BODY line to Server\n";

		}while(strcmp(body_line,".")!=0);

		read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
		if(read_count<0) error("Error in reading OK from Server !");
		else if(pkt.command_code != packet::OK)
		{
			printf("SMTP Server : %s\n",pkt.mssg);
			if(pkt.command_code==packet::RSET)
			{
				error("Fatal error ! Email not sent !");
			}
			else
				error("Unsolvable Error !");
			
		}
		printf("SMTP Sever : OK %s\n",pkt.mssg);

		cout<<"\n\nBody packets sent succesfully \n";

	}

	void sendMailContent()
	{
		int write_count,read_count;
		struct packet pkt;

		//system("clear");

		struct email input = mail_to_redirect;

		int sent_header=0;
		cout << "Sending envelope and header SMTP packets ...\n\n";

		do
		{
			// Sending MAIL_FROM
			pkt = packet(packet::MAIL_FROM,input.from);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing MAIL_FROM to Server !");
			else cout << "Sent MAIL_FROM to Server\n";

			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading OK from Server !");
			else if(pkt.command_code != packet::OK)
			{
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					sent_header=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}
			printf("SMTP Sever : OK %s\n",pkt.mssg);

			// Sending RCPT_TO
			pkt = packet(packet::RCPT_TO,input.to);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing RCPT_TO to Server !");
			else cout << "Sent RCPT_TO to Server\n";

			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading OK from Server !");
			else if(pkt.command_code != packet::OK)
			{
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					sent_header=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}
			printf("SMTP Sever : OK %s\n",pkt.mssg);



			// Sending DATA
			pkt = packet(packet::DATA,"data");
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing DATA to Server !");
			else cout << "Sent DATA to Server\n";

			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading start_mail_input from Server !");
			else if(pkt.command_code != packet::start_mail_input)
			{
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					sent_header=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}
			printf("SMTP Sever : start_mail_input %s\n",pkt.mssg);



			// Sending HEADER_FROM
			pkt = packet(packet::HEADER_FROM,input.from);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HEADER_FROM to Server !");
			else cout << "Sent HEADER_FROM to Server\n";
			

			// Sending HEADER_TO
			pkt = packet(packet::HEADER_TO,input.to);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HEADER_TO to Server !");
			else cout << "Sent HEADER_TO to Server\n";
			

			// Sending HEADER_DATE
			char input_date_str[100];
			sprintf(input_date_str,"%d",(int)input.timestamp);
			pkt = packet(packet::HEADER_DATE,input_date_str);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HEADER_DATE to Server !");
			else cout << "Sent HEADER_DATE to Server\n";
			

			// Sending HEADER_SUBJECT
			pkt = packet(packet::HEADER_SUBJECT,input.subject);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HEADER_SUBJECT to Server !");
			else cout << "Sent HEADER_SUBJECT to Server\n";
			
			// Sending BLANK
			pkt = packet(packet::BLANK,"blank line");
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing BLANK to Server !");
			else cout << "Sent BLANK to Server\n";
			
			sent_header=1;
			cout<<"\n\nEnvelope and header packets sent succesfully \n";

		}while(sent_header==0);

		//cout << "Enter the email body (one sentence in each line (End line with a dot)\n\n";

		char* body_pointer = input.body; // place @ the beginning of body of the email
		int body_len = strlen(input.body);

		char body_line[100];
		do
		{
			//cin.getline(body_line,100);
			int copied_len = (body_len>99) ? 99 : body_len;
			strncpy(body_line,body_pointer,99);

			body_len -= copied_len;
			body_pointer += copied_len;

			// fetch the next 99 characters of email input

			// Sending BODY line
			pkt = packet(packet::BODY,body_line);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing BODY to Server !");
			else cout << "Sent BODY line to Server\n";

		}while( body_len > 0 );

		pkt = packet(packet::BODY,".");
		write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
		if(write_count<0) error("Error in writing BODY to Server !");
		else cout << "Sent BODY line to Server\n";


		read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
		if(read_count<0) error("Error in reading OK from Server !");
		else if(pkt.command_code != packet::OK)
		{
			printf("SMTP Server : %s\n",pkt.mssg);
			if(pkt.command_code==packet::RSET)
			{
				error("Fatal error ! Email not sent !");
			}
			else
				error("Unsolvable Error !");
			
		}
		printf("SMTP Sever : OK %s\n",pkt.mssg);

		cout<<"\n\nBody packets sent succesfully \n";
	}


	int authorize()
	{
		int write_count,read_count;
		struct packet pkt;

		if(redirect_forwarding_mirroring_mode)
		{
			cout << "Redirection started...\n";
			pkt = packet(packet::FRWD,"Redirecting / Forwarding / Mirroring");
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing FRWD to Server !");
			else cout << "Sent FRWD username to Server" << endl;

			return 0;

		}

		cout<<"Please enter full username : <user@domain.com> "<<endl;
		cin.getline(sender_domain,40);

		if(strchr(sender_domain,'@') == NULL)
		{
			cout << "Invalid email address !" << endl;
			return -1;
		}

		char* buf = strtok(sender_domain," @\n");
		
		strcpy(sender,buf);
		buf = strtok(NULL," @\n");
		strcpy(domain,buf);

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

		pkt = packet(packet::VRFY_USER,sender);
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

		printf("SMTP Sever : OK %s\n",pkt.mssg);

		pkt = packet(packet::VRFY_PWD,pwd);
		write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
		if(write_count<0) error("Error in writing VRFY_PWD to Server !");
		else cout << "Sent VRFY_PWD password to Server" << endl;


		read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
		if(read_count<0) error("Error in reading OK from Server !");
		else if(pkt.command_code != packet::OK)
		{
			printf("SMTP Sever : %s\n",pkt.mssg);
			return -1;
		}

		printf("SMTP Sever : OK %s\n",pkt.mssg);

		cout << "username and password verified\n";


	}

	void establishConnection()
	{
		int write_count,read_count;
		struct packet pkt;

		int established = 0;

		do
		{
			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading service_ready from Server !");
			else if(pkt.command_code != packet::service_ready)
			{
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					established=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}

			printf("SMTP Server : service_ready %s\n",pkt.mssg);


			pkt = packet(packet::HELO,domain);
			write_count = write(sockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing HELO to Server !");



			read_count=read(sockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading OK from Server !");
			else if(pkt.command_code != packet::OK)
			{
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					established=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}

			printf("SMTP Server : OK %s\n",pkt.mssg);

			if(pkt.command_code == packet::OK)
			{
				printf("Connection Established !\n\n\n\n");
				established=1;
			}


		}while(established==0);
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
				printf("SMTP Server : %s\n",pkt.mssg);
				if(pkt.command_code==packet::RSET)
				{
					terminated=0;
					continue;
				}
				else
					error("Unsolvable Error !");
				
			}

			printf("SMTP Server : service_closed %s\n",pkt.mssg);

			if(pkt.command_code == packet::service_closed)
			{
				printf("Connection Terminated !\n\n\n\n");
				terminated=1;
			}


		}while(terminated==0);
	}

/*
		void createAndSendNews()
	{

		

		struct record obj;
		int opt;

		cout<<" Welcome Reporter ! "<<endl;

	do{
		cout<<"\n Enter the option for the type of news you want to create: \n 1. ACADEMIC\n 2. NON-ACADEMIC\n";
		opt=getch();
		opt-='0';
	}while(opt!=1 && opt!=2);

		obj.type= (opt==1) ? 0:1;
		
		cout<<"Please enter the Headline !"<<endl;
		cin.getline(obj.headline,MAX_HEADLINE);
		cout<<" Please enter the news body !"<<endl;
		cin.getline(obj.body,MAX_BODY);
		obj.locked=false;
		time_t now = time(0);
   		obj.date_time=now;

   		int write_count=write(sockfd,(char*)&obj,sizeof(obj));

   		if(write_count<0) error("Error in sending news report to SMTP Server !");
   		else{
   			system("clear");
   			cout<<"\n\n A News report on \n" << obj.headline <<" \n has been succesfully sent to SMTP Server\n\n";
   		}

   		
	}
	*/
};


#endif