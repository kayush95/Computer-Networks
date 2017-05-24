#include <string.h>
#include <stdio.h>
#include <bits/stdc++.h>

using namespace std;

// train.h
#ifndef TRAIN_H
#define TRAIN_H

#define MAX_SIZE 1024


class Berth
{
public:
	bool alloc;	// true if occupied/allocated
	int seat_no,position; // one of the 5 constants

	void error(const char* error_mssg)
    {
      perror(error_mssg);
      exit(1);
    }


public:
	const static int SU=1;
	const static int SL=2;
	const static int UB=3;
	const static int MB=4;
	const static int LB=5;

	bool isOccupied()
	{
		return alloc;
	}

	Berth(int seatno,int tier) : seat_no(seatno) // seatno. from 1 - 72 (_3T)	 or 	1 - 54 (_2T)
	{
		alloc=false;
		int mod;// = (tier==2) ? 6: ((tier==3) ? 8: 0);
		switch(tier)
		{
			case 2:
			{
				mod=6;
				switch((seatno-1)%mod)
				{
					case 0:
					case 2:
					 		position = LB;
					 		break;
					case 1:
					case 3:
					 		position = UB;
					 		break;
					case 4:	
							position = SL;
							break;
					case 5:
							position = SU;
							break;
					default:
						error("Impossible !");
				}
				break;
			}
			case 3:
			{
				mod=8;
				switch((seatno-1)%mod)
				{
					case 0:
					case 3:
					 		position = LB;
					 		break;
					case 1:
					case 4:
					 		position = MB;
					 		break;
					case 2:
					case 5:
					 		position = UB;
					 		break;
					case 6:	
							position = SL;
							break;
					case 7:
							position = SU;
							break;
					default:
						error("Impossible !");
				}
				break;
			}
			default:
				error("Erroneous tier value !");
		}
		

	}
};


class Coach
{	
public:
	char name[4];
	int type;	// AC or NON_AC
	vector<Berth> berths;
	int tier;

	Coach(int typ,char* nam) : type(typ)
	{
		strcpy(name,nam);
	}

	void error(const char* error_mssg)
    {
      perror(error_mssg);
      exit(1);
    }
public:
	// no. of compartments if fixed @ 9
	// no. of seats per coach = 6 (in 2T), 8 (in 3T)
	const static int AC=6;
	const static int Sleeper=7;	
	
    int getTier()
    {
    	return tier;
    }

	bool isAC()
	{
		return type==Coach::AC;
	}

	int getNoofAvailableSeats(bool opt=true)
	{
		int availCounter=0;
		for(vector<Berth>::iterator it=berths.begin(); it!=berths.end(); it++)
		{
			if(it->isOccupied() == !opt) availCounter++;
		}
		return availCounter;
	}

	int getSU()
	{
		int availCounter=0;
		for(vector<Berth>::iterator it=berths.begin(); it!=berths.end(); it++)
		{
			if(!it->alloc && it->position == Berth :: SU)
				availCounter++;
		}
		return availCounter;
	}
	int getSL()
	{
		int availCounter=0;
		for(vector<Berth>::iterator it=berths.begin(); it!=berths.end(); it++)
		{
			if(!it->alloc && it->position == Berth :: SL)
				availCounter++;
		}
		return availCounter;
	}
	int getUB()
	{
		int availCounter=0;
		for(vector<Berth>::iterator it=berths.begin(); it!=berths.end(); it++)
		{
			if(!it->alloc && it->position == Berth :: UB)
				availCounter++;
		}
		return availCounter;
	}
	int getMB()
	{
		int availCounter=0;
		for(vector<Berth>::iterator it=berths.begin(); it!=berths.end(); it++)
		{
			if(!it->alloc && it->position == Berth :: MB)
				availCounter++;
		}
		return availCounter;
	}
	int getLB()
	{
		int availCounter=0;
		for(vector<Berth>::iterator it=berths.begin(); it!=berths.end(); it++)
		{
			if(!it->alloc && it->position == Berth :: LB)
				availCounter++;
		}
		return availCounter;
	}

	
};

class _2T : public Coach
{
public:
	_2T(int typ,char* nam) : Coach(typ,nam)
	{
		tier=2;
		for(int sno=1;sno<=54;sno++)
		{
			Berth brth = Berth(sno,2);
			berths.push_back(brth);
		}
	}
};

class _3T : public Coach
{
public:
	_3T(int typ,char* nam) : Coach(typ,name)
	{
		tier=3;
		for(int sno=1;sno<=72;sno++)
		{
			Berth brth = Berth(sno,3);
			berths.push_back(brth);
		}
	}
};

class Train
{

public:
	int ID;
	char name[MAX_SIZE];
	vector<Coach*> coaches;

	void error(const char* error_mssg)
    {
      perror(error_mssg);
      exit(1);
    }


public:
	Train(int id,const char _name_[MAX_SIZE]) : ID(id)
	{
		strcpy(name,_name_);
	}
	
	void addCoach(Coach* c)
	{
		this->coaches.push_back(c);
	}

	int getNoofAvailableSeats()
	{
		int availCounter=0;

		for(vector<Coach*>::iterator it=coaches.begin(); it!=coaches.end() ; it++)
		{
			availCounter+= (*it)->getNoofAvailableSeats();
		}

		return availCounter;
	}

	int getNoofAvailableSeats(bool var1,bool var2,bool var3 = true)	
	{
		/**************************************
		var1 = true  for tier 2
		var1 = false for tier 3

		var2 = true  for AC
		var2 = false for sleeper

		var3 = true for AVAILABLE_ONLY option
		var3 = false for OCCUPIED_ONLY option

		**************************************/

		int availCounter=0;

		for(vector<Coach*>::iterator it=coaches.begin(); it!=coaches.end() ; it++)
		{
			//cout<< " typeid(**it) : " << typeid(**it).name() << " typeid(_2T) : " << typeid(_2T).name() << " == " << (typeid(**it)==typeid(_2T)) << endl;
			if(var1 && var2 && (*it)->getTier()==2 && (*it)->isAC())
				availCounter+= (*it)->getNoofAvailableSeats(var3);
			
			else if(var1 && !var2 && (*it)->getTier()==2 && !(*it)->isAC())
				availCounter+= (*it)->getNoofAvailableSeats(var3);
			
			else if(!var1 && var2 && (*it)->getTier()==3 && (*it)->isAC())
				availCounter+= (*it)->getNoofAvailableSeats(var3);
			
			else if(!var1 && !var2 && (*it)->getTier()==3 && !(*it)->isAC())
				availCounter+= (*it)->getNoofAvailableSeats(var3);
		}

		return availCounter;
	}

	char* getName()
	{
		return strdup(name);
	}

	int getID()
	{
		return ID;
	}

};

#endif
