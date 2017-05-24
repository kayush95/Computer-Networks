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

#include "record.h"

// client.h
#ifndef CLIENT_H
#define CLIENT_H

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


class Client
{
protected:
	int sockfd,portno;
	char* serverIP;
	struct sockaddr_in serv_addr;

	void error(const char* error_mssg)
    {
    	perror(error_mssg);
    	exit(1);
    }

	Client(char* serverIP,int portno,int protocol)
	{
		this->serverIP=strdup(serverIP);
		sockfd = socket(AF_INET, protocol, 0);
		cout<<"FDi:"<<sockfd<<endl;
		if(sockfd<0)
			error("Error in opening Client Socket");

		this->portno=portno;

		bzero((char *) &serv_addr, sizeof(serv_addr));
 		serv_addr.sin_family = AF_INET;
 		serv_addr.sin_port = htons(portno);

 		/* either this
		struct hostent* server;
 		server = gethostbyname(serverIP);	// if serverIP is a DNS name
  		 if (server == NULL) {
   		    fprintf(stderr,"ERROR, no such host\n");
   		    exit(0);
    	}
 	    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    	*/


 	    // OR this if serverIP is an IP address in dot notation
		inet_pton(AF_INET, serverIP, &(serv_addr.sin_addr));
		//

	}

public:
	virtual void run()
	{

	}
	
};

class Reporter : public Client
{
public:
	Reporter(char* serverIP,int port_num ) : Client(serverIP,port_num,SOCK_STREAM)
	{
	}

	void run()
	{
		createAndSendNews();
	}

	void createAndSendNews()
	{

		system("clear");

		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error in connecting to TCP Server !");
		else cout<<" Connected to the TCP Server "<< serverIP<<endl;
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
		////////////////////////////////////
		cout<<"Please enter the Date !"<<endl;
		char da[11];
		cin>>da;
		strcpy(obj.date,da);
		///////////////////////////////////

   		int write_count=write(sockfd,(char*)&obj,sizeof(obj));

   		if(write_count<0) error("Error in sending news report to TCP Server !");
   		else{
   			system("clear");
   			cout<<"\n\n A News report on \n" << obj.headline <<" \n has been succesfully sent to TCP Server\n\n";
   		}

   		close(sockfd);
	}
};

class Reader: public Client
{
private:
	char* createFile(struct record report)	// returns the name of the temporary file_name
	{
		ofstream out;
		out.open("temp.txt",ios::out | ios::trunc);
		out << "\n HEADLINE : " << report.headline << "\n\n";
		char* dt = ctime(&report.date_time);
		//out << "DATE & TIME of Creation : " << dt;
		out << "DATE & TIME of Creation : " << report.date;
		out << "\n NEWS BODY : \n" << report.body << "\n\n";
		out.close();
		return strdup("temp.txt");

	}

public:
	Reader(char* serverIP,int port_num ) : Client(serverIP,port_num,SOCK_STREAM)
	{
		
	}

	void run()
	{	
		browseThroughNews();
	}

	void browseThroughNews()
	{

		system("clear");
     		
		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error in connecting to TCP Server !");
		else cout<<" Connected to the TCP Server "<< serverIP<<endl;
		struct record news_array[MAX_SIZE];
		int arr_counter=0;

		struct record obj;
		int opt=0;

		cout<<" Welcome Reader ! "<<endl;

		do{
		cout<<"\n Enter the option for the type of news you wanna browse through: \n 1. ACADEMIC\n 2. NON-ACADEMIC\n";
		opt=getch();
		opt-='0';
		}while(opt!=1 && opt!=2);
		//obj.type= (opt==1) ? record::ACADEMIC : record::NON_ACADEMIC;
		
   		int read_count=read(sockfd,(char*)&obj,sizeof(obj));

   		while(read_count>0)
		{
			
   			if( (opt==1 && obj.type==record::ACADEMIC) || (opt==2 && obj.type==record::NON_ACADEMIC) )
   			news_array[arr_counter++]=record(obj);
   			read_count=read(sockfd,(char*)&obj,sizeof(obj));

   		}

   		if(arr_counter==0)
   		{
   			cout<<"\n\nNo news entries published in this domain !\n"<<endl;
   			close(sockfd);
   			return;
   		}

   		system("clear");

   		cout<<"\nRetrieved "<<arr_counter<<" entries from the TCP Server \n\n";

   		for(int i=arr_counter-1;i>=0;i--)
   		{
   			cout<<"------------------------------------------------------------------\n";
   			cout<<" NEWS HEADLINE "<< (i+1) << " : " << news_array[i].headline << endl;
   			//cout<<" News Body : \n" << news_array[i].body <<endl;
   			char* dt = ctime(&news_array[i].date_time);
////////////////////////////////////////////////
   			//cout<<"\n DATED : " << dt <<endl;
			cout<<"\n DATED : " << news_array[i].date <<endl;
/////////////////////////////////////////////////

   		}
   		cout<<"------------------------------------------------------------------\n\n\n";

   		do
		{
   			cout<<"Please enter the headline index of the news item you want to be displayed\n\n";
   			opt=getch();
   			opt-='0';
   		}
		while(opt<1 || opt>arr_counter);

   		int pid=fork();

   		if(pid==0)
   		{
	   		close(sockfd);

	   		char* filename = createFile(news_array[opt-1]);

	   		//display the contents of the (opt-1)th news
	   		int rtv=execlp("xterm","xterm","-hold","-e","more",filename,(const char*)NULL);

	   		if(rtv==-1)
	   			error("Error in executing xterm !\n\n");
   		
   		}
   		wait(NULL);

   		close(sockfd);
	}
};

class Administrator: public Client
{
public:
	Administrator(char* serverIP,int port_num ) : Client(serverIP,port_num,SOCK_DGRAM)
	{
		
	}

	void run()
	{
		browseThroughNews();
		specifyExpiryDateLimit();
	}

	void specifyExpiryDateLimit()
	{
	  cout << "\n Please enter an expiry date for updating the news.\n\n";
		
	  /*time_t rawtime;
	  struct tm * timeinfo;
	  int year, month ,day;
	  const char * weekday[] = { "Sunday", "Monday",
		                     "Tuesday", "Wednesday",
		                     "Thursday", "Friday", "Saturday"};

	  // prompt user for date 
	  printf ("Enter year: "); fflush(stdout); scanf ("%d",&year);
	  printf ("Enter month: "); fflush(stdout); scanf ("%d",&month);
	  printf ("Enter day: "); fflush(stdout); scanf ("%d",&day);

	  // get current timeinfo and modify it to the user's choice 
	  time ( &rawtime );
	  timeinfo = localtime ( &rawtime );
	  timeinfo->tm_year = year - 1900;
	  timeinfo->tm_mon = month - 1;
	  timeinfo->tm_mday = day;

	  // call mktime: timeinfo->tm_wday will be set 
	  time_t _send_ = mktime ( timeinfo );*/
//////////////////////////////////////////////////////////////////
          char d[11];
	  strcpy(d,"\0");
	  cout<<"Enter Date:"<<endl;
	  cin>>d;

	  while (strlen(d) != 10)
  	  {
		cout<<"Wrong!\nEnter Again:"<<endl;
	  	cin>>d;
	  }

	  //cout << "Setting expiry date : " << ctime(&_send_) << endl;

	  int send_len;

   	  //if (send_len = sendto(sockfd, (char*)_send_, sizeof(_send_), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) 
				//error("Error in sending expiry date to UDP Server\n\n");
		
	  if (send_len = sendto(sockfd, d, sizeof(d), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) 
			error("Error in sending expiry date to UDP Server\n\n");
//////////////////////////////////////////////////////////////////////////////
	}

	void browseThroughNews()
	{
		system("clear");

		struct record news_array[MAX_SIZE];
		int arr_counter=0;

		struct record obj;
		int opt=0;

		cout<<" Welcome Administrator ! \n\n Loading..."<<endl;

		sleep(2);

		system("clear");

		char mssg[10]="";

		int write_count=sendto(sockfd, mssg, 10, 0, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
			if(write_count<0) error("Error in writing to Administrative Client\n");
		
   		int read_count;
   		socklen_t serv_addr_len=sizeof(serv_addr);

   		if(read_count=recvfrom(sockfd, (char*)&obj, sizeof(obj), 0, (struct sockaddr *) &serv_addr, &serv_addr_len) == -1) error("Error in receiving from UDP Server\n\n");

   		while(read_count>0)
   		{
   			news_array[arr_counter++]=record(obj);
   			read_count=recvfrom(sockfd, (char*)&obj, sizeof(obj), 0, (struct sockaddr *) &serv_addr, &serv_addr_len);
   		}

   		cout<<"\nRetrieved "<<arr_counter<<" entries from the TCP Server \n\n";

   		for(int i=arr_counter-1;i>=0;i--)
   		{
   			cout<<"------------------------------------------------------------------\n";
   			cout<<" NEWS HEADLINE "<< (i+1) << " : " << news_array[i].headline << endl;
   			cout<<" TYPE : \n" << (news_array[i].type == 0 ? "ACADEMIC" : "NON-ACADEMIC" ) <<endl;
   			char* dt = ctime(&news_array[i].date_time);
///////////////////////////////////////////////////////////////////
   			//cout<<"\n DATED : " << dt <<endl;
			cout<<"\n DATED : " << news_array[i].date <<endl;

   		}
   		cout<<"------------------------------------------------------------------\n\n\n";

   		//close(sockfd);
	}
};


#endif
