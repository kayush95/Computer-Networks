#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fstream>
#include <stdio.h>
#include <termios.h>
#include <ctype.h>
#include <sstream>
#include "../request.h"
#include "../train.h"

// booking_agent.h
#ifndef BOOKING_AGENT_H
#define BOOKING_AGENT_H

#define MAX_SIZE 1024

using namespace std;


static struct termios old;
static struct termios _new;


vector<string> split(string str, char delimiter) {
  vector<string> internal;
  stringstream ss(str); // Turn the string into a stream.
  string tok;
  
  while(getline(ss, tok, delimiter)) {
    internal.push_back(tok);
  }
  
  return internal;
}

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

string itoa(int a)
{
    string ss="";   //create empty string
    while(a)
    {
        int x=a%10;
        a/=10;
        char i='0';
        i=i+x;
        ss=i+ss;      //append new character at the front of the string!
    }
    return ss;
}


class Booking_Agent
{
private:
  Request req_list[20];
  Book_Details book_detail_list[20];
  int req_list_counter,book_detail_list_counter;

  int sockfd,portno;
  char serverIP[20];
  struct sockaddr_in serv_addr;

  void error(const char* error_mssg)
    {
      perror(error_mssg);
      exit(1);
    }
public:
  Booking_Agent(char serverIP[20],int portno)
  {
    req_list_counter=0;
    book_detail_list_counter=0;

    strcpy(this->serverIP,serverIP);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd<0)
      error("Error in opening Booking_Agent Socket");

    this->portno=portno;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    /* either this
    struct hostent* server;
    server = gethostbyname(serverIP); // if serverIP is a DNS name // need to insert a new mapping in /etc/hosts file
       if (server == NULL) {
          fprintf(stderr,"ERROR, no such host\n");
          exit(0);
      }
      bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
      */


      // OR this 
  //  if serverIP is an IP address in dot notation
    inet_pton(AF_INET, serverIP, &(serv_addr.sin_addr));

  }
private:
  void parseInputCSV(const char* filename)
  {
    //cout << 323;

    // parse the csv file and build the object

    // Read the file ----------
    /*
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) error("Can't open CSV file ! ");
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    //char *pData = new char[size + 1];
    char* pData = (char*)malloc((size+1)*sizeof(char));
    fread(pData, sizeof(char), size, fp);
    fclose(fp);
    */
    string line[100];
    int i=0;

   ifstream myfile( filename);
  if (myfile)  // same as: if (myfile.good())
    {
    while (getline( myfile, line[i] ))  // same as: while (getline( myfile, line ).good())
      {
        i++;
      }
    myfile.close();
    }
    
    int lineCount=i;

    // line[lineCount]=NULL
    for(i=0;i<lineCount;i++)
    {
     // cout << "line[i]" << line[i] << endl;

      vector<string> tokens = split(line[i],',');

      int _id_=stoi(tokens[0],NULL,10);
      int _train_no_=stoi(tokens[1],NULL,10);
      int _coach_type_;
      if(tokens[2]=="AC")
        _coach_type_=Coach::AC;
      else if(tokens[2]=="Sleeper")
        _coach_type_=Coach::Sleeper;

      int _num_of_berths_=stoi(tokens[3],NULL);

      int prefer[_num_of_berths_+1];
      if(tokens[4]=="NA")
        prefer[0]=-1;
      else
      {
        vector<string> tok = split(tokens[4],'-');

        //prefer = (int*)malloc(_num_of_berths_*sizeof(int));

        int n = _num_of_berths_-1;
        prefer[_num_of_berths_] = -1;
        while(n >= 0)
        {
          if(tok[n] == "SU")
            prefer[n]=Berth::SU;
          else if(tok[n] == "SL")
            prefer[n]=Berth::SL;
          else if(tok[n] == "UB")
            prefer[n]=Berth::UB;
          else if(tok[n] == "MB")
            prefer[n]=Berth::MB;
          else if(tok[n] == "LB")
            prefer[n]=Berth::LB;
          else error("Erroneous parsing here !");
          n--;
        }
      }

        int ages[_num_of_berths_];
      
        vector<string> tok1 = split(tokens[5],'-');

        //ages = (int*)malloc(_num_of_berths_*sizeof(int));
        int n = _num_of_berths_- 1;
        
        while(n >= 0)
        {
          
          ages[n]=stoi(tok1[n],NULL);
          n--;
        }

      /*
      // do for every line : 
      char* str1=strtok(line[i],",");
      char* str2=strtok(NULL,",");
      char* str3=strtok(NULL,",");
      char* str4=strtok(NULL,",");
      char* str5=strtok(NULL,",");
      char* str6=strtok(NULL,",");

      //cout << "\n\n " << str1 << "\n\n " << str2 << "\n\n " << str3 << "\n\n " << str4 << "\n\n " << str5 << "\n\n " << str6 << endl;

      int _id_;
      sscanf(str1,"%d",&_id_);

      int _train_no_;
      sscanf(str2,"%d",&_train_no_);

      
      int _coach_type_;
      if(strcmp(str3,"AC")==0)
        _coach_type_=Coach::AC;
      else if(strcmp(str3,"Sleeper")==0)
        _coach_type_=Coach::Sleeper;
      else
        error("Erroneous parsing Coach type !");


      
      int _num_of_berths_;
      sscanf(str4,"%d",&_num_of_berths_);

      //cout << "_num_of_berths_" << _num_of_berths_ << endl;

      
      int* prefer;
      if(strcmp(str5,"NA")==0)
        prefer=NULL;
      else
      {
        int counter=0;
        //prefer=new int[_num_of_berths_];
        prefer = (int*)malloc(_num_of_berths_*sizeof(int));
        char* prf = strtok(str5,"-");

        while(prf!=NULL)
        {

        if(strcmp(prf,"SU")==0)
          prefer[counter]=Berth::SU;
        else if(strcmp(prf,"SL")==0)
          prefer[counter]=Berth::SL;
        else if(strcmp(prf,"UB")==0)
          prefer[counter]=Berth::UB;
        else if(strcmp(prf,"MB")==0)
          prefer[counter]=Berth::MB;
        else if(strcmp(prf,"LB")==0)
          prefer[counter]=Berth::LB;
        else error("Erroneous parsing here !");

        //cout<<"counter : " << counter << " prefer_str : " <<  prf << " age : " << prefer[counter] << endl;
          counter++;        
          prf = strtok(NULL,"-");

        }
       //cout << counter << " ---- " << _num_of_berths_ << endl;
        if(counter != _num_of_berths_)
          error("Berth Preference counter mismatch !");
      }

      
    //cout << " str6 :" << str6  << endl;

        int counter=0;
        //int* ages=new int[_num_of_berths_];
        int* ages = (int*)malloc(_num_of_berths_*sizeof(int));
        char* age_str = strtok(str6,"-");

        while(age_str!=NULL)
        {
          sscanf(age_str,"%d",&ages[counter]);
        //cout<<"counter : " << counter << " age_str : " << age_str << " age : " << ages[counter] << endl;
          counter++;        
          age_str = strtok(NULL,"-");
        }
       //cout << counter << " ---- " << _num_of_berths_ << endl;
      
        if(counter != _num_of_berths_)
          error("Age counter mismatch !");

        // just now 

        */
     // Request obj= Request();//(_id_,_coach_type_,_train_no_,_num_of_berths_,_timestamp_,prefer,ages);
      int indx=req_list_counter++;
      time_t _timestamp_ = time(0); 
      req_list[indx].book_ID=_id_;
      req_list[indx].train_no=_train_no_;
      req_list[indx].num_of_berths=_num_of_berths_;
      req_list[indx].timestamp = time(0);

    //  cout << req_list[indx].book_ID<<endl;

      for(int i=0;i<=_num_of_berths_;i++)
      {
        if(prefer[0]!=-1)
          req_list[indx].berth_preferences[i]=prefer[i];
        req_list[indx].pass_ages[i]=ages[i];
      }

    if(prefer[0]==-1)
      req_list[indx].berth_preferences[0]=-1;
     // cout << "obj size : " << sizeof(obj) << endl;

      //cout << "req_list b4 pushing ... " << req_list_counter<< endl;
     // req_list[req_list_counter++] = obj;
      //cout << "req_list after pushing ... " << req_list_counter << endl;

      //if(prefer!=NULL) free(prefer);//delete[] prefer;

      //free(ages);//delete[] ages;
    }

  // Parsed the file content ----------  
    //free(pData);//delete[] pData;
    return;
  }

  void dealWithServer()
  {
   // cout<<420;
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error in connecting to TCP Server !");
    else cout<<" Connected to the TCP Server "<< serverIP<<endl;
   // system("clear");
  //  cout << req_list_counter << endl;
    for(int i=0; i<req_list_counter ; i++)
    {
        int write_count=write(sockfd,(char*)&req_list[i],sizeof(req_list[i]));

        if(write_count<=0) error("Error in sending object to TCP Server !\n\n");
        else
          cout << "sent the request object to the Booking server \n\n";

         int read_count=read(sockfd,(char*)&book_detail_list[i],sizeof(book_detail_list[i]));

        if(read_count<=0) error("Error in receiving response from TCP Server !\n");
        else cout << "\n\nreceived Server Response \n"<< endl;

       

    cout<<book_detail_list[0].book_ID<<endl;
    cout<<book_detail_list[0].num_of_berths<<endl;
    cout<<book_detail_list[0].train_no<<endl;
    cout<<book_detail_list[0].num_of_berths<<endl;
    for(int i=0;i<book_detail_list[0].num_of_berths;i++)
    {
      cout << "yo\n";
      cout<<book_detail_list[0].coaches[i]<<endl;
      cout << "yo\n";
      cout<<book_detail_list[0].seat_no[i]<<endl;
      cout << "yo\n";
      cout<<book_detail_list[0].position[i]<<endl;
      cout << "yo\n";
    }


        usleep(500);
    }

    close(sockfd);
  }

   string pos_repr(int* pos_arr,int n)
  {
    string str="";
    int i=0;

    switch(pos_arr[i])
    {
      case Berth::UB:
      str=str+"UB";
      break;
      case Berth::LB:
      str=str+"LB";
      break;
      case Berth::MB:
      str=str+"MB";
      break;
      case Berth::SL:
      str=str+"SL";
      break;
      case Berth::SU:
      str=str+"SU";
      break;
    }

    for(i=1;i<n;i++)
    {
      switch(pos_arr[i])
    {
      case Berth::UB:
      str=str+"-UB";
      break;
      case Berth::LB:
      str=str+"-LB";
      break;
      case Berth::MB:
      str=str+"-MB";
      break;
      case Berth::SL:
      str=str+"-SL";
      break;
      case Berth::SU:
      str=str+"-SU";
      break;
    }

    }
    return str;
  }

  string coach_repr(string coach_arr[],int n)
  {
    string str=string(coach_arr[0]);

    for(int i=0;i<n;i++){
      str=str+"-";
      str=str+string(coach_arr[i]);
    }

    return str;

  }

   string seat_repr(int* seat_arr,int n)
  {
    string str=itoa(seat_arr[0]);

    for(int i=0;i<n;i++){
      str=str+"-";
      str=str+itoa(seat_arr[i]);
    }

    return str;

  }


  void writeOutputCSV(const char* filename)
  {
    // Open the file ----------

    ofstream fp;
    cout << 0;
    fp.open (filename, ios::out | ios::app );
    cout << 1;

    if (fp.fail() || fp.bad()) error("Can't create CSV file for writing output ! ");
  cout << 2;    

  
    for(int i=0; i < req_list_counter ; i++)
    {
      cout << 3;
      
    //  fp << "----------------------------------------------------\n";
    //  cout << book_detail_list[i].book_ID << "," << book_detail_list[i].train_no << "/" << seat_repr(book_detail_list[i].seat_no,book_detail_list[i].num_of_berths) << "/" << pos_repr(book_detail_list[i].position,book_detail_list[i].num_of_berths) << "/" << coach_repr(book_detail_list[i].coaches,book_detail_list[i].num_of_berths) << "\n";

    //  fp << "----------------------------------------------------\n\n\n";
      cout << 4;
      //for(;;)
        //cout <<3;
    }
    cout << 5;

    fp.close();

  }

 


public:
  void run()
  {
    parseInputCSV("booking.csv");
    cout<<"done parsing"<<endl;
    // now, the list req_list contains all the records parsed from the CSV file
    dealWithServer(); // send the req_list
    cout<<"done dealing"<<endl;
    writeOutputCSV("tickets.csv");
    cout<<"done writing"<<endl;
  }
  
};

#endif
