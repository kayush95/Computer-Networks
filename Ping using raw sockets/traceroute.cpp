#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/sem.h>
#include <poll.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h> //hostent
#include <errno.h>
#include <sys/un.h>
#define SA (struct sockaddr*)
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <iostream>

using namespace std;

#define TIMEOUT  100000//usec

unsigned short csum (unsigned short *buf, int nwords)
{
  unsigned long sum;
  for (sum = 0; nwords > 0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

int get_IP_from_hostname(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}

void prepare_IP_header(struct ip* ip_header,char* source_IP,char* dest_IP,char* buf,int TTL)
{
      ip_header->ip_hl = 5;
      ip_header->ip_v = 4;
      ip_header->ip_tos = 0;
      ip_header->ip_len = 20 + 8;
      ip_header->ip_id = 10000;
      ip_header->ip_off = 0;
      ip_header->ip_ttl = TTL;
      ip_header->ip_p = IPPROTO_ICMP;
      inet_pton (AF_INET, source_IP, &(ip_header->ip_src));
      inet_pton (AF_INET, dest_IP, &(ip_header->ip_dst));
      ip_header->ip_sum = csum ((unsigned short *) buf, 9);
}

void prepare_ICMP_header(struct icmphdr* icmp_header, char* buf, int TTL)
{
     icmp_header = (struct icmphdr *) (buf + 20);
      icmp_header->type = ICMP_ECHO;
      icmp_header->code = 0;
      icmp_header->checksum = 0;
      icmp_header->un.echo.id = 0;
      icmp_header->un.echo.sequence = TTL + 1;
      icmp_header->checksum = csum ((unsigned short *) (buf + 20), 4);
}

int main (int argc, char *argv[])
{
  if (argc != 3)
  {
    cout << "Usage: <destination_IP_or_hostname> <source_IP>\n" ;
    exit (0);
  }
  char *hostname = argv[1];
  char ip[100];
  get_IP_from_hostname(hostname , ip);
  // cout << hostname <<  " resolved to " << ip << nendl;
  cout << "Traceroute to " << argv[1] << " (" << ip << ")" << "30 hops max, 28 bytes packets \n" << endl ;
  int socket_fd = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP);
  char buf[4096] = { 0 };
  struct ip *ip_header = (struct ip *) buf;
  int TTL_num_hops = 1;

  int one = 1;
  const int *val = &one;
  if (setsockopt (socket_fd, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
    cout << "Cannot set HDRINCL!\n" ;

  struct sockaddr_in destination_address;
  destination_address.sin_port = htons (7);
  destination_address.sin_family = AF_INET;
  inet_pton (AF_INET, ip, &(destination_address.sin_addr));


  while (TTL_num_hops<=30)
  {

     prepare_IP_header(ip_header,argv[2],ip,buf,TTL_num_hops);

      struct icmphdr* icmp_header;
      prepare_ICMP_header(icmp_header,buf,TTL_num_hops);

      // send_timeo (socket_fd, buf, 28, 0, SA & destination_address, sizeof destination_address);
      struct timeval send_time,receive_time,difference;
      int count=3;
      bool flag = false;
      
      while(count--)
      {
          gettimeofday(&send_time,NULL);

          sendto (socket_fd, buf, sizeof(struct ip) + sizeof(struct icmphdr), 0, SA & destination_address, sizeof destination_address);
          char buff[4096] = { 0 };
          struct sockaddr_in receive_from_address;
          socklen_t len = sizeof (struct sockaddr_in);
          // recvfrom (socket_fd, buff, 28, 0, SA & receive_from_address, &len);
          fd_set rd_set;
          FD_ZERO(&rd_set);
          FD_SET(socket_fd, &rd_set);
          struct timeval _time_out_;
          _time_out_.tv_sec = 0;
          _time_out_.tv_usec = TIMEOUT; 
          int sel = select(socket_fd + 1, &rd_set, NULL, NULL, &_time_out_);
          if (sel == -1)
          {
            perror("Server: Error in select");
            exit(1);
          }
          if(FD_ISSET(socket_fd, &rd_set))
          {
              recvfrom (socket_fd, buff, sizeof(buff), 0, SA & receive_from_address, &len);
              gettimeofday(&receive_time,NULL);
              timersub(&receive_time,&send_time,&difference);
              double elapsed_time_msec=(difference.tv_sec)*1000+((double)(difference.tv_usec))/1000;
              struct icmphdr *icmp_header_ptr = (struct icmphdr *) (buff + 20);

              if(count==2)
              {
                    char hostname[1000], servname[1000];
                    int x;
                    if ((x=getnameinfo((struct sockaddr*)&receive_from_address, sizeof receive_from_address, hostname, sizeof hostname, servname, sizeof servname, NI_NAMEREQD)) == 0) 
                        printf ("%2d  %s (%s) %lf ms  ", TTL_num_hops, hostname,inet_ntoa (receive_from_address.sin_addr),elapsed_time_msec);
                    else
                      printf ("%2d  %s (%s) %lf ms  ", TTL_num_hops, inet_ntoa (receive_from_address.sin_addr),inet_ntoa (receive_from_address.sin_addr),elapsed_time_msec);
               
                if (icmp_header_ptr->type != 0 && strcmp(inet_ntoa(receive_from_address.sin_addr),ip)!=0)
                  flag = false;
                else
                  flag = true;
                
              }
              else cout << elapsed_time_msec << " ms  ";
          }
          else
          {
              if(count==2)
                printf("%2d  * ",TTL_num_hops);
              else
                cout << " *  ";            
          }
      }
      cout << endl;
      if(flag)
      {
        exit(0);
      }
      TTL_num_hops++; //increment the TTL by 1, for the next iteration
  }

  return 0;
}