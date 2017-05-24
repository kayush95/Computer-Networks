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
#include <fcntl.h>

#include <bits/stdc++.h>

#include <termios.h>
#include <stdio.h>
#include <ctype.h>

#include "../request.h"
#include "../train.h"

// server.h
#ifndef SERVER_H
#define SERVER_H

#define MAX_SIZE 1024
#define MAX_BACKLOG 10

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



struct record
{
  vector<Request> req;
  int file_descrip;

  //record(vector<Request> r,int fd) : req(r) , file_descrip(fd) {}

};
/*
bool pair_comparator(const record& struct1,const record& struct2)
{

      time_t t1,t2;

      t1=struct1.req.timestamp;
      t2=struct2.req.timestamp;

      return difftime(t1,t2)<0;   // !!! //
}

bool numberCompare(const Request& first, const Request& second)
{  
  	return ( first.num_of_berths > second.num_of_berths);
}
*/

class Server
{

private:
   vector<Request> client_vector; // maximum length of this vector = MAX_BACKLOG 
   // client_socket[i] file descriptor --- > corresponds to the <--- client_vector[i] request object

    int pid,sd;

  //  Book_Details response_2_client;//[MAX_SIZE];
   // Request buffer[MAX_SIZE];
    int buffer_list_counter,book_detail_list_counter;

    list<Train> train_list;
   // vector<record> req_list__vector; // a vector of many (lists of requests)

    fd_set readfds,writefds,errorfds;
    int listen_socket,new_socket,max_sockfd,client_socket[MAX_BACKLOG];
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr; // server address and client address respectively

void setnonblocking(int sock)
{
  int opts;

  opts = fcntl(sock,F_GETFL);
  if (opts < 0) {
    perror("fcntl(F_GETFL)");
    exit(EXIT_FAILURE);
  }
  opts = (opts | O_NONBLOCK);
  if (fcntl(sock,F_SETFL,opts) < 0) 
  {
    perror("fcntl(F_SETFL)");
    exit(EXIT_FAILURE);
  }
  return;
}

public:
  Server(int portno,list<Train> train_list)
  {
    buffer_list_counter=0;
    book_detail_list_counter=0;

    this->train_list=train_list;
    if((listen_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) <0) error("Error in opening TCP Server Socket !");

    /* So that we can re-bind to it without TIME_WAIT problems */
    int reuse_addr=1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    setnonblocking(listen_socket);

     bzero((char *) &serv_addr, sizeof(serv_addr));

     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     if ((bind(listen_socket,(struct sockaddr *)&serv_addr, sizeof(serv_addr)))<0) error("Error in binding the TCP Socket to the address !");
     
     clilen = sizeof(cli_addr);

     //initialise all client_socket[] to 0 so not checked
    for (int i = 0; i < MAX_BACKLOG; i++) client_socket[i] = -1;
    
     this->startListening();

  }

  ~Server()
  {
    //close(listen_socket);
  }
private:
    void error(const char* error_mssg)
    {
      perror(error_mssg);
      exit(1);
    }

void startListening()
{
    
    listen(listen_socket,MAX_BACKLOG);

    //cout<< "My IP : "<< this->getIP() << " !" << endl;

    while(1)
    {

      int status;
      if(waitpid(pid,&status,WNOHANG)==pid)
        if(WEXITSTATUS(status)==1)
          exit(0);

      cout<<"\n TCP Server running...\n\n";
      
      cout << "\nEnter 1 to show the Reservation Details of all the trains...\n";
      cout << "Enter Q to quit...\n";


      //clear the socket set
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&errorfds);
  
        //add master socket to set
        FD_SET(listen_socket, &readfds);   

        // add stdin to the read set
        FD_SET(0,&readfds);

        max_sockfd = listen_socket;
         
        //add child sockets to set
        for (int i = 0 ; i < MAX_BACKLOG ; i++) 
        {
            //socket descriptor
            int sd = client_socket[i];
             
            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            //highest file descriptor number, need it for the select function
            if(sd > max_sockfd)
                max_sockfd = sd;
        }
          
          struct timeval tm;
          tm.tv_sec = 10;
          tm.tv_usec = 0;
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        int activity = select( max_sockfd + 1 , &readfds , &writefds , &errorfds , &tm);
    
        if ((activity < 0) && (errno!=EINTR)) error("select error");
        else 
          cout << "activity : " << activity << endl;

        // if something happened on STDIN, process the input
        if(FD_ISSET(0,&readfds))
        {
          if((pid=fork())==0)
          {
          system("clear");
          fflush(0);
          waitForInput();
          exit(0);
          }
        }
          
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(listen_socket, &readfds)) 
        {
            if ((new_socket = accept(listen_socket, (struct sockaddr *)&cli_addr, (socklen_t*)&clilen))<0) error("Error on Accepting TCP Connection Request !");
            setnonblocking(new_socket);
            //inform user of socket number - used in send and receive commands
             char cli_ip[INET_ADDRSTRLEN];
            if(inet_ntop(AF_INET,&(cli_addr.sin_addr),cli_ip,INET_ADDRSTRLEN)==NULL) error("Error in determining client IP Address !");
            
            cout<<"@ port : " << ntohs(cli_addr.sin_port) << " TCP Connection made with the client " << cli_ip << " !\n\n";
          

           // char message[20]="Welcome !\n";
            //send new connection greeting message
           // if( send(new_socket, message, strlen(message), 0) != strlen(message) ) error("Error in send !");
              
          //  puts("Welcome message sent successfully");
              
            //add new socket to array of sockets
            for (int i = 0; i < MAX_BACKLOG; i++) 
            {
                //if position is empty
                if( client_socket[i] == -1 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding the socket %d to list of sockets at index %d\n" ,client_socket[i], i);
                     
                    break;
                }
            }
        }
          
        //else its some IO operation on some other socket :)
        for (int i = 0; i < MAX_BACKLOG; i++) 
        {
            sd = client_socket[i];

            if(sd<=0)
              continue;
              cout << "Reached 6!\n";
            if (FD_ISSET( sd , &readfds)) 
            {
              //vector<Request> inp_req_list;
            //  int valread;
                //Check if it was for closing , and also read the incoming message
              /*  if ((valread = read( sd ,(char*) &inp_req_list, sizeof(inp_req_list))) == 0)
                {
                  cout << "Reached 2!";
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&cli_addr , (socklen_t*)&clilen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(cli_addr.sin_addr) , ntohs(cli_addr.sin_port));
                      
                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = -1;
                }*/
                  
                //Echo back the message that came in
            //    else
             //   {
                     cout << "Reached 3!";

                  

                  Request obj;

                  int number_of_bytes = read(sd, (char*)&obj, sizeof(obj));

    if(number_of_bytes<=0) error("Error in receiving Request object from booking agent !\n\n");
    else
      cout<<" Received Request object from booking agent ! ";

   Book_Details response_2_client;//(obj.book_ID,obj.num_of_berths,obj.train_no);
   
   response_2_client.book_ID=obj.book_ID;
   response_2_client.num_of_berths=obj.num_of_berths;
   response_2_client.train_no=obj.train_no;

    cout<<response_2_client.book_ID<<endl;
    cout<<response_2_client.num_of_berths<<endl;
    cout<<response_2_client.train_no<<endl;

    for(int i=0;i<obj.num_of_berths;i++)
    {
      response_2_client.coaches[i]=string("A1");
      response_2_client.seat_no[i]=i;
      response_2_client.position[i]=Berth::UB;

      cout<<response_2_client.coaches[i]<<endl;
      cout<<response_2_client.seat_no[i]<<endl;
      cout<<response_2_client.position[i]<<endl;
    }




int valwrite;
          //handleClient(i);//,inp_req_list);
// sending index i  means handle the client_socket[i] file_descriptor and client_vector[i] request object
          
          if ((valwrite = write( sd,(char*) &response_2_client, sizeof(response_2_client))) <= 0)
                error("Error in sending response_2_client !\n");
          else{
            getpeername(sd , (struct sockaddr*)&cli_addr , (socklen_t*)&clilen);
////////////////////////////////////////////////////////////////////////////
            //printf("Host disconnected , ip %s , port %d \n" ,  , );
////////////////////////////////////////////////////////////////////////////      
            cout << "Sent the response to the Client @ IP : " << inet_ntoa(cli_addr.sin_addr) << " and port : " << ntohs(cli_addr.sin_port) << "\n\n";
          
          }
          
         // cout << "SD : " << sd;
         // handleClient(sd);

                 
                 // struct record obj;
                  //obj.req = inp_req_list;
                 // obj.file_descrip=sd;
                  //struct record obj(inp_req_list,sd);
                  cout << "Reached 5!";
                 // req_list__vector.push_back(obj);
                  cout << "Reached 4!";
                  //handleClient(sd);
                }
            }
       // }
/*
        //  sort the vector
        sort(req_list__vector.begin(),req_list__vector.end(),pair_comparator);

        for(vector<record>::iterator it=req_list__vector.begin() ; it!=req_list__vector.end() ; it++) // handle the clients in the sorted order
        {
          int valwrite;
          list<Book_Details> response_2_client = handleClient(it->file_descrip);
          if ((valwrite = write( it->file_descrip ,(char*) &response_2_client, sizeof(response_2_client))) <= 0)
                error("Error in sending response_2_client !\n");
          else{
            getpeername(it->file_descrip , (struct sockaddr*)&cli_addr , (socklen_t*)&clilen);
////////////////////////////////////////////////////////////////////////////
            //printf("Host disconnected , ip %s , port %d \n" ,  , );
////////////////////////////////////////////////////////////////////////////      
            cout << "Sent the response to the Client @ IP : " << inet_ntoa(cli_addr.sin_addr) << " and port : " << ntohs(cli_addr.sin_port) << "\n\n";
          }
        }
        */

        //cout << "\nSleeping for 5 seconds...\n\n";

  }
}

void printReservationDetails()
{
  cout << "\n\n TRAIN NAME      TRAIN NO.          AC Seats :  Total [ Booked / Available ]          Sleeper Seats : Total [ Booked / Available ] \n\n";
  for(list<Train>::iterator it=train_list.begin() ; it!= train_list.end() ; it++)
  {
    int train_no=it->getID();
    char* name = it->getName();

    int bookedAC = it->getNoofAvailableSeats(true,true,false) + it->getNoofAvailableSeats(false,true,false);
    int availAC = it->getNoofAvailableSeats(true,true,true) + it->getNoofAvailableSeats(false,true,true);
    int totalAC = bookedAC + availAC;

    int availSlpr = it->getNoofAvailableSeats(true,false,true) + it->getNoofAvailableSeats(false,false,true);
    int bookedSlpr = it->getNoofAvailableSeats(true,false,false) + it->getNoofAvailableSeats(false,false,false);
    int totalSlpr = bookedSlpr + availSlpr;

    cout << " " << name << "     " << train_no << "                      " << totalAC << "[ " << bookedAC << " / " << availAC << " ]                          " << totalSlpr<< " [ " << bookedSlpr<< " / " << availSlpr<<" ] \n\n";
  
  }
}


int policySelect(int indx1, int indx2)
{
  // analyze the selection by iterating the vector client_vector 
  // so as to resolve between the indices indx1 and indx2

  // and return the winner index 
}

/*
void handleClient(int index)
{
  // handle the client using client_socket[index] (as file descriptor) & client_vector[index] (as Request object)

  // and save the result / response  to the response_2_client (Book_Details object)
}
*/

void handleClient(int sd)//,vector<Request>& buffer)
{
  cout << "Reached 1!";
	 
	  int sizeBuffer,number_of_bytes=-1;

    number_of_bytes = recv(sd, (int*)&buffer_list_counter, sizeof(buffer_list_counter),0);

    if(number_of_bytes<=0) error("Error in receiving counter from booking agent !\n\n");
    else
      cout<<" Received counter from booking agent ! ";

    char* buffer;
	  number_of_bytes = recv(sd, (char*)buffer, sizeof(buffer),0);

    if(number_of_bytes<=0) error("Error in receiving from booking agent !\n\n");
    else
      cout<<" Received from booking agent ! ";


	  sizeBuffer = buffer_list_counter;
	  
	  int binary[sizeBuffer];
	  for(int i=0; i < sizeBuffer; i++)
	    binary[i] = 0;
////////////////////////////////////////////////////////////////////////////
	  //buffer.sort(numberCompare);
	  //buffer.sort();
	  sort(buffer.begin(), buffer.end(),numberCompare);

	  int maxBerths = buffer[0].num_of_berths;

  	  for(int i = maxBerths ; i > 0 ; i--)
  	  {
	     int count = 0;
	     for (int j = 0 ; j < sizeBuffer ; j++)
	     {
	        if(buffer[j].num_of_berths == maxBerths)
		   count++;
	     }

	    while (count--)
	    {
	        int max_age = 0, selected;
	        for(int k = 0; k < sizeBuffer; k++)
	        {
		   if(buffer[k].num_of_berths == i)
		   {
		       int age_one[buffer[k].num_of_berths];
		       for(int s=0; s<buffer[k].num_of_berths; s++)
		       {		            
				age_one[s] = buffer[k].pass_ages[s];
		       }        
		       int *one = max_element(age_one,age_one+buffer[k].num_of_berths);
		       if (*one > max_age && binary[k] == 0)
		            {max_age = *one;selected = k;}                     
		   }         
	        }

	        int book_ID = buffer[selected].book_ID;
	        int train_no = buffer[selected].train_no;
	        int coach_type = buffer[selected].coach_type;   // either AC or sleeper
	        int num_of_berths = i;
	        int* berth_preferences = buffer[selected].berth_preferences;
	        int* pass_ages = buffer[selected].pass_ages;
	      //time_t timestamp;

	        bool var2;

	        if(var2 == 6)  //AC or sleeper
		     var2 = true;
	        else 
		     var2 = false;
	      
	        int flag = 0;

	        for (list<Train>::iterator it1 = train_list.begin(); it1 != train_list.end(); it1++)
	        {
		    if(it1->ID == train_no)
		    {
		        if( (it1->getNoofAvailableSeats(true, var2) + it1->getNoofAvailableSeats(false, var2)) < num_of_berths )
		            cout << "Less number of seats available!\n";
		        else
		        {
		    binary[selected] = 1;
		    //Book_Details det = new Book_Details(book_ID, num_of_berths, train_no);
		    Book_Details det(book_ID, num_of_berths, train_no);
		    flag = 0;
		    int prefer[6];
		    memset(prefer,0,6);
		    for(int k=0; k<num_of_berths;k++)
		    {
		      prefer[berth_preferences[k]]++;
		    }
		    
		    for(vector<Coach*>::iterator it2 = (it1->coaches).begin(); it2 !=  (it1->coaches).end(); it2++)
		    {
		      if((*it2)->type == coach_type)
		      {               
		        if((*it2)->getSU()>=prefer[1] && (*it2)->getSL()>=prefer[2] && (*it2)->getUB()>=prefer[3] && (*it2)->getMB()>=prefer[4] && (*it2)->getLB()>=prefer[5])
		        {
		          flag = 1;
		          for(vector<Berth>::iterator it3 = ((*it2)->berths).begin(); it3 !=  ((*it2)->berths).end(); it3++)
		          {
		            for(int h = 1; h<=5; h++)
		            {   
		              if(prefer[h]--)
		              {
		                if(it3->alloc == false && it3->position == h)
		                {
		                  it3->alloc = true;
		                  (det.coaches).push_back((*it2)-> name);
		                  (det.position).push_back(it3-> position);
		                  (det.seat_no).push_back(it3-> seat_no);
		                }   
		              }
		            }                                                       
		          } 
		        }                                   
		      }
		    }
		    if(flag == 1)
		    {
		      details.push_back(det);
		    }
		    else
		    {
		      for(vector<Coach*>::iterator it2 = (it1->coaches).begin(); it2 !=  (it1->coaches).end(); it2++)
		      {
		        if((*it2)->type == coach_type)
		        {               
		          {
		            for(vector<Berth>::iterator it3 = ((*it2)->berths).begin(); it3 !=  ((*it2)->berths).end(); it3++)
		            {
		              for(int h = 1; h<=5; h++)
		              {   
		                if(prefer[h]--)
		                {
		                  if(it3->alloc == false && it3->position == h)
		                  {
		                    it3->alloc = true;
		                    (det.coaches).push_back((*it2)->name);
		                    (det.position).push_back(it3->position);
		                    (det.seat_no).push_back(it3->seat_no);
		                  }   
		                }
		              }                                                       
		            } 
		          }                                   
		        }
		      }
		      details.push_back(det); 
		    }
		  }                 
		}
	      } 
	    }//while loop
  } 
///////////////////////////////////////////////////////////////////////////////////////  
}

void waitForInput()
{
        char choice=getch();
          switch (choice) 
          {
            case '1': printReservationDetails();
                      break;
            case 'q': /* terminate the server */
            case 'Q':
                      cout << "Terminating the server !\n\n";
                      exit(1);
                      break;
            default: /* bad input */
                      cout << "ERROR: unknown command\n" ;
                      
          }
    
}  


public:
    char* getIP()
    {
      char ip[INET_ADDRSTRLEN];
      if(inet_ntop(AF_INET,&(serv_addr.sin_addr),ip,INET_ADDRSTRLEN)==NULL) error("Error in returning IP Address !");
      return strdup(ip);
    }

};

#endif
