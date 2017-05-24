#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <termios.h>
#include <ctype.h>
#include <time.h>

#include "../rtlp.h"

// client.h
#ifndef RTLP_CLIENT_H
#define RTLP_CLIENT_H

#define MAX_SIZE 1024

// The packet length
#define PCKT_LEN 1000
#define ACK_DELAY 10  // seconds


static struct termios old;
static struct termios _new;

inline bool fileExists (const string& name) {
    ifstream f(name.c_str());
    if (f.good()) {
        f.close();
        return true;
    } else {
        f.close();
        return false;
    }   
}


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

using namespace std;


class RTLP_Client
{
private:
  char* sourceIP;
  int sourcePortNo;
  char* destIP;
  int destPortNo;

  int client_seq;

  int sd; // socket descriptor // No data/payload just datagram
  char buffer[PCKT_LEN]; // Our own headers' structures
  struct iphdr *ip;
  struct rtlpheader *rtlp;
  char* data;

  int start_ack_2_server;

  // Source and destination addresses: IP and port
  struct sockaddr_in sin, din;

  void error(const char* error_mssg)
  {
     perror(error_mssg);
     exit(1);
  }


public:
  RTLP_Client(char* _sourceIP,int _sourcePortNo, char* _destIP, int _destPortNo) : sourceIP(_sourceIP) , sourcePortNo(_sourcePortNo) , destIP(_destIP) , destPortNo(_destPortNo)
  {
    ip = (struct iphdr *) buffer; 
    rtlp = (struct rtlpheader *) (buffer+sizeof(struct iphdr));
    data = (char*) (buffer +sizeof(struct iphdr)+sizeof(struct rtlpheader));
    // OR
    // data = (char*) (rtlp+1);

    srand(time(NULL));

    client_seq = 1+ (rand() % 10000);

    int one = 1;
    const int *val = &one;
    memset(buffer, 0, PCKT_LEN);

    sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    if(sd < 0) error("Encountered error in creation of socket !\n");

    memset(din.sin_zero, 0, sizeof(din.sin_zero));
    memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

    // The source is redundant, may be used later if needed
    // Address family
    sin.sin_family = AF_INET;
    din.sin_family = AF_INET;
    // Source port, can be any, modify as needed
    sin.sin_port = htons(sourcePortNo); // though not needed as we are creating our own TCP header
    din.sin_port = htons(destPortNo); // though not needed as we are creating our own TCP header
    // Source IP, can be any, modify as neede d
    sin.sin_addr.s_addr = inet_addr(sourceIP);
    din.sin_addr.s_addr = inet_addr(destIP);

    // Inform the kernel do not fill up the headers'
    // structure, we fabricated our own
    if(setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) error("Error in setting socket options !\n");

    printf("Using Source IP: %s @ port: %u,      Target IP: %s @ port: %u.\n", sourceIP, sourcePortNo, destIP, destPortNo);
    // sendto() loop, send every 2 second for 50 counts

    // The source and destination ports are always fixed
  }

  ~RTLP_Client()
  {}

  void run()
  {
    establishConnection();

    cout << "Successful connection establishment !!!\n\n";

    runEchoTest();

    terminateConnection();

    cout << "Successful connection termination !!!\n\n";
  }


  void runEchoTest()
  {
  	cout << "Starting ECHO test @ client.. \n\n";

    char ch = 'y';
    int n = 1;
    int running_seq=client_seq, reply_ack_2_server=start_ack_2_server;

    fd_set read_fd_set;

    do
    {
      FD_ZERO(&read_fd_set);

      FD_SET(sd,&read_fd_set);

      char temp[20];
      sprintf(temp,"ECHO REQ %d",n);

      setAddress();
      setIPHeader();

      sendpacket(DATA,temp,running_seq+=strlen(temp),reply_ack_2_server);

      struct timeval tm;
      tm.tv_sec = ACK_DELAY;
      tm.tv_usec = 0;

      int activity = select( sd+1 , &read_fd_set , NULL, NULL , &tm);
      
      if ((activity < 0) && (errno!=EINTR)) error("Error in selection of activity socket !\n");

      if(activity==0)
      {
        cout << "No activity found ... The sent DATA packet not acknowledged ! Resending the packet...\n";
        cout << "Resending the packet ... \n";
      //  acknowledged=false;
        continue;
      }

      int din_len = sizeof(din);

      if(recvfrom(sd, (char*)&buffer, sizeof(buffer), 0, (struct sockaddr *)&din, (socklen_t*) &din_len) <= 0)
        error("Error in receiving packet from server !\n");
      else
      {

        cout << "Received a " ;
        
            switch(ntohs(rtlp->type))
            {
              case SYN: cout << "SYN"; break;
              case SYN_ACK: cout << "SYN-ACK"; break; // <<<------- This is what is expected to be received
              case ACK: cout << "ACK"; break;
              case FIN: cout << "FIN"; break;
              case FIN_ACK: cout << "FIN-ACK"; break;
              case DATA: cout << "DATA"; break;
              default : error("Impossible error");
            }
              cout << " packet from server"<< endl;

        cout << "Checking Integrity using checksum... \n\n";

        unsigned int computed_checksum = csum(buffer);

        cout << "Checking computed chksum : " << computed_checksum << " against received value : " << ntohl(rtlp->rtlph_chksum) << "\n\n" ;

        if( ntohs(rtlp->type) == DATA && computed_checksum == ntohl(rtlp->rtlph_chksum) ) 
        {
            cout << "Valid packet received : " << data << "\n\n";
            cout << "\nINFO : This received packet contains Sequence No. : " << ntohl(rtlp->rtlph_seqnum) << " and Acknowledgement No. : " << ntohl(rtlp->rtlph_acknum) << "\n\n";

           reply_ack_2_server = ntohl(rtlp->rtlph_seqnum);
           n++;
         }
         else cout << "Invalid packet received. Ignoring..\nThe previous packet will be resent in the next iteration..\n";
         
      }

      cout << "Do you wanna continue ? Press (Y/N) : ";
      ch = getch();

    }while(ch == 'Y' || ch=='y');

    sendpacket(FIN,NULL,running_seq,reply_ack_2_server);
}
  

/*
  void sendMessage(int type,char mssg[MAX_SIZE])
  {
  	establishConnection();
  	//
  		 send the message
  	//

  	terminateConnection();
  }
  */

private:
  void setIPHeader()
  {
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(struct rtlpheader) + sizeof(struct iphdr) + sizeof(data));    /* 16 byte value */
    ip->frag_off = 0;       /* no fragment */
    ip->ttl = 64;           /* default value */
    ip->protocol = IPPROTO_RAW; /* protocol at L4 */
    ip->check = 0;          /* not needed in iphdr */
    ip->saddr = sin.sin_addr.s_addr;
    ip->daddr = din.sin_addr.s_addr; 
  }

  void setAddress()
  {

    memset(din.sin_zero, 0, sizeof(din.sin_zero));
    memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

    inet_pton(AF_INET, sourceIP, (struct in_addr *)&sin.sin_addr.s_addr);
    inet_pton(AF_INET, destIP, (struct in_addr *)&din.sin_addr.s_addr);

    // The source is redundant, may be used later if needed
    // Address family
    sin.sin_family = AF_INET;
    din.sin_family = AF_INET;
    // Source port, can be any, modify as needed
    sin.sin_port = htons(sourcePortNo); // though not needed as we are creating our own TCP header
    din.sin_port = htons(destPortNo); // though not needed as we are creating our own TCP header
    // Source IP, can be any, modify as neede d
    sin.sin_addr.s_addr = inet_addr(sourceIP);
    din.sin_addr.s_addr = inet_addr(destIP);
  }

  void sendpacket(const int type,char* data_ptr,int seq_no,int ack_no)
  {
    setAddress();
    setIPHeader();
   
     // rtlp structure
    // The source port, spoofed, we accept through the command line
    rtlp->rtlph_srcport = htons(sourcePortNo);
    // The destination port, we accept through command line
    rtlp->rtlph_destport = htons(destPortNo);


    if(data_ptr!=NULL) strcpy(data,data_ptr);
    rtlp->type    = htons(type);
    rtlp->rtlph_seqnum = htonl(seq_no);
    rtlp->rtlph_acknum = htonl(ack_no);

    rtlp->rtlph_chksum = htonl(csum(buffer));//, ( sizeof(struct rtlpheader) - sizeof(checksum) ) / (sizeof(unsigned short) ) ); 
    
    cout << "\nINFO : This packet is being sent with Sequence No. : " << seq_no << " and Acknowledgement No. : " << ack_no << "\n\n";

    int din_len = sizeof(din);
    if(sendto(sd, (char*)buffer, sizeof(buffer), 0, (struct sockaddr *)&din, din_len) <= 0)
        error("Error in sending packet !\n");
    else
      cout << "Sent a " ;
    switch(type)
    {
      case SYN: cout << "SYN"; break;
      case SYN_ACK: cout << "SYN-ACK"; break;
      case ACK: cout << "ACK"; break;
      case FIN: cout << "FIN"; break;
      case FIN_ACK: cout << "FIN-ACK"; break;
      case DATA: cout << "DATA"; break;
      default : error("Impossible error");
    }
      cout << " packet to server"<< endl;
    
  }

  void establishConnection()
  {
    bool acknowledged=false;

    fd_set read_fd_set;

    while(!acknowledged)
    {
      sendpacket(SYN,NULL,client_seq,1); // 1 dentoes NULL

      FD_ZERO(&read_fd_set);
      FD_SET(sd,&read_fd_set);

      struct timeval tm;
      tm.tv_sec = ACK_DELAY;
      tm.tv_usec = 0;

      int din_len = sizeof(din);

      int activity = select( sd+1 , &read_fd_set , NULL, NULL , &tm);
      
      if ((activity < 0) && (errno!=EINTR)) error("Error in selection of activity socket !\n");

      if(activity==0)
      {
        cout << "No activity found ... The sent SYN packet not acknowledged ! Resending the packet...\n";
        acknowledged=false;
        continue;
      }
      else
      {
          int din_len = sizeof(din);
          if(recvfrom(sd, (char*)&buffer, sizeof(buffer), 0, (struct sockaddr *)&din, (socklen_t*) &din_len) <= 0)
            error("Error in receiving packet !\n");
          else
          {
            cout << "Received a " ;
        
            switch(ntohs(rtlp->type))
            {
              case SYN: cout << "SYN"; break;
              case SYN_ACK: cout << "SYN-ACK"; break; // <<<------- This is what is expected to be received
              case ACK: cout << "ACK"; break;
              case FIN: cout << "FIN"; break;
              case FIN_ACK: cout << "FIN-ACK"; break;
              case DATA: cout << "DATA"; break;
              default : error("Impossible error");
            }
              cout << " packet from server"<< endl;

              if(ntohs(rtlp->type) == SYN_ACK)
              {
                unsigned int calculated_checksum = csum(buffer);
                cout << "calculated_checksum = " << calculated_checksum << " and the received one : " << ntohl(rtlp->rtlph_chksum) << endl;
                
                if(calculated_checksum == ntohl(rtlp->rtlph_chksum))
                {
                  cout << "\nINFO : This received packet contains Sequence No. : " << ntohl(rtlp->rtlph_seqnum) << " and Acknowledgement No. : " << ntohl(rtlp->rtlph_acknum) << "\n\n";

                  cout << "Acknowledged !!!" << endl;
                  acknowledged=true;
                }
                else
                  acknowledged = false;

              }
              else
              {
                cout << "Received packet is not of type SYN-ACK (expected) !\n";
                acknowledged=false;
              }
          }
      }

        if(acknowledged)
          break;
        else
          cout << "Finally, The sent SYN packet not acknowledged ! Resending the packet...\n";
      }

      start_ack_2_server = ntohl(rtlp->rtlph_seqnum);
      sendpacket(ACK,NULL,client_seq,ntohl(rtlp->rtlph_seqnum));

  }

  void terminateConnection()
  {

      bool acknowledged=false;

    fd_set read_fd_set;

    while(!acknowledged)
    {
      sendpacket(FIN,NULL,4321,1); // 1 dentoes NULL

      FD_ZERO(&read_fd_set);
      FD_SET(sd,&read_fd_set);

      struct timeval tm;
      tm.tv_sec = ACK_DELAY;
      tm.tv_usec = 0;

      int din_len = sizeof(din);

      int activity = select( sd+1 , &read_fd_set , NULL, NULL , &tm);
      
      if ((activity < 0) && (errno!=EINTR)) error("Error in selection of activity socket !\n");

      if(activity==0)
      {
        cout << "No activity found ... The sent FIN packet not acknowledged ! Resending the packet...\n";
        acknowledged=false;
        continue;
      }
      else
      {
          int din_len = sizeof(din);
          if(recvfrom(sd, (char*)&buffer, sizeof(buffer), 0, (struct sockaddr *)&din, (socklen_t*) &din_len) <= 0)
            error("Error in receiving packet !\n");
          else
          {
            cout << "Received a " ;
        
            switch(ntohs(rtlp->type))
            {
              case SYN: cout << "SYN"; break;
              case SYN_ACK: cout << "SYN-ACK"; break; // <<<------- This is what is expected to be received
              case ACK: cout << "ACK"; break;
              case FIN: cout << "FIN"; break;
              case FIN_ACK: cout << "FIN-ACK"; break;
              case DATA: cout << "DATA"; break;
              default : error("Impossible error");
            }
              cout << " packet from server"<< endl;

              if(ntohs(rtlp->type) == FIN_ACK)
              {
                unsigned int calculated_checksum = csum(buffer);
                cout << "calculated_checksum = " << calculated_checksum << " and the received one : " << ntohl(rtlp->rtlph_chksum) << endl;
                
                if(calculated_checksum == ntohl(rtlp->rtlph_chksum))
                {
                  cout << "\nINFO : This received packet contains Sequence No. : " << ntohl(rtlp->rtlph_seqnum) << " and Acknowledgement No. : " << ntohl(rtlp->rtlph_acknum) << "\n\n";

                  cout << "Acknowledged !!!" << endl;
                  acknowledged=true;
                }
                else
                  acknowledged = false;

              }
              else
              {
                cout << "Received packet is not of type FIN-ACK (expected) !\n";
                acknowledged=false;
              }
          }
      }

        if(acknowledged)
          break;
        else
          cout << "Finally, The sent FIN packet not acknowledged ! Resending the packet...\n";
      }

      sendpacket(ACK,NULL,4321,rtlp->rtlph_seqnum);


  }

  
};

#endif
