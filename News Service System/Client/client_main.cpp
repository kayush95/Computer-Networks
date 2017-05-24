#include <bits/stdc++.h>
#include <sys/wait.h>

#include "client.h"

using namespace std;

#define MAX_SIZE 1024

// Call as : ./main <IP_Addr> <TCPport> <UDPport>
int main(int argc,char* argv[])
{
	int tcpport,udpport;
     char serverIP[1024];

	if (argc != 4) 
	{
         cerr << "ERROR, Please provide <IP_Address> <Port_TCP> <Port_UDP>\n";
         exit(1);
     }
     else
     {
     	tcpport=atoi(argv[2]);
     	udpport=atoi(argv[3]);
          strcpy(serverIP,argv[1]);
     }
     
     
     int choice=0;
     while(choice<1 || choice>3) 
          {
               cout<<"\n Please enter option 1/2/3 to represent login type : \n1. Reporter\n2. Reader\n3. Administrator\n\n";
               choice=getch();
               choice-='0';
          }
          

     Client* _client=NULL;

     char ch;
     do
     {
          switch(choice)
     	  {
          case 1:   _client=new Reporter(serverIP,tcpport);
          break;
          case 2:   _client=new Reader(serverIP,tcpport+1);
          break;
          case 3:   {
		            // Input password and verify
		            cout << "\n Please enter password : ";

		            string pass ="";
		            char pass_ch;
		            pass_ch = getch();
		            while(pass_ch != '\n')
		            {//character 13 is enter
		                 pass.push_back(pass_ch);
		                 cout << '*';
		                 pass_ch = getch();
		            }

		            if(pass == "kharagpur")
		            {
		               cout << "\nAccess granted :P\n";
		              _client=new Administrator(serverIP,udpport);
		            }    
		            else
		            {
		               cout << "\nSorry ! Wrong password.. Access aborted...\n";
		               _client=NULL;
		            }

                    break;
		    }
          default:printf("Error ! No valid option \n");
          }
     
          if(_client!=NULL) _client->run();

          cout<<"Please enter # to quit or any other alphabet to continue..."<<endl<<endl;
          ch=getch();

     }while(ch!='#');

     system("clear");
     return 0;
}
