#include <ctime>
#include <string.h>
#include <stdio.h>
#include <bits/stdc++.h>

// request.h
#ifndef REQUEST_H
#define REQUEST_H

using namespace std;

#define MAX_SIZE 1024

class Request
{
public:

	int book_ID;
	int train_no;
	int coach_type; // either AC or Sleeper
	int num_of_berths;
	time_t timestamp;
	int berth_preferences[MAX_SIZE],pass_ages[MAX_SIZE];

	void error(const char* error_mssg)
    {
      perror(error_mssg);
      exit(1);
    }


public:

	Request()//int _id_,int _coach_type_,int _train_no_,int _num_of_berths_,time_t _timestamp_,int* prefer,int* ages) : coach_type(_coach_type_) ,book_ID(_id_) , train_no(_train_no_), num_of_berths(_num_of_berths_), timestamp(_timestamp_)
	{
		//berth_preferences = new int[num_of_berths];
		//pass_ages= new int[num_of_berths];
/*
		for(int i=0;i<num_of_berths;i++)
		{
			if(prefer!=NULL)
				berth_preferences[i]=prefer[i];
			pass_ages[i]=ages[i];
		}

		if(prefer==NULL)
			berth_preferences[0]=-1;//=NULL;
*/
	}
/*
	int* getBerthPreferences()
	{
		return berth_preferences;
	}

	int* getPassengerAges()
	{
		return pass_ages;
	}
*/

	//////////////////////////////////////////////////////////
	/*bool operator<(const Request& first)
	{  
	  	return ( this->num_of_berths > first.num_of_berths);
	}*/
	///////////////////////////////////////////////////////////

};


class Book_Details
{
public:
	int book_ID; // booking ID
	int num_of_berths;
	int train_no;
	
	string coaches[20];
	int position[20]; // any of the 5 Berth::constants
	int seat_no[20];

	/*void error(const char* error_mssg)
    {
      perror(error_mssg);
      exit(1);
    }*/


public:
	//Book_Details()//(int b_id,int _num_of_berths_, int train) : book_ID(b_id) , num_of_berths(_num_of_berths_)  , train_no(train)
	//{
		//position = new int[_num_of_berths_];
		//seat_no = new int[_num_of_berths_];
	//}

	// setter functions
/*
	vector<int> getSeatPositions()
	{
		return position;
	}

	vector<int> getSeatNos()
	{
		return seat_no;
	}

	vector<char*> getCoaches()
	{
		return coaches;
	}

*/

};

#endif
