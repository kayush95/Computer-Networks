#include <ctime>
#include <string.h>
#include <iostream>

using namespace std;

// email.h
#ifndef EMAIL_H
#define EMAIL_H

#define MAX_BODY 500
#define MAX_SUBJECT 50

struct email 
{
	char from[40];	// email address : username@domain.com
	char to[40];	// email address : username@domain.com
	char subject[MAX_SUBJECT];
	time_t timestamp;
	char body[MAX_BODY];

	email(char from[40],char to[40],char subject[MAX_SUBJECT],char body[MAX_BODY])
	{
		strcpy(this->from,from);
		strcpy(this->to,to);
		strcpy(this->subject,subject);
		strcpy(this->body,body);
		timestamp = time(0);
	}

	email()
	{
		
	}

	void print()
	{
		cout << "\n\n----------------------------------------------\n\n";
		cout << "FROM : " << from << endl;
		cout << "TO : " << to << endl;
		cout << "SUBJECT : " << subject << endl;
		cout << "\nBODY : \n" << body << endl;
		cout << "\n----------------------------------------------\n\n";
	}
};

#endif
