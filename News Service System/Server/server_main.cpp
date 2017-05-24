#include <bits/stdc++.h>
#include <sys/wait.h>
#include "server.h"

using namespace std;

// Call as : ./main <TCPport> <UDPport>
int main(int argc,char* argv[])
{
	int tcpport,udpport;

	if (argc != 3) 
	{
         cerr << "ERROR, no ports provided\n";
         exit(1);
     }
     else
     {
     	tcpport=atoi(argv[1]);
     	udpport=atoi(argv[2]);
     }

     int pid1=1,pid2=1,pid3=1;

     pid1=fork();

     if(pid1!=0)
     pid2=fork();

 	if(pid1!=0 && pid2!=0)
     pid3=fork();


     if(pid1<0 || pid2<0 || pid3<0)
     {
     	perror("Fork Error..Exiting !\n");
     	exit(1);
     }
     else if(pid1==0)
     {
     	TCPServer _TCP_Server_Rep = TCPServer(tcpport,TCPServer::REPORTER_CLIENT);	// for Reporter Client
		//Reporter _Rep[2];	// Instantiating 2 Client Reporters

		//_Rep[0]=Reporter();
		//_Rep[1]=Reporter();
		 //pass _TCP_Server_Rep.getIP() and tcpport as arguments
     }
     else if(pid2==0)
     {
     	TCPServer _TCP_Server_Readr = TCPServer(tcpport+1,TCPServer::READER_CLIENT);	// for Reader Client

     	//Reader _Read[3];	// Instantiating 3 Client Readers

		// _Read[0]=Reader();
		// _Read[1]=Reader();
		// _Read[2]=Reader();
		 //pass _TCP_Server_Readr.getIP() and tcpport as arguments

		 

     }
     else if (pid3==0)
     {
     	UDPServer _UDP_Server = UDPServer(udpport);	// for Administrative client


		//Administrator _Admin=Administrator() ;	// Instantiating Administrator
		// pass _UDP_Server.getIP() & udpport as arguments

     }
     else
     {
     	wait(NULL);
     	wait(NULL);
     	wait(NULL);
     }


	return 0;
}
