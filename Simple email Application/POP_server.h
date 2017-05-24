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

#include <algorithm>    // std::reverse
#include <vector>       // std::vector

// POP_Server.h
#ifndef POP_SERVER_H
#define POP_SERVER_H

#define MAX_SIZE 1024
#define MAX_BACKLOG 10

using namespace std;

class POP_Server
{
private:
	vector<struct email> retrieved_email_list;
	char _user_[10];
	char statString[50];
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

	POP_Server(int portno, char domain[20])
	{
	
		strcpy(this->domain,domain);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if(sockfd<0)
				error("Error in opening POP Server Socket !");

	   	 bzero((char *) &serv_addr, sizeof(serv_addr));
		 this->portno = portno;
	     serv_addr.sin_family = AF_INET;
	     serv_addr.sin_addr.s_addr = INADDR_ANY;
	     //inet_pton(AF_INET, "127.0.2.83", &(serv_addr.sin_addr));	// for specifying a specific address
	     serv_addr.sin_port = htons(portno);

	     if (bind(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) 
	      		 error("Error in binding the POP Socket to the address !");
	     
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
		cout<<"\n POP Server running ...\n\n";
		while(1)
		{
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

			if (newsockfd < 0) error("Error on accepting POP Connection Request !");
      		else 
      			{
      				char cli_ip[INET_ADDRSTRLEN];
      				if(inet_ntop(AF_INET,&(cli_addr.sin_addr),cli_ip,INET_ADDRSTRLEN)==NULL) error("Error in determining client IP Address !");
    	
      				cout<<"@ port : " << portno << " POP Connection made with the client " << cli_ip << " !\n\n";
    	  	}
    	  /* Create POP child process */
			pid=fork();

			if(pid<0) error("Error on forking a new POP Child Process !");
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

		showStats_listMails();

		terminateConnection();
	}

	int validate(char usr[40],char pwd[10])
	{
		// hardcoded but can also be generalized using registration / signup
		/*
		  Assume   that   there   are  
		four   users   of   abc.com:   Alice,   Arun,   Ananya,   and   Alex   and   four   users   of   xyz.com:   Bob,   Bilal,  
		Alex   and   Bernee.
		*/
		char temp_user_domain[40];
		strcpy(temp_user_domain,usr);
		char* buf = strtok(temp_user_domain," @");
		strcpy(_user_,buf);

		if(strcmp(domain,"abc.com")==0)
		{
			if(strcmp(usr,"Alice@abc.com")==0 && strcmp(pwd,"alicepwd")==0)
				return 1;
			else if(strcmp(usr,"Arun@abc.com")==0 && strcmp(pwd,"arunpwd")==0)
				return 1;
			else if(strcmp(usr,"Ananya@abc.com")==0 && strcmp(pwd,"ananyapwd")==0)
				return 1;
			else if(strcmp(usr,"Alex@abc.com")==0 && strcmp(pwd,"alexpwd")==0)
				return 1;
			else return 0;
		}
		else
			if(strcmp(domain,"xyz.com")==0)
			{
				if(strcmp(usr,"Bob@xyz.com")==0 && strcmp(pwd,"bobpwd")==0)
					return 1;
				else if(strcmp(usr,"Bilal@xyz.com")==0 && strcmp(pwd,"bilalpwd")==0)
					return 1;
				else if(strcmp(usr,"Bernee@xyz.com")==0 && strcmp(pwd,"berneepwd")==0)
					return 1;
				else if(strcmp(usr,"Alex@xyz.com")==0 && strcmp(pwd,"alexpwd")==0)
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

		char usr[40],pwd[10]; // Plz NOTE : a change in usr meaning : in POP it stores the whole usr@domain.com

		read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
		if(read_count<0) error("Error in reading VRFY_USER from Client !");
		else if(pkt.command_code != packet::VRFY_USER)
		{
			if(pkt.command_code == packet::FRWD)
			{
				// this means that this is a forwarded / redirected email.. no need for further authorization
				cout <<"POP Client : Redirected / Forwarded Email \n";
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
			printf("POP Client : VRFY_USER %s\n",pkt.mssg);
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
				printf("POP Client : VRFY_PWD %s\n",pkt.mssg);
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

	void showStats_listMails()
	{
		int write_count,read_count;
		struct packet pkt;

		int established = 0;

			do
			{
				
				read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
				if(read_count<0) error("Error in reading STAT from Client !");
				else if(pkt.command_code != packet::STAT)
				{
					pkt = packet(packet::RSET,"Reset");
					if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
					established=0;
					continue;
				}

				printf("POP Client : STAT %s\n",pkt.mssg);
				//if(strcmp(pkt.mssg,domain)!=0)	error("FATAL Error ! Domain name Invalid ! Exiting\n");
					
				if(!setStatString()) // if no mailbox exists
				{
					pkt = packet(packet::RSET,"Total : 0 emails, Size : 0 bytes");
					write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
					if(write_count<0) error("Error in writing Stats to Client !");
					return;
				}

				pkt = packet(packet::OK,statString);
				write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
				if(write_count<0) error("Error in writing Stats to Client !");
				

				read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
				if(read_count<0) error("Error in reading LIST from Client !");
				else if(pkt.command_code != packet::LIST)
				{
					pkt = packet(packet::RSET,"Reset");
					if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
					established=0;
					continue;
				}

				printf("POP Client : LIST %s\n",pkt.mssg);

				send_Email_List();

				read_count=read(newsockfd,(char*)&pkt,sizeof(pkt));
				if(read_count<0) error("Error in reading RETR from Client !");
				else if(pkt.command_code != packet::RETR)
				{
					pkt = packet(packet::RSET,"Reset");
					if( write_count=write(newsockfd,(char*)&pkt,sizeof(pkt)) < 0) error("Error in writing RESET to Client !");
					established=0;
					continue;
				}

				printf("POP Client : RETR %s\n",pkt.mssg);

				int _choice_;
				sscanf(pkt.mssg,"Retrieve %d",&_choice_);

				send_chosen_Mail(_choice_);

				//printf("Connection Established !\n\n\n\n");
				established=1;
				

			}while(established==0);

	}

	void send_Email_List()
	{
		int write_count,read_count;
		struct packet pkt;

		char filename[20];

		strcpy(filename,"mailbox_");
		strcat(filename,_user_);
		strcat(filename,".dat");

		ifstream _in_(filename,ios::in);

		struct email obj;

		retrieved_email_list.clear();

		while(_in_.read((char*)&obj,sizeof(obj)))
		{

			retrieved_email_list.push_back(obj);

			//write_count=write(newsockfd,(char*)&obj,sizeof(obj));

			//if(write_count<0) error("Error in writing to Reader Client !");
		}

		_in_.close();

		reverse(retrieved_email_list.begin(),retrieved_email_list.end());   // to bring the latest emails to the beginning 

		int counter = 0;
		for (vector<struct email>::iterator it=retrieved_email_list.begin(); it!= retrieved_email_list.end(); ++it)
		{
			counter++;
			char _mail_details_[100];
			sprintf(_mail_details_,"Email %d : SUBJECT : %s FROM : %s \n",counter,it->subject,it->from);
			pkt = packet(packet::DATA,_mail_details_);
			write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
			if(write_count<0) error("Error in writing Mail Data to Client !");
				
		}

		pkt = packet(packet::DATA,".");
		write_count = write(newsockfd,(char*)&pkt,sizeof(pkt));
		if(write_count<0) error("Error in writing Mail Data to Client !");
			
    
		printf(" POP Server has successfully sent the list of all the Mails of %s @ %s\n",_user_,domain);
		
	}

	void send_chosen_Mail(int _choice_)
	{
		int write_count,read_count;
		struct packet pkt;

		char filename[20];

		strcpy(filename,"mailbox_");
		strcat(filename,_user_);
		strcat(filename,".dat");

		ifstream _in_(filename,ios::in);

		struct email obj;

		retrieved_email_list.clear();

		while(_in_.read((char*)&obj,sizeof(obj)))
		{

			retrieved_email_list.push_back(obj);

			//write_count=write(newsockfd,(char*)&obj,sizeof(obj));

			//if(write_count<0) error("Error in writing to Reader Client !");
		}

		_in_.close();

		reverse(retrieved_email_list.begin(),retrieved_email_list.end());   // to bring the latest emails to the beginning 

		bool found = false;
		int counter = 0;
		for (vector<struct email>::iterator it=retrieved_email_list.begin(); it!= retrieved_email_list.end(); ++it)
		{
			counter++;
			if(counter == _choice_)
			{
				struct email eml = *it;
				write_count = write(newsockfd,(char*)&eml,sizeof(eml));
				if(write_count<0) error("Error in writing Mail Data to Client !");
				found = true;
				break;
			}
		}

		if(!found)
		{
			struct email eml;
			strcpy(eml.from,"-1");
			strcpy(eml.to,"-1");
			// send Error Email
			write_count = write(newsockfd,(char*)&eml,sizeof(eml));
			if(write_count<0) error("Error in writing Erroneous Mail Data to Client !");
				
			return;
		}

		printf(" POP Server has successfully sent the chosen Email to the Client\n");
		
	}

	bool setStatString()
	{	
		// enter the info into statString
		char filename[20];

		strcpy(filename,"mailbox_");
		strcat(filename,_user_);
		strcat(filename,".dat");

		//cout << "Filename : " << filename << endl;

		ifstream _in_(filename,ios::in);

		if(!_in_.is_open())
		{	
			cout << "Sorry ! No emails or Mailbox exists !\n";
			return false;
		}

		struct email obj;
		int email_counter=0,size_counter=0;

		while(_in_.read((char*)&obj,sizeof(obj)))
		{
		//	cout << "Hi\n";
			email_counter++;
			
			size_counter += strlen(obj.from) + strlen(obj.to) + strlen(obj.subject) + strlen(obj.body) + sizeof(obj.timestamp);
			// size_counter no. of bytes

			//write_count=write(newsockfd,(char*)&obj,sizeof(obj));

			//if(write_count<0) error("Error in writing to Reader Client !");
		}

		_in_.close();

		printf(" POP Server has successfully collected all the statistics of the mailbox of %s @ %s\n",_user_,domain);
		
		sprintf(statString,"Total : %d emails, Size : %d bytes\n",email_counter,size_counter);
		return true;
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

			printf("POP Client : QUIT %s\n",pkt.mssg);

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


};



#endif
