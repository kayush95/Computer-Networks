#include <ctime>
#include <string>

// record.h
#ifndef RECORD_H
#define RECORD_H

#define MAX_BODY 1000
#define MAX_HEADLINE 100

struct record 
{
	const static int ACADEMIC=0;
	const static int NON_ACADEMIC=1;

	char headline[MAX_HEADLINE];
	time_t date_time;
	char date[11];
	
	char body[MAX_BODY];
	int type;	// ACADEMIC / NON_ACADEMIC
	bool locked; 	// false @ beginning // locked when in use by the user
};

#endif
