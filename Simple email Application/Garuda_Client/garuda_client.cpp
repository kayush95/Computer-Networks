
#include "../SMTP_client.h"
#include "../POP_client.h"

using namespace std;

#define FIRST_SERVER_PORT__SMTP  6000
#define FIRST_SERVER_PORT__POP   7000

#define SECOND_SERVER_PORT__SMTP  8000
#define SECOND_SERVER_PORT__POP   9000

/******************************** input both ips in makefile **************************************/
/******************************** input both ips in makefile **************************************/
/******************************** input both ips in makefile **************************************/
/******************************** input both ips in makefile **************************************/
/******************************** input both ips in makefile **************************************/
/******************************** input both ips in makefile **************************************/


int main(int argc,char* argv[])
{
     char serverIP[20];
     char server_domain_name[20];
     int choice=-1;

     if (argc == 3) 
     {
          strcpy(serverIP,argv[1]);
          strcpy(server_domain_name,argv[2]);
     }
     else if (argc == 4) 
     {
      	strcpy(serverIP,argv[1]);
        strcpy(server_domain_name,argv[2]);
	     choice = atoi(argv[3]);
     }
     else
     {
         cerr << "ERROR, Please provide <IP_Address> <domain_name> \n";
         exit(1);
     }
     
     if( choice == -1)
     {
	     while(choice<1 || choice>3) 
		  {
		       cout<<"\n Please enter option 1 / 2 / 3 : \n1. Send New Email\n2. Retrieve Inbox Emails\n3. Retreive mails from both domains\n\n";
		   
		       choice=getch();
		       choice-='0';

		       system("clear");
		  }
     }

          SMTP_Client* _smtp_client=NULL;
          POP_Client* _pop_client=NULL;

     char ch='A';
     do
     {
     	system("clear");

          switch(choice)
    	 {
	          case 1:  
	          {	
	          		if(strcmp(server_domain_name,"abc.com")==0)
	          		 _smtp_client=new SMTP_Client(serverIP,FIRST_SERVER_PORT__SMTP);
	          		else if(strcmp(server_domain_name,"xyz.com")==0)
	          		 _smtp_client=new SMTP_Client(serverIP,SECOND_SERVER_PORT__SMTP);
	          		else 
	          			cout << "Please choose between abc.com and xyz.com"<<endl;
	         	
	         	 	break;
	          }
	          case 2: 
	           {	
	          		if(strcmp(server_domain_name,"abc.com")==0)
	          		   _pop_client=new POP_Client(serverIP,FIRST_SERVER_PORT__POP);
	          		else if(strcmp(server_domain_name,"xyz.com")==0)
	          			_pop_client=new POP_Client(serverIP,SECOND_SERVER_PORT__POP);
	          		else 
	          			cout << "Please choose between abc.com and xyz.com"<<endl;
	         	
	         	 	break;
	          }
      		  case 3:
      		  {
      			
      		  	system("xterm -e ./garuda_client 127.0.0.1 xyz.com 2 &");
      		  	system("xterm -e ./garuda_client 10.42.0.1 abc.com 2 &");
              break;
      		  }
            
	          default:printf("Error ! No valid option \n");
     	}
     
          if(_smtp_client!=NULL) _smtp_client->run();
          if(_pop_client!=NULL) _pop_client->run();

          cout<<"Press E to exit or any other key to continue..."<<endl<<endl;
          ch=getch();

     }while(ch!='E' && ch!='e');

     
          
/*
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
                    cout << "\nAuthorization required to log in as Admin...\n Please enter password : ";

                    string pass ="";
                      char pass_ch;
                      pass_ch = getch();
                          while(pass_ch != '\n')
                          {//character 13 is enter
                             pass.push_back(pass_ch);
                             cout << '*';
                             pass_ch = getch();
                          }

                     if(pass == "kgp")
                     {
                        cout << "\nAccess granted :P\n";
                       _client=new Administrator(serverIP,udpport);
                    }    
                    else
                    {
                       cout << "\nSorry ! Wrong password.. Access aborted...\n";
                        _client=NULL;
                     }

                     break;}
          default:printf("Error ! No valid option \n");
     }
     
          if(_client!=NULL) _client->run();

          cout<<"Please enter # to quit or any other alphabet to continue..."<<endl<<endl;
          ch=getch();

     }while(ch!='#');

     system("clear");
     */
	return 0;
}
