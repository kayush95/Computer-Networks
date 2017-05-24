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

#include "record.h"

// server.h
#ifndef SERVER_H
#define SERVER_H

#define MAX_SIZE 1024
#define MAX_BACKLOG 10

using namespace std;

class Server
{
protected:
	int sockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;	// server address and client address respectively

	Server(int portno,int protocol)
	{
		sockfd = socket(AF_INET, protocol, 0);

				if(sockfd<0)
			error("Error in opening TCP Server Socket !");

   	 bzero((char *) &serv_addr, sizeof(serv_addr));
	 this->portno = portno;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     //inet_pton(AF_INET, "127.0.2.83", &(serv_addr.sin_addr));	// for specifying a specific address
     serv_addr.sin_port = htons(portno);

     if (bind(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) 
      		 error("Error in binding the TCP Socket to the address !");
     
     clilen = sizeof(cli_addr);

	}

    void error(const char* error_mssg)
    {
    	perror(error_mssg);
    	exit(1);
    }

public:
    char* getIP()
    {
    	char ip[INET_ADDRSTRLEN];
    	if(inet_ntop(AF_INET,&(serv_addr.sin_addr),ip,INET_ADDRSTRLEN)==NULL) error("Error in returning IP Address !");
    	return strdup(ip);
    }

};

class TCPServer : public Server
{
public:
	const static int READER_CLIENT=0;
	const static int REPORTER_CLIENT=1;


	TCPServer(int port_num,int type) : Server(port_num,SOCK_STREAM)
	{
		client_type=type;

      	this->startListening();
	}

private:
	int newsockfd,pid,client_type;

	void startListening()
	{
		listen(sockfd,MAX_BACKLOG);

		//cout<< "My IP : "<< this->getIP() << " !" << endl;
		cout<<"\n TCP Server running...\n\n";
		while(1)
		{
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

			if (newsockfd < 0) error("Error on Accepting TCP Connection Request !");
      		else 
      			{
      				char cli_ip[INET_ADDRSTRLEN];
      				if(inet_ntop(AF_INET,&(cli_addr.sin_addr),cli_ip,INET_ADDRSTRLEN)==NULL) error("Error in determining client IP Address !");
    	
      				cout<<"@ port : " << portno << " TCP Connection made with the client " << cli_ip << " !\n\n";
    	  	}
    	  /* Create TCP child process */
			pid=fork();

			if(pid<0) error("Error on forking a new TCP Child Process !");
    	  	else if (pid==0)
    	  	{
    	  		// in child process
    	  		close(sockfd);
    	  		
    	  		if(client_type==REPORTER_CLIENT)
    	  			handleReporterClient(newsockfd);
    	  		else if(client_type==READER_CLIENT)
    	  			handleReaderClient(newsockfd);

    	  		exit(0);

    	  	}
    	  	else close(newsockfd);	// in parent process
      
		}
	}

	void handleReporterClient(int newsockfd)
	{
		int read_count;

		struct record obj;

		read_count=read(newsockfd,(char*)&obj,sizeof(obj));

		if(read_count<0) error("Error in reading from Reporter Client !");

		ofstream _out_("library.dat",ios::out | ios::app);

		_out_.write((char*)&obj,sizeof(obj));

		_out_.close();

		cout<< " TCP Sever has successfully saved the received news record to the library !\n\n";

	}

	void handleReaderClient(int newsockfd)
	{
		int write_count,news_counter=0;

		ifstream _in_("library.dat",ios::in);

		ofstream _out_("temporary.dat",ios::out | ios::app);

		struct record obj;

		while(!_in_.eof())
		{
			news_counter++;
			_in_.read((char*)&obj,sizeof(obj));

			_out_.write((char*)&obj,sizeof(obj));
		}
		_in_.close();
		_out_.close();
		
		ifstream _innew_("temporary.dat",ios::in);

		news_counter=0;
		while(!_innew_.eof())
		{
			news_counter++;
			_innew_.read((char*)&obj,sizeof(obj));

			write_count=write(newsockfd,(char*)&obj,sizeof(obj));
		}

		_innew_.close();
		
		system("rm temporary.dat");

		cout<< " TCP Sever has successfully transferred all the " << news_counter << " news reports to the client, after retrieval from the library !\n\n";
		
		
	}

};

class UDPServer: public Server
{
public:
	UDPServer(int port_num) : Server(port_num,SOCK_DGRAM)
	{
		handleAdminClient();
	}

private:
	bool adminFLAG;	// true means admin Connected

	int dateCompare(const char* d1, const char* d2)
	{
	    int dd1,dd2,mm1,mm2,yy1,yy2;
	    sscanf(d1, "%d/%d/%d", &dd1, &mm1, &yy1);
	    sscanf(d2, "%d/%d/%d", &dd2, &mm2, &yy2);
	    // Algorithm to find out which string is "lesser" than the other
	    if(yy1 > yy2)
		return 1;
	    else if(yy1 < yy2)
		return 0;
	    else if(yy1 == yy2)
		if(mm1 > mm2)
		    return 1;
		else if(mm1 < mm2)
		    return 0;
		else if(mm1 == mm2)
		    if(dd1 >= dd2)
		        return 1;
		    else
		        return 0;
	}

	void sendNewsReportstoAdmin()
	{
		int write_count,news_counter=0;

		ifstream _in_("library.dat",ios::in);

		struct record obj;

		while(!_in_.eof())
		{
			news_counter++;
			_in_.read((char*)&obj,sizeof(obj));

			write_count=sendto(sockfd, (char*)&obj, sizeof(obj), 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));
			if(write_count<0) error("Error in writing to Administrative Client\n");
		}

		_in_.close();

		cout<< " UDP Sever has successfully transferred all the " << news_counter << " news reports to the client, after retrieval from the library !\n\n";
			
	}
	
	void handleAdminClient()	
	{
		char mssg[10];
		int recv_len; 

		while(1)
		{
			cout<<"\nUDP Server waiting for start message...\n\n";
			if (recv_len = recvfrom(sockfd, mssg, 10, 0, (struct sockaddr *) &cli_addr, &clilen) == -1) error("Error in receiving from Administrative Client\n\n");
		
			sendNewsReportstoAdmin();

		
			cout<<"\nUDP Server waiting for expiry date...\n\n";
				//try to receive some data, this is a blocking call
	       ///////////////////////////////////////////////////////////////////////			
			//time_t dat_tym;
			char d[11];
			strcpy(d,"\0");
			
			/*if (recv_len = recvfrom(sockfd, (char*)&dat_tym, sizeof(dat_tym), 0, (struct sockaddr *) &cli_addr, &clilen) == -1) 
				error("Error in receiving from Administrative Client\n\n");
			else 
			cout << "UDP Server received the following date & time : " << ctime(&dat_tym) << "from " << inet_ntoa(cli_addr.sin_addr) << ":"<< ntohs(cli_addr.sin_port)<<"\n";*/
		
			if (recv_len = recvfrom(sockfd, d, sizeof(d), 0, (struct sockaddr *) &cli_addr, &clilen) == -1) 
				error("Error in receiving from Administrative Client\n\n");
			else 
				cout << "UDP Server received the following date & time : " << d << " from " << inet_ntoa(cli_addr.sin_addr) << ":"<< ntohs(cli_addr.sin_port)<<"\n";
              		

			if(strlen(d) != 10)exit(0);

		
		////////////////////////// Deletion /////////////////////////////
			int write_count,news_counter=0;
			char temp[11];

			ifstream _in_("library.dat",ios::in);

			ofstream _out_("del.dat",ios::out | ios::app);

			struct record obj;

			while(!_in_.eof())
			{
				news_counter++;
				_in_.read((char*)&obj,sizeof(obj));
				
				strcpy(temp, obj.date);
				if (dateCompare(d, temp) == 0)
				{				
					_out_.write((char*)&obj,sizeof(obj));
					
				}	
				else
					cout << "Deleting\n";
			}
			_in_.close();
			_out_.close();
			system("rm library.dat");
			cout << "Deleting done\n" ;
			cout << "Old library.dat removed\n" ;
			cout << "Renaming\n" ;
			if(rename("del.dat","library.dat") == 0)
				cout<<"Successfully Renamed\n";
			/*ifstream _innew_("del.dat",ios::in);
			ofstream _outnew_("library.dat",ios::out | ios::app);

			news_counter=0;
			while(!_outnew_.eof())
			{
				news_counter++;
				_innew_.read((char*)&obj,sizeof(obj));
				_outnew_.write((char*)&obj,sizeof(obj));
				cout << "Duplicating\n";		
			}
			cout << "Duplicating done\n" ;
			_innew_.close();
			_outnew_.close();
		
			system("rm del.dat");
			cout << "del.dat removed\n" ;*/
		///////////////////////////////////////////////////////////////////////
			close(sockfd);
			exit(0);
///////////////////////////////////////////////////////////////////////
     
		}
		
		
	}
};



#endif
