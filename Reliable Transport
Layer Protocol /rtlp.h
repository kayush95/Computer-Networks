#include <ctime>
#include <string.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <bits/stdc++.h>

// rtlp.h
#ifndef RTLP_H
#define RTLP_H

using namespace std;

#define MAX_SIZE 1024
#define SYN 1
#define ACK 2
#define DATA 3
#define FIN 4
#define SYN_ACK 5
#define DATA_ACK 6
#define FIN_ACK 7

// primes used for computing hash function
#define PRIME1 13
#define PRIME2 17
#define PRIME3 19
#define PRIME4 23
#define PRIME5 29

/* Structure of a RTLP header */
struct rtlpheader 
{
	unsigned short int rtlph_srcport;
	unsigned short int rtlph_destport;
	unsigned int rtlph_seqnum;
	unsigned int rtlph_acknum;
	unsigned short int type;	
	unsigned int rtlph_chksum;           
};

// Function for checksum calculation. From the RFC,
// the checksum algorithm is:
//  "The checksum field is the 16 bit one's complement of the one's
//  complement sum of all 16 bit words in the header.  For purposes of
//  computing the checksum, the value of the checksum field is zero."
/*unsigned int csum(unsigned short *buf, int len)
{
	unsigned long sum;
	for(sum=0; len>0; len--)
		sum += *buf++;
	sum = (sum >> 16) + (sum &0xffff);
	sum += (sum >> 16);
	return (unsigned int)(~sum);
}*/


unsigned int csum(char *packet)
{
    struct iphdr *ip = (struct iphdr *)packet;
    struct rtlpheader *rtlp = (struct rtlpheader *)(packet + sizeof(struct iphdr));
    int *data = (int *)(packet + sizeof(struct rtlpheader) + sizeof(struct iphdr));

    unsigned int _check_sum_ = 0;

    _check_sum_ = rtlp->rtlph_srcport;
    _check_sum_ *= PRIME1;
    _check_sum_ += rtlp->rtlph_destport;
    _check_sum_ *= PRIME2;
    _check_sum_ += rtlp->rtlph_seqnum;
    _check_sum_ *= PRIME3;
    _check_sum_ += rtlp->rtlph_acknum;
    _check_sum_ *= PRIME4;
    _check_sum_ += *data;
    _check_sum_ *= PRIME5;

    return _check_sum_;
}
#endif



