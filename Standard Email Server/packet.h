#include <string.h>

// packet.h
#ifndef PACKET_H
#define PACKET_H

#define MAX_MSSG_SIZE 100

// an SMTP data packet template which is sent to and fro between the SMTP Server and SMTP Client

struct packet 
{
	/***** SMTP Client Command Codes ***************/

	static const int FRWD = 5;	// used for forwarding / redirecting / mirroring email to its respective server (example -> abc.com to xyz.com)

	static const int HELO = 10;
	static const int MAIL_FROM = 20;
	static const int RCPT_TO = 30;
	static const int DATA = 40;

	static const int HEADER_FROM 	= 50;
	static const int HEADER_TO 		= 60;
	static const int HEADER_DATE	= 70;
	static const int HEADER_SUBJECT = 80;

	static const int BLANK = 90;
	static const int BODY = 100; // in this type, the mssg part is appended to the email body until a '.' (dot) is encountered

	static const int SAML = 101;
	static const int VRFY_USER = 1020;
	static const int VRFY_PWD = 1021;

	static const int RSET = 103;
	static const int QUIT = 104;

	/****** POP Client Command Coded ***************/

	static const int STAT = 110;
	static const int LIST = 120;
	static const int RETR = 130;
	// VRFY_USER , VRFY_PWD and QUIT are common with SMTP code set

	/***** SMTP Server Command Codes ***************/

	static const int service_ready = 220;
	static const int OK = 250;
	static const int start_mail_input = 354;
	static const int service_closed = 221;

	int command_code;	// when sent by the SMTP client, command code can be one of the command types defined in the macro above
		// when sent by the SMTP server, command codes are like 250, 354, 220, 221 that indicate the specific purpose of the SMTP packet
	char mssg[MAX_MSSG_SIZE];

	packet(int code,const char message[MAX_MSSG_SIZE])
	{
		command_code = code;
		strcpy(mssg,message);
	}

	packet()
	{

	}
};

#endif