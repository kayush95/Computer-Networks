#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <time.h>

#include "../rtlp.h"

// server.h
#ifndef RTLP_SERVER_H
#define RTLP_SERVER_H

#define MAX_SIZE 1024

// The packet length
#define PCKT_LEN 1000
#define ACK_DELAY 10  // seconds

using namespace std;


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

class RTLP_Server
{
private:
  char* sourceIP;
  int sourcePortNo;
  char* destIP;
  int destPortNo;
  int server_seq;
  int sd; // socket descriptor // No data/payload just datagram
  char buffer[PCKT_LEN]; // Our own headers' structures
  struct iphdr *ip;
  struct rtlpheader *rtlp;
  char* data;

  // Source and destination addresses: IP and port
  struct sockaddr_in sin, din;

  void error(const char* error_mssg)
  {
     perror(error_mssg);
     exit(1);
  }


public:
  RTLP_Server(char* _sourceIP,int _sourcePortNo, char* _destIP, int _destPortNo) : sourceIP(_sourceIP) , sourcePortNo(_sourcePortNo) , destIP(_destIP) , destPortNo(_destPortNo)
  {
    ip = (struct iphdr *) buffer; 
    rtlp = (struct rtlpheader *) (buffer+sizeof(struct iphdr));
    data = (char*) (buffer +sizeof(struct iphdr)+sizeof(struct rtlpheader));
    // OR
    // data = (char*) (rtlp+1);

    srand(time(NULL));
    server_seq = rand()%10000+1;
    cout << "Initial sequence number = " << server_seq << endl;
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
    cout << "RTLP Server started...\n\n";
  }

  ~RTLP_Server()
   {}

  void run()
  {
    establishConnection();

    cout << "Successful connection establishment !!!\n\n";
 
    cout << "Running Echo Test!\n";

    while(1)
    {
	runEchoTest();
	if(ntohs(rtlp->type) == FIN)
		break;
    }

    terminateConnection();

    cout << "Successful connection termination !!!\n\n";
  }


  void runEchoTest()
  {

    char response[20];
    strcpy(response, "ECHO RES ");
    int reply_ack;
    bool acknowledged=false;

    fd_set read_fd_set;

    int din_len = sizeof(din);

    while(!acknowledged)
    {
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
		       cout << " packet from client"<< endl;
	     }

       	      if(ntohs(rtlp->type) == DATA)
              {
                	unsigned int calculated_checksum = csum(buffer);
		        cout << "calculated_checksum = " << calculated_checksum << " and the received one : " << ntohl(rtlp->rtlph_chksum) << endl;

		        cout << "Received Sequence Number = "<< ntohl(rtlp->rtlph_seqnum) << endl;
			cout << "Received Acknowledgment Number = "<<ntohl(rtlp->rtlph_acknum) << endl;

		        if(calculated_checksum == ntohl(rtlp->rtlph_chksum))
		        {
				  cout << "The DATA from client has been Acknowledged !!!" << endl;
				  reply_ack = ntohl(rtlp->rtlph_seqnum);
				  
				  //sendpacket(SYN_ACK,NULL,5678,htonl(reply_ack)); // -1 dentoes NULL
				  acknowledged=true;				 
				  break;
		        }
		        else
		        {
				  error("Received packet's checksum doesn't agree ! Waiting for next resend...\n");
				  acknowledged = false;
				  continue;
		        }

              }
	      else if(ntohs(rtlp->type) == FIN)
	      {
		     cout << "FIN type data\n";
	       	     break;
	      }
              else
              {
		        error("Received packet is not of type DATA (expected) ! Waiting for next resend...\n");
		        acknowledged=false;
		        continue;
              }
      }

      if(ntohs(rtlp->type) == FIN)
		return;
	
      int num;

      cout << "Client Request : " << data << endl;

      sscanf(data,"ECHO REQ %d",&num); 

      cout << "num = " << num << endl; 
      
      /*while(data[i] != '\0')
      {
		strcat(num,(const char*)buffer[i]);
      }*/


      strcat(response, itoa(num+1).c_str());

      //sprintf(response, "ECHO RES %d", num+1 );

      cout << "Server Response : " << response << endl << endl;  

      /*strcat(response, itoa( atoi(num) + 1 ));*/

      setAddress();
      setIPHeader();

      //strcpy(data, response); 
      
      server_seq = server_seq + strlen(response);

      sendpacket(DATA,response,server_seq, (reply_ack));

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
    
    cout << "Sent Sequence Number = "<< seq_no << endl;
    cout << "Sent Acknowledgment Number = "<< ack_no << endl;


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
      cout << " packet to client"<< endl;

    
    
  }

  void establishConnection()
  {
  	int reply_ack;
    bool acknowledged=false;

    fd_set read_fd_set;

     int din_len = sizeof(din);

    while(!acknowledged)
    {
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
         cout << " packet from client"<< endl;
     }

       		if(ntohs(rtlp->type) == SYN)
              {

		       
                unsigned int calculated_checksum = csum(buffer);
                cout << "calculated_checksum = " << calculated_checksum << " and the received one : " << ntohl(rtlp->rtlph_chksum) << endl;

                cout << "Received Sequence Number = "<< ntohl(rtlp->rtlph_seqnum) << endl;
		cout << "Received Acknowledgment Number = "<<ntohl(rtlp->rtlph_acknum) << endl;

                if(calculated_checksum == ntohl(rtlp->rtlph_chksum))
                {
                  cout << "The SYN from client has been Acknowledged !!!" << endl;
                  reply_ack = ntohl(rtlp->rtlph_seqnum);
                  //sendpacket(SYN_ACK,NULL,5678,htonl(reply_ack)); // -1 dentoes NULL
                  acknowledged=true;
                 
                  break;
                }
                else
                {
                error("Received packet's checksum doesn't agree ! Waiting for next resend...\n");
                  acknowledged = false;
                  continue;
                }

              }
              else
              {
                error("Received packet is not of type SYN (expected) ! Waiting for next resend...\n");
                acknowledged=false;
                continue;
              }
          }


           acknowledged = false; // starting another repeat of events for receiving the FINAL ACK packet

		while(!acknowledged)
		{	
      setAddress();
      setIPHeader();

			sendpacket(SYN_ACK,NULL,server_seq,(reply_ack)); // -1 dentoes NULL
       cout << "Waiting for receiving final Acknowledgement ... \n";
			
			sleep(1);

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
		        cout << "No activity found ... The sent SYN-ACK packet not acknowledged ! Resending the packet...\n";
		        acknowledged=false;
		        continue;
		      }
		      else
		      {
		          int din_len = sizeof(din);

		          if(recvfrom(sd, (char*)buffer, sizeof(buffer), 0, (struct sockaddr *)&din, (socklen_t*) &din_len) <= 0)
		            error("Error in receiving final ACK packet !\n");
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
		              cout << " packet from client"<< endl;

		              if(ntohs(rtlp->type) == ACK)
		              {
		                unsigned int calculated_checksum = csum(buffer);
		                cout << "calculated_checksum = " << calculated_checksum << " and the received one : " << ntohl(rtlp->rtlph_chksum) << endl;

		                cout << "Received Sequence Number = "<< ntohl(rtlp->rtlph_seqnum) << endl;
				cout << "Received Acknowledgment Number = "<<ntohl(rtlp->rtlph_acknum) << endl;

		                if(calculated_checksum == ntohl(rtlp->rtlph_chksum))
		                {
		                  cout << "Acknowledged ACK !!!" << endl;
		                  acknowledged=true;
		                  break;
		                }
		                else
		                  acknowledged = false;

		              }
		              else
		              {
		                cout << "Received packet is not of type ACK (expected) !\n";
		                acknowledged=false;
		              }
		          }
		      }

		        if(!acknowledged)
		          cout << "Finally, The sent SYN packet not acknowledged ! Resending the packet...\n";
		      	else
		      		error("Impossible !");
      }
      

//      sendpacket(ACK,NULL,1234,rtlp->rtlph_seqnum);

  }

  void terminateConnection()
  {

    int reply_ack;
    bool acknowledged=false;

    fd_set read_fd_set;

     int din_len = sizeof(din);

    while(!acknowledged)
    {
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
         cout << " packet from client"<< endl;
     }

          if(ntohs(rtlp->type) == FIN)
              {
                unsigned int calculated_checksum = csum(buffer);
                cout << "calculated_checksum = " << calculated_checksum << " and the received one : " << ntohl(rtlp->rtlph_chksum) << endl;

                cout << "Received Sequence Number = "<< ntohl(rtlp->rtlph_seqnum) << endl;
		cout << "Received Acknowledgment Number = "<<ntohl(rtlp->rtlph_acknum) << endl;

                if(calculated_checksum == ntohl(rtlp->rtlph_chksum))
                {
                  cout << "The FIN from client has been Acknowledged !!!" << endl;
                  reply_ack = ntohl(rtlp->rtlph_seqnum);
                  //sendpacket(FIN_ACK,NULL,5678,htonl(reply_ack)); // -1 dentoes NULL
                  acknowledged=true;
                 
                  break;
                }
                else
                {
                error("Received packet's checksum doesn't agree ! Waiting for next resend...\n");
                  acknowledged = false;
                  continue;
                }

              }
              else
              {
                error("Received packet is not of type FIN (expected) ! Waiting for next resend...\n");
                acknowledged=false;
                continue;
              }
          }

    acknowledged = false; // starting another repeat of events for receiving the FINAL ACK packet

    while(!acknowledged)
    { 
      setAddress();
      setIPHeader();

      sendpacket(FIN_ACK,NULL,server_seq,(reply_ack)); // -1 dentoes NULL
       cout << "Waiting for receiving final Acknowledgement ... \n";
      
      sleep(1);

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
            cout << "No activity found ... The sent FIN-ACK packet not acknowledged ! Resending the packet...\n";
            acknowledged=false;
            continue;
          }
          else
          {
              int din_len = sizeof(din);

              if(recvfrom(sd, (char*)buffer, sizeof(buffer), 0, (struct sockaddr *)&din, (socklen_t*) &din_len) <= 0)
                error("Error in receiving final ACK packet !\n");
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
                  cout << " packet from client"<< endl;

                  if(ntohs(rtlp->type) == ACK)
                  {
                    unsigned int calculated_checksum = csum(buffer);
                    cout << "calculated_checksum = " << calculated_checksum << " and the received one : " << ntohl(rtlp->rtlph_chksum) << endl;

                    cout << "Received Sequence Number = "<< ntohl(rtlp->rtlph_seqnum) << endl;
		    cout << "Received Acknowledgment Number = "<<ntohl(rtlp->rtlph_acknum) << endl;

                    if(calculated_checksum == ntohl(rtlp->rtlph_chksum))
                    {
                      cout << "Acknowledged ACK !!!" << endl;
                      acknowledged=true;
                      break;
                    }
                    else
                      acknowledged = false;

                  }
                  else
                  {
                    cout << "Received packet is not of type ACK (expected) !\n";
                    acknowledged=false;
                  }
              }
          }

            if(!acknowledged)
              cout << "Finally, The sent FIN packet not acknowledged ! Resending the packet...\n";
            else
              error("Impossible !");
      }
      
  }

  
};

#endif
