#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>

#include "email.h"
#include "packet.h"
#include "SMTP_client.h"

// SMTP_server.h
#ifndef SMTP_SERVER_H
#define SMTP_SERVER_H

#define MAX_SIZE 1024
#define MAX_BACKLOG 10

#define FIRST_SERVER_PORT__SMTP  6000
#define FIRST_SERVER_PORT__POP   7000

#define SECOND_SERVER_PORT__SMTP  8000
#define SECOND_SERVER_PORT__POP   9000

using namespace std;

class SMTP_Server
{
private:
	char email_redirection_IP[20]; // abc.com redirects to xyz.com server and vice versa

	char domain[20];	// example : abc.com or xyz.com
	int sockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;	// server address and client address respectively

    void error(const char* error_mssg)
    {
    	perror(error_mssg);
    	exit(1);
    }

public:

	SMTP_Server(int portno, char domain[20],char email_redirection_IP[20])
	{
		strcpy(this->email_redirection_IP,email_redirection_IP);
		strcpy(this->domain,domain);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if(sockfd<0)
				error("Error in opening SMTP Server Socket !");

	   	 bzero((char *) &serv_addr, sizeof(serv_addr));
		 this->portno = portno;
	     serv_addr.sin_family = AF_INET;
	     serv_addr.sin_addr.s_addr = INADDR_ANY;
	     //inet_pton(AF_INET, "127.0.2.83", &(serv_addr.sin_addr));	// for specifying a specific address
	     serv_addr.sin_port = htons(portno);

	     if (bind(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) 
	      		 error("Error in binding the SMTP Socket to the address !");
	     
	     clilen = sizeof(cli_addr);

	     this->startListening();

	}

    char* getIP()
    {
    	char ip[INET_ADDRSTRLEN];
    	if(inet_ntop(AF_INET,&(serv_addr.sin_addr),ip,INET_ADDRSTRLEN)==NULL) error("Error in returning IP Address !");
    	return strdup(ip);
    }


private:
	int newsockfd,pid;

	void startListening()
	{
		listen(sockfd,MAX_BACKLOG);

		//cout<< "My IP : "<< this->getIP() << " !" << endl;
		cout<<"\n SMTP Server running ...\n\n";
		while(1)
		{
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

			if (newsockfd < 0) error("Error on accepting SMTP Connection Request !");
      		else 
      			{
      				char cli_ip[INET_ADDRSTRLEN];
      				if(inet_ntop(AF_INET,&(cli_addr.sin_addr),cli_ip,INET_ADDRSTRLEN)==NULL) error("Error in determining client IP Address !");
    	
      				cout<<"@ port : " << portno << " SMTP Connection made with the client " << cli_ip << " !\n\n";
    	  	}
    	  /* Create SMTP child process */
			pid=fork();

			if(pid<0) error("Error on forking a new SMTP Child Process !");
    	  	else if (pid==0)
    	  	{
    	  		// in child process
    	  		close(sockfd);
    	  		
    	  		run(); // using newsockfd

    	  		close(newsockfd);
    	  		exit(0);

    	  	}
    	  	else close(newsockfd);	// in parent process
      
		}
	}

	void run()
	{

		int retv = authorize();

		if(retv==-1)
			return;

		establishConnection();

		struct email email_received = receiveMailContent();	

		char received_buf[40];
		strcpy(received_buf,email_received.to);
		char* email_recipient_ = strtok(received_buf," @\t\n");
		char* email_domain = strtok(NULL," @\t\n");
		printf("Extracted recipient domain %s from the received email !\n", email_domain);

		if(strcmp(email_domain,"abc.com")!=0 && strcmp(email_domain,"xyz.com")!=0)
			error("Discarding email.. Neither belongs to abc.com nor to xyz.com");
		else
		{
			if(strcmp(email_domain,this->domain)==0)
			{
				// store on this server's mailbox
				//printf("I will store this message on my mailbox...\n");

				char filename[20];

				strcpy(filename,"mailbox_");
				strcat(filename,email_recipient_);
				strcat(filename,".dat");
			
				ofstream _out_(filename,ios::out | ios::app);

				_out_.write((char*)&email_received,sizeof(email_received));

				_out_.close();

				printf("\n\n Garuda Sever has successfully saved the received email to the mailbox of %s @ %s server !\n\n",email_recipient_,this->domain);

			}
			else
			{
				//printf("I will act as an SMTP Client and redirect this message to %s SMTP server...\n",email_domain);
			
				SMTP_Client *redirect_client = NULL;
				
				// start an SMTP client and redirect to the other SMTP server
				 	if(strcmp(email_domain,"abc.com")==0)
		          	 redirect_client=new SMTP_Client(email_redirection_IP,FIRST_SERVER_PORT__SMTP,email_received);
		          	else if(strcmp(email_domain,"xyz.com")==0)
		          	 redirect_client=new SMTP_Client(email_redirection_IP,SECOND_SERVER_PORT__SMTP,email_received);
		          	else 
		          		error("Redirection failed, server other than abc.com and xyz.com");
		         
		           if(redirect_client!=NULL) redirect_client->run();
		         printf("\n\n Garuda Sever has successfully redirected the received email to the %s server !\n\n",email_domain);
	
			}
		}	

		terminateConnection();
	}

	int validate(char usr[10],char pwd[10])
	{
		// hardcoded but can also be generalized using registration / signup
		/*
		  Assume   that   there   are  
		four   users   of   abc.com:   Alice,   Arun,   Ananya,   and   Alex   and   four   users   of   xyz.com:   Bob,   Bilal,  
		Alex   and   Bernee.
		*/

		if(strcmp(domain,"abc.com")==0)
		{
			if(strcmp(usr,"Alice")==0 && strcmp(pwd,"alicepwd")==0)
				return 1;
			else if(strcmp(usr,"Arun")==0 && strcmp(pwd,"arunpwd")==0)
				return 1;
			else if(strcmp(usr,"Ananya")==0 && strcmp(pwd,"ananyapwd")==0)
				return 1;
			else if(strcmp(usr,"Alex")==0 && strcmp(pwd,"alexpwd")==0)
				return 1;
			else return 0;
		}
		else
			if(strcmp(domain,"xyz.com")==0)
			{
				if(strcmp(usr,"Bob")==0 && strcmp(pwd,"bobpwd")==0)
					return 1;
				else if(strcmp(usr,"Bilal")==0 && strcmp(pwd,"bilalpwd")==0)
					return 1;
				else if(strcmp(usr,"Bernee")==0 && strcmp(pwd,"berneepwd")==0)
					return 1;
				else if(strcmp(usr,"Alex")==0 && strcmp(pwd,"alexpwd")==0)
					return 1;
				else return 0;
			}
			else
				return 0;
	}

	int authorize()
	{
		int write_count,read_count;
		struct packet pkt;

		char usr[10],pwd[10];

		read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
		if(read_count<0) error("Error in reading VRFY_USER from Client !");
		else if(pkt.command_code != packet::VRFY_USER)
		{
			if(pkt.command_code == packet::FRWD)
			{
				// this means that this is a forwarded / redirected email.. no need for further authorization
				cout <<"SMTP Client : Redirected / Forwarded Email \n";
				return 0;
			}

			pkt = packet(packet::service_closed,"expected Username !");
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing service_closed to Client !");
			else
			{
				printf("Connection terminated !\n");
				return -1;
			}

		}
		else
		{
			printf("SMTP Client : VRFY_USER %s\n",pkt.mssg);
			strcpy(usr,pkt.mssg);

			pkt = packet(packet::OK,"OK");
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing OK to Client !");
			else printf("Sent OK to Client !\n");
			
		}
		
		read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
		if(read_count<0) error("Error in reading VRFY_PWD from Client !");
		else if(pkt.command_code != packet::VRFY_PWD)
		{
			pkt = packet(packet::service_closed,"expected Password !");
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing service_closed to Client !");
			else
			{
				printf("Connection terminated !\n");
				return -1;
			}

		}
		else 
			{
				printf("SMTP Client : VRFY_PWD %s\n",pkt.mssg);
				strcpy(pwd,pkt.mssg);

				if(validate(usr,pwd))
				{
					pkt = packet(packet::OK,"OK");
						write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
						if(write_count<0) error("Error in writing OK to Client !");
						else printf("Sent OK to Client !\n");

					cout << "Username and Password authorized \n";
					return 0;
				}
				else
				{
					cout << "Authorization failed !"<< endl;
					pkt = packet(packet::service_closed,"wrong username or password !");
					write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
					if(write_count<0) error("Error in writing service_closed to Client !");
					else
					{
						printf("Connection terminated !\n");
						return -1;
					}
				}

				
			}



	}

	void establishConnection()
	{
		int write_count,read_count;
		struct packet pkt;

		int established = 0;

			do
			{

				pkt = packet(packet::service_ready,"Service Ready");
				write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
				if(write_count<0) error("Error in writing service_ready to Client !");

				
				read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
				if(read_count<0) error("Error in reading HELO from Client !");
				else if(pkt.command_code != packet::HELO)
				{
					pkt = packet(packet::RSET,"Reset");
					if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
					established=0;
					continue;
				}

				printf("SMTP Client : HELO %s\n",pkt.mssg);
				if(strcmp(pkt.mssg,domain)!=0)	error("FATAL Error ! Domain name Invalid ! Exiting\n");
					
				pkt = packet(packet::OK,"OK");
				write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
				if(write_count<0) error("Error in writing OK to Client !");
				else{
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

		do{
			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading QUIT from Client !");
			else if(pkt.command_code != packet::QUIT) 
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				terminated = 0;
				continue;
			}

			printf("SMTP Client : QUIT %s\n",pkt.mssg);

			pkt = packet(packet::service_closed,"Service Closed");
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing service_closed to Client !");
			else
			{
				terminated=1;
				printf("Connection terminated !\n\n\n\n");
			}

		}while(terminated==0);
	}

	struct email receiveMailContent()
	{
		int write_count,read_count;
		struct packet pkt;
		struct email return_email;

		int contentScanned = 0;

		do{
			// Receiving MAIL_FROM
			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading MAIL_FROM from Client !");
			else if(pkt.command_code != packet::MAIL_FROM)
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				contentScanned=0;
				continue;
			}

			printf("SMTP Client : MAIL_FROM %s\n",pkt.mssg);
			strcpy(return_email.from,pkt.mssg);

			pkt = packet(packet::OK,"OK");
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing OK to Client !");




			// Receiving RCPT_TO
			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading RCPT_TO from Client !");
			else if(pkt.command_code != packet::RCPT_TO)
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				contentScanned=0;
				continue;
			}

			printf("SMTP Client : RCPT_TO %s\n",pkt.mssg);
			strcpy(return_email.to,pkt.mssg);

			pkt = packet(packet::OK,"OK");
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing OK to Client !");






			// Receiving DATA
			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading DATA from Client !");
			else if(pkt.command_code != packet::DATA)
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				contentScanned=0;
				continue;
			}

			printf("SMTP Client : DATA %s\n",pkt.mssg);
			//strcpy(return_email.to,pkt.mssg);

			pkt = packet(packet::start_mail_input,"Start Mail Input");
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing start_mail_input to Client !");



			// Receiving HEADER and the BODY
			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading HEADER_FROM from Client !");
			else if(pkt.command_code != packet::HEADER_FROM)
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				contentScanned=0;
				continue;
			}
			printf("SMTP Client : HEADER_FROM %s\n",pkt.mssg);

			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading HEADER_TO from Client !");
			else if(pkt.command_code != packet::HEADER_TO)
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				contentScanned=0;
				continue;
			}
			printf("SMTP Client : HEADER_TO %s\n",pkt.mssg);

			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading HEADER_DATE from Client !");
			else if(pkt.command_code != packet::HEADER_DATE)
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				contentScanned=0;
				continue;
			}
			printf("SMTP Client : HEADER_DATE %s\n",pkt.mssg);
		 	int int_timestamp;
		 	sscanf(pkt.mssg,"%d",&int_timestamp);
		 	return_email.timestamp = (time_t) int_timestamp;

			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading HEADER_SUBJECT from Client !");
			else if(pkt.command_code != packet::HEADER_SUBJECT)
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				contentScanned=0;
				continue;
			}
			printf("SMTP Client : HEADER_SUBJECT %s\n",pkt.mssg);
			strcpy(return_email.subject,pkt.mssg);

			read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
			if(read_count<0) error("Error in reading BLANK LINE from Client !");
			else if(pkt.command_code != packet::BLANK)
			{
				pkt = packet(packet::RSET,"Reset");
				if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
				contentScanned=0;
				continue;
			}

			printf("SMTP Client : Blank Line %s\n",pkt.mssg);
			
			
			strcpy(return_email.body,"");
			do
			{
				read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
				if(read_count<0) error("Error in reading BODY from Client !");
				else if(pkt.command_code != packet::BODY)
				{
					pkt = packet(packet::RSET,"Reset");
					if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
					contentScanned=0;
					continue;
				}

				printf("SMTP Client : Body continuing... %s\n",pkt.mssg);
				if(strcmp(pkt.mssg,".")!=0) 
					{
						strcat(return_email.body," ");
						strcat(return_email.body,pkt.mssg);
					}

			}while(strcmp(pkt.mssg,".")!=0);

			pkt = packet(packet::OK,"OK");
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing OK to Client !");
			else{
				printf("Email content successfully scanned !\n");
				contentScanned=1;
			}

			

		}while(contentScanned==0);

		return return_email;

	}
/*
	void handleReporterClient(int newsockfd)
	{
		int read_count;

		struct record obj;

		read_count=read(newsockfd,(char*)&obj,sizeof(obj));

		if(read_count<0) error("Error in reading from Reporter Client !");

		ofstream _out_("library.dat",ios::out | ios::app);

		_out_.write((char*)&obj,sizeof(obj));

		_out_.close();

		cout<< " SMTP Sever has successfully saved the received news record to the library !\n\n";

	}

	void handleReaderClient(int newsockfd)
	{
		int write_count,news_counter=0;

		ifstream _in_("library.dat",ios::in);

		struct record obj;

		while(!_in_.eof())
		{
			news_counter++;
			_in_.read((char*)&obj,sizeof(obj));

			write_count=write(newsockfd,(char*)&obj,sizeof(obj));

			if(write_count<0) error("Error in writing to Reader Client !");
		}

		_in_.close();

		cout<< " SMTP Sever has successfully transferred all the " << news_counter << " news reports to the client, after retrieval from the library !\n\n";
		
	}
	*/

};



#endif
