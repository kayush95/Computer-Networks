
#include <sys/wait.h>
#include <string.h>

#include "../SMTP_server.h"
#include "../POP_server.h"

using namespace std;

#define FIRST_SERVER_PORT__SMTP  6000
#define FIRST_SERVER_PORT__POP   7000

#define SECOND_SERVER_PORT__SMTP  8000
#define SECOND_SERVER_PORT__POP   9000


// Call as : ./main server_domain_name
int main(int argc,char* argv[])
{
     char server_domain_name[20];
     char redirection_IP[20];

	if (argc != 3) 
	{
         cerr << "ERROR, please provide arguments : ./main <server_domain_name> <redirection_IP> \n";
         exit(1);
     }
     else
     {
     	strcpy(server_domain_name,argv[1]);
          strcpy(redirection_IP,argv[2]);
     }

     int pid1=1,pid2=1;

     pid1=fork();

     if(pid1!=0)
     pid2=fork();

     if(pid1<0 || pid2<0)
     {
     	perror("Fork Error..Exiting !\n");
     	exit(1);
     }
     else if(pid1==0)
     {
          if(strcmp(server_domain_name,"abc.com")==0)
     	    SMTP_Server smtp_server_instance = SMTP_Server(FIRST_SERVER_PORT__SMTP,server_domain_name,redirection_IP);
          else if(strcmp(server_domain_name,"xyz.com")==0)
              SMTP_Server smtp_server_instance = SMTP_Server(SECOND_SERVER_PORT__SMTP,server_domain_name,redirection_IP);
          else 
               cerr << "Please choose between abc.com and xyz.com";
    

     }
     else if(pid2==0)
     {
          if(strcmp(server_domain_name,"abc.com")==0)
     	    POP_Server pop_server_instance = POP_Server(FIRST_SERVER_PORT__POP,server_domain_name);
           else if(strcmp(server_domain_name,"xyz.com")==0)
              POP_Server pop_server_instance = POP_Server(SECOND_SERVER_PORT__POP,server_domain_name);
          else 
               cerr << "Please choose between abc.com and xyz.com";
    
  }

     else
     {
     	wait(NULL);
     	wait(NULL);
     }


	return 0;
}