#include "server.h"

using namespace std;

int main(int argc, char* argv[])
{

	if(argc != 5)
	{
		printf("- Invalid parameters!!!\n");
		printf("- Usage %s <source hostname/IP> <source port> <target hostname/IP> <target port>\n", argv[0]);
		exit(-1);
	}

	RTLP_Server server= RTLP_Server(argv[1],atoi(argv[2]),argv[3],atoi(argv[4]));

	server.run();
	
	return 0;
}
