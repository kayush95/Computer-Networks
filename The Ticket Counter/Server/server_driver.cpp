#include <bits/stdc++.h>

#include "server.h"

#define PORT_NO 6500

using namespace std;

int main()
{
	Train Superfast = Train(12321,"Superfast Express");
	Train Rajdhani = Train(12301,"Rajdhani Express");

	char name_buf[4];
	for(int i=1;i<=3;i++)
	{
		sprintf(name_buf,"A%d",i);
		Superfast.addCoach(new _3T(Coach::AC,name_buf));
	}
	for(int i=1;i<=12;i++)
	{
		sprintf(name_buf,"S%d",i);
		Superfast.addCoach(new _3T(Coach::Sleeper,name_buf));
	}

	for(int i=1;i<=13;i++)
	{
		sprintf(name_buf,"A%d",i);
		Rajdhani.addCoach(new _2T(Coach::AC,name_buf));
	}

	list<Train> train_list;

	train_list.push_back(Superfast);
	train_list.push_back(Rajdhani);

	Server server=Server(PORT_NO,train_list);


	return 0;
}
