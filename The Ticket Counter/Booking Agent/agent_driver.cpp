
#include "booking_agent.h"

#define PORT_NO 6500

using namespace std;

int main(int argc,char* argv[])
{
	char serverIP[MAX_SIZE];//="10.105.75.28";


	if (argc != 2) 
	{
         cerr << "ERROR, Please provide <IP_Address> \n";
         exit(1);
    }
    else strcpy(serverIP,argv[1]);


    Booking_Agent BA = Booking_Agent(serverIP,PORT_NO);

    BA.run();

    cout << "bye"<<endl;

	return 0;
}
