/* trainsim.c
        User program version of project 1's train simulation
  
        Use system calls so as not to directly make calls to protected operations
  
   Copyright (c) 1992-1993 The Regents of the University of California.
   All rights reserved.  See copyright.h for copyright notice and limitation
   of liability and disclaimer of warranty provisions.
*/

#include "syscall.h"

typedef enum { false, true } bool;
#define false 0
#define true 1

#define NULL -1


/*----------------------------------------------------------------------
   TrainSIM
        Setup and run a train simulation
                -josh
----------------------------------------------------------------------*/

       
/*------------------
  INTERNAL STRUCTS      
  ------------------*/

/*a simple structure for holding a passenger's ticket info*/

typedef struct {
        int getOnStop, getOffStop;
        int holder;
        int okToBoard;
	int seatNumber;
	bool hasBedding;
	bool hasEaten;
	bool hasSlept;
	int foodOrder;
	bool validFoodOrder;
	int waiter;
} Ticket;




	

/*---------------------
   GLOBAL DATA
---------------------*/
bool TEST_1 = false;
bool TEST_2 = false;
bool TEST_3 = false;
bool TEST_4 = false;
bool TEST_5 = false;

int passengersFinished = 0;
int passengersEaten = 0;
int passengersSlept = 0;

#define MAX_STOPS 10
#define DINING_SEATS 10
#define PASSENGERS 20
#define COACH_ATTENDANTS 3
#define PORTERS 3
#define WAITERS 3
#define CHEFS 2

#define MIN_STOCK 2
#define RESTOCK_AMOUNT 5

int passengerID = 0;
int coachAttendantID = 0;
int porterID = 0;
int waiterID = 0;
int chefID = 0;

int IDLock;

int totalRevenue=0;
int ticketRevenue=0;
int diningCarRevenue=0;
int waiterRevenue=0;

int currentStop = -1;
int passengersOnTrain = 0;

int foodStock[4] = {10,10,10,10};
bool foodStockLow[4] = {false,false,false,false};

bool nowStopped=false;
int atAStopLock;
int getOffAtThisStopLock;
int getOffAtThisStop;

int ticketRevenueLock;
int diningCarRevenueLock;
int waiterRevenueLock;
/*no need for total revenue lock as only conductor will total these*/

/* data for passenger/ticketChecker interactions*/
int passengersWaitingToBoard[MAX_STOPS];
int ticketCheckerLineLength = 0;
int ticketCheckerAvailable = false;
int waitingForTrain[MAX_STOPS];/*should this be MAX_STOPS-1?*/
int ticketChecker; /*TC waits on this for a passenger*/
int ticketCheckerLine; /*passengers wait on this when TC is unavailable*/
int waitingForTrainLock[MAX_STOPS];/*same issue?*/
int ticketCheckerLock;
Ticket* ticketToCheck;
int ticketLock;
int ticketCheck;
/*--end passenger/ticketChecker data*/

/*data for CoachAttendant/Passenger interactions*/
int coachAttendantRegularLineLength[COACH_ATTENDANTS];
int coachAttendantFirstLineLength[COACH_ATTENDANTS];
int coachAttendantAvailable[COACH_ATTENDANTS];
int coachAttendant[COACH_ATTENDANTS];
int coachAttendantRegularLine[COACH_ATTENDANTS];
int coachAttendantFirstLine[COACH_ATTENDANTS];
int coachAttendantLine[COACH_ATTENDANTS];
int coachAttendantLock[COACH_ATTENDANTS];
int coachAttendantLineLock[COACH_ATTENDANTS];
int globalSeatLock;
int nextSeat = 0;
int getSeatLock[COACH_ATTENDANTS];
int getSeat[COACH_ATTENDANTS];
Ticket* passengerTickets[COACH_ATTENDANTS];
int foodOrdersWaiting[COACH_ATTENDANTS];
int CAWaitingForPorter[COACH_ATTENDANTS];
int CAWaitingForPorterLock[COACH_ATTENDANTS];
int coachAttendantFoodLineLength[COACH_ATTENDANTS];
int coachAttendantFoodLine[COACH_ATTENDANTS];
int coachAttendantGetFood[COACH_ATTENDANTS];
int coachAttendantGetFoodLock[COACH_ATTENDANTS];
Ticket* coachAttendantOrder[COACH_ATTENDANTS];
/*--end CoachAttendant/Passenger interactions*/

/*data for Porter/Passenger interactions*/
int porterBaggageLineLength[PORTERS];
int porterAvailable[PORTERS];
int porter[PORTERS];
int porterBaggageLine[PORTERS];
int porterLine[PORTERS];
int porterLock[PORTERS];
int porterBaggageLineLock[PORTERS];
Ticket* passengerBaggage[PORTERS];
int porterBeddingLineLength[PORTERS];
int porterFirstClassBeddingLineLength[PORTERS];
int porterFirstClassBeddingLine[PORTERS];
int porterBeddingLine[PORTERS];
int porterBeddingLineLock[PORTERS];
Ticket* passengerBedding[PORTERS];
/*--end Porter/Passenger data*/

/*data for Waiter/Passenger interactions*/
int waiterLineLength[WAITERS];
Ticket* passengerFood[WAITERS];
int waiterLock[WAITERS];
int waiter[WAITERS];
int waiterLineLock[WAITERS];
int waiterLine[WAITERS];
int waiterGetFoodLock[WAITERS];
int waiterGetFood[WAITERS];
int globalFoodLock;
/*--end Waiter/Passenger data*/

/*data for ticketChecker/Conductor interactions*/
int passengerCountLock;
int waitingForGoLock;
int waitingForStopLock;
int waitingForGo;
int waitingForStop;
bool ticketCheckerWaiting =false;
bool conductorWaiting =false;
/*--end ticketChecker/Conductor data*/

/*data for Conductor/CoachAttendant interactions*/
int waitingForBoardingCompletionLock;
int waitingForBoardingCompletion;
int boardingPassengersCountLock;
int passengersBoardedAtThisStop[MAX_STOPS];
/*--end conductor/CA interactions*/

/*data for steward/passenger interations*/
int stewardLock;
int steward;
int stewardLineLength = 0;
int diningCarCapacity = DINING_SEATS;
int diningCarCapacityLock;
bool stewardAvailable = false;
Ticket* stewardTicketToCheck;
int stewardGiveWaiterLock;
int stewardGiveWaiter;
int stewardLine;
int finalConfirmation;
/*--end steward/passenger data*/

/*data for steward/waiter interactions*/
bool waiterOnBreak[WAITERS];
/*--end steward/waiter data*/

/*data for chef/ANYONE interactions*/
int chefLock[CHEFS];
int chefOrdersLineLock[CHEFS];
int chef[CHEFS];
int chefRegularOrdersLine[CHEFS];
int chefFirstClassOrdersLine[CHEFS];
bool chefOnBreak[CHEFS];
int chefFirstClassOrdersLeft[CHEFS];
int chefRegularOrdersLeft[CHEFS];
/*--end chef/ANYONE shared data*/

/*HELPER FUNCTIONS
  ----------------*/

/*a function to generate a random greater than the start value and less than the range value*/
int randomRange(unsigned int start, unsigned int range){
          /* Simple "srand()" seed: just use "time()" */
        /*unsigned int iseed = (unsigned int)time(NULL);*/
        /*srand (iseed);*/
        /*reuse of iseed for brevity*/
        unsigned int iseed;
        if(range<=start){
                return NULL;
        } else{
                iseed = (Rand()%range);
                /*generate random numbers until we have one greater than our start*/
                        while(iseed<=start){
                                iseed = (Rand()%range);
                        }
                return iseed;
        }
}


/*----------------
   THREAD TYPES
----------------*/

void Passenger(){
        unsigned int myID;
        int i = 0;
        int shortestLength;
        
        int one[1];
	int two[2];
	int three[3];
	int four[4];

        
	int myPorter=-1;
	int myCoachAttendant = -1; /* initially no coach attendant associated*/
										/* with this passenger*/
										
	/*passengers have to remember whether they simply walked up to the ticketChecker, or waited in line*/
        /*this affects if they decrement the line count or not*/
        bool passengerGotInLine = NULL;
        /*initialization of ticket structure*/
	bool disembarked =false;
        Ticket myTicket;
	/*decide if passenger is first class or not*/
	bool isFirstClass = Rand()%2;
	ticketRevenue+= 2 +(1*(int)isFirstClass);
	
	Acquire(IDLock);
	myID = passengerID;
	passengerID += 1;
	Release(IDLock);
        
        /*assumes passengers only get off at the last stop(for now)*/
        myTicket.getOnStop = (Rand()%(MAX_STOPS-1));
        myTicket.getOffStop = randomRange(myTicket.getOnStop,MAX_STOPS);
        myTicket.holder=myID;/*FOR DEBUGGING*/
       	myTicket.hasEaten = false;
	myTicket.hasSlept = false;
	myTicket.hasBedding = false;
	myTicket.foodOrder = -1;
	myTicket.validFoodOrder = false;
	myTicket.waiter = -1;
	one[0] = myID;
	Print("Passenger %d belongs to Train 0\n",sizeof("Passenger %d belongs to Train 0\n"),one,1);
	two[0] = myID;
	two[1] = myTicket.getOnStop;
	Print("Passenger %d is going to get on the Train at stop number %d\n",sizeof("Passenger %d is going to get on the Train at stop number %d\n"),two,2);
	two[1] = myTicket.getOffStop;
	Print("Passenger %d is going to get off the Train at stop number %d\n",sizeof("Passenger %d is going to get off the Train at stop number %d\n"),two,2);

        /*passenger is now ready to go to a stop and wait to board the train*/
        Acquire(waitingForTrainLock[myTicket.getOnStop]);
        /*so he increments the number of people at the stop and waits*/
        passengersWaitingToBoard[myTicket.getOnStop] += 1;
        /*printf("passenger%d is in line at stop %d\n",myID, myTicket.getOnStop);*/
        Wait(waitingForTrain[myTicket.getOnStop], waitingForTrainLock[myTicket.getOnStop]);
        Release(waitingForTrainLock[myTicket.getOnStop]);

        /*and then checks on the ticketChecker*/
        Acquire(ticketCheckerLock);
        if(!ticketCheckerAvailable){
                /*passenger gets in line*/
                passengerGotInLine=true;
                ticketCheckerLineLength += 1;
                Wait(ticketCheckerLine, ticketCheckerLock);
        }
        else {
                /*ticket checker is free so passenger goes up to him*/
                ticketCheckerAvailable = false;
                passengerGotInLine=false;
        }
        /*hand TC our ticket and wait to see what he says*/
        Acquire(ticketLock);
        ticketToCheck = &myTicket;
        Signal(ticketChecker, ticketCheckerLock);
        Release(ticketCheckerLock);
        /*printf("Passenger%d has signaled and is now waiting on ticketChecker\n", myID);*/
        Wait(ticketCheck, ticketLock);
        /*let the ticketChecker go on to do other things, and see if we should get on the train*/
        if(passengerGotInLine)
                ticketCheckerLineLength -= 1;
        if(!myTicket.okToBoard){
                /*i had a bad ticket*/
                /*print msg and return*/
                /*printf("passenger%d had a bad ticket and is leaving\n", myID);*/
		Print("Passenger %d of Train 0 has an invalid ticket\n",sizeof("Passenger %d of Train 0 has an invalid ticket\n"),one,1);
		Release(ticketLock);
                return;
        }
		Print("Passenger %d of Train 0 has a valid ticket\n",sizeof("Passenger %d of Train 0 has a valid ticket\n"),one,1);
        /*I can get on the train and wait for my next thing*/
        /*printf("passenger%d got on the train\n", myID);*/
        /*ticketCheckerLock->Release();*/
        Release(ticketLock);

	/*Passenger is now on the train and checking on the CoachAttendants*/
	passengerGotInLine = NULL;  /*Reusing this since it's not needed anymore for P/TC interaction*/
	
	/*Check for any available CA's first*/
	for(i = 0; i < COACH_ATTENDANTS; i += 1){
		Acquire(coachAttendantLock[i]);
		if(coachAttendantAvailable[i]){		
			coachAttendantAvailable[i] = false;
			/*passengerGotInLine = false;*/
			myCoachAttendant= i;
			/*my coach attendant was waiting for work, so i come up and signal him*/
			Signal(coachAttendant[myCoachAttendant], coachAttendantLock[myCoachAttendant]);
			break;
		}
		else{
			Release(coachAttendantLock[i]);
		}
	}
	/*If no available CA could be found, then get in the shortest line*/
	if(myCoachAttendant == -1){
		myCoachAttendant = 0;
		Acquire(coachAttendantLock[0]);
		shortestLength = isFirstClass ? coachAttendantFirstLineLength[0] : coachAttendantRegularLineLength[0];
		
		for(i = 1; i < COACH_ATTENDANTS; i += 1){
			Acquire(coachAttendantLock[i]);
			if(isFirstClass){
				if(coachAttendantFirstLineLength[i] < shortestLength){
					shortestLength = coachAttendantFirstLineLength[i];
					Release(coachAttendantLock[myCoachAttendant]);
					myCoachAttendant = i;
					
				}
				else{
					Release(coachAttendantLock[i]);
				}
			}
			else{
				if(coachAttendantRegularLineLength[i] < shortestLength){
					shortestLength = coachAttendantRegularLineLength[i];
					Release(coachAttendantLock[myCoachAttendant]);
					myCoachAttendant = i;
				}
				else{
					Release(coachAttendantLock[i]);
				}
			}
		}
		/*signal our coach attendant*/
		/*printf("passenger%d getting in coachAttendant%d\'s line\n", myID, myCoachAttendant);*/
		Signal(coachAttendant[myCoachAttendant], coachAttendantLock[myCoachAttendant]); /*QUESTIONABLE*/
	}
	
	Acquire(coachAttendantLineLock[myCoachAttendant]);
	/*tell CA what kind of passenger you are*/
	if (isFirstClass){ 
		coachAttendantFirstLineLength[myCoachAttendant] += 1;
	}
	else{
		coachAttendantRegularLineLength[myCoachAttendant] += 1;
	}
	
	Release(coachAttendantLock[myCoachAttendant]);
	
	/*printf("passenger%d waiting in coachAttendant%d\'s line\n", myID, myCoachAttendant);*/
	isFirstClass ? 
		Wait(coachAttendantFirstLine[myCoachAttendant], coachAttendantLineLock[myCoachAttendant] ) : 
		Wait(coachAttendantRegularLine[myCoachAttendant], coachAttendantLineLock[myCoachAttendant] );
	/*printf("passenger%d is moving up to coachAttendant%d\n", myID, myCoachAttendant);*/
	
	
	/*Passenger is ready to get a seat now*/
	Acquire(getSeatLock[myCoachAttendant]);

	/*now that coach attendant is ready for me i hand him my ticket so he knows my name*/
	passengerTickets[myCoachAttendant] = &myTicket;
	Signal(coachAttendantLine[myCoachAttendant], coachAttendantLineLock[myCoachAttendant] );
	Release(coachAttendantLineLock[myCoachAttendant]);
	/*printf("Passenger%d has signaled and is now waiting on coachAttendant%d\n", myID, myCoachAttendant);*/
	
	Wait(getSeat[myCoachAttendant], getSeatLock[myCoachAttendant] );

	if(!isFirstClass){
		three[0] = myID;
		three[1] = myTicket.seatNumber;
		three[2] = myCoachAttendant;
		Print("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n", sizeof("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n"), three, 3);
	}

	if(isFirstClass)
		Acquire(CAWaitingForPorterLock[myCoachAttendant]);

	Release(getSeatLock[myCoachAttendant]);/*end of regular class passenger/CA*/

	if(isFirstClass){
	/*Do porter stuff*/
		/*since it's a first class passenger, get a porter*/
		/*Check for any available porters first*/
		for(i = 0; i < PORTERS; i += 1){
			Acquire(porterLock[i]);
			if(porterAvailable[i]){			
				porterAvailable[i] = false;
				/*passengerGotInLine = false;*/
				myPorter= i;
				/*my porter was waiting for work, so i come up and signal him*/
				Signal(porter[myPorter], porterLock[myPorter]);
				break;
			}
			else{
				Release(porterLock[i]);
			}
		}
		/*No Porter was available, so I have to get in line*/
		if(myPorter == -1){
			myPorter = 0;
			Acquire(porterLock[0]);
			shortestLength = porterBaggageLineLength[0];
		
			for(i = 1; i < PORTERS; i += 1){
				Acquire(porterLock[i]);
				if(porterBaggageLineLength[i] < shortestLength){
					shortestLength = porterBaggageLineLength[i];
					Release(porterLock[myPorter]);
					myPorter = i;
					
				}
				else{
					Release(porterLock[i]);
				}				
			}

			/*printf("passenger%d getting in porter%d\'s line\n", myID, myPorter);*/
			Signal(porter[myPorter], porterLock[myPorter]); /*QUESTIONABLE*/
		}
		Acquire(porterBaggageLineLock[myPorter]);
		porterBaggageLineLength[myPorter] += 1;
		Release(porterLock[myPorter]);
		
		/*printf("passenger%d waiting in porter%d\'s line\n", myID, myPorter);*/
		Wait(porterBaggageLine[myPorter], porterBaggageLineLock[myPorter]);
		/*printf("passenger%d is giving bags up to porter%d\n", myID, myPorter);*/
	
		/*show the porter my name*/
		passengerBaggage[myPorter]=&myTicket;
		Signal(porterLine[myPorter], porterBaggageLineLock[myPorter] );
		Release(porterBaggageLineLock[myPorter]);

		Signal(CAWaitingForPorter[myCoachAttendant], CAWaitingForPorterLock[myCoachAttendant]);
		Release(CAWaitingForPorterLock[myCoachAttendant]);
		three[0] = myID;
		three[1] = myTicket.seatNumber;
		three[2] = myCoachAttendant;
		Print("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n",sizeof("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n"), three, 3);
		two[0] = myID;
		two[1] = myPorter;
		Print("1st class Passenger %d of Train 0 is served by Porter %d\n",sizeof("1st class Passenger %d of Train 0 is served by Porter %d\n"),two,2); 
				
	}
	/*HAVE TO SET THESE BACK, due to bullhockey- they might be used later for ordering food or bedding*/
	myPorter=-1;
        myCoachAttendant=-1;

	while(!myTicket.hasEaten || !myTicket.hasSlept){
		if(!myTicket.hasEaten){
			/*Randomly decide to get hungry*/
			if(Rand()%4 == 0){
				/*printf("passenger%d is now hungry\n",myID);*/
				/*Acquire food somehow (depends on passenger class)*/
				if(isFirstClass){
					/*Do CA/Passenger food interaction*/
					for(i = 0; i < COACH_ATTENDANTS; i += 1){
						Acquire(coachAttendantLock[i]);
						if(coachAttendantAvailable[i]){			
							coachAttendantAvailable[i] = false;

							myCoachAttendant= i;
							/*my CA was waiting for work, so i come up and signal him*/
							Signal(coachAttendant[myCoachAttendant], coachAttendantLock[myCoachAttendant]);
							break;
						}
						else{
							Release(coachAttendantLock[i]);
						}
					}
					/*No CA was available, so I have to get in line*/
					if(myCoachAttendant == -1){
						myCoachAttendant = 0;
						Acquire(coachAttendantLock[0]);
						shortestLength = coachAttendantFoodLineLength[0];
					
						for(i = 1; i < COACH_ATTENDANTS; i += 1){
							Acquire(coachAttendantLock[i]);
							if(coachAttendantFoodLineLength[i] < shortestLength){
								shortestLength = coachAttendantFoodLineLength[i];
								Release(coachAttendantLock[myCoachAttendant]);
								myCoachAttendant = i;
								
							}
							else{
								Release(coachAttendantLock[i]);
							}				
						}

						/*printf("passenger%d getting in porter%d\'s line\n", myID, myPorter);*/
						Signal(coachAttendant[myCoachAttendant], coachAttendantLock[myCoachAttendant]); /*QUESTIONABLE*/
					}
					Acquire(coachAttendantLineLock[myCoachAttendant]);
					coachAttendantFoodLineLength[myCoachAttendant] += 1;
					Release(coachAttendantLock[myCoachAttendant]);
		
					/*printf("passenger%d waiting in coachAttendant%d\'s food line\n", myID, myCoachAttendant);*/
					Wait(coachAttendantFoodLine[myCoachAttendant], coachAttendantLineLock[myCoachAttendant]);
					/*now I have my CA's attention, so I try to order*/
					Acquire(coachAttendantGetFoodLock[myCoachAttendant]);
					Release(coachAttendantLineLock[myCoachAttendant]);

					do{
						myTicket.foodOrder = Rand()%4;
						coachAttendantOrder[myCoachAttendant] = &myTicket;
						Signal(coachAttendantGetFood[myCoachAttendant], coachAttendantGetFoodLock[myCoachAttendant] );
						Wait(coachAttendantGetFood[myCoachAttendant], coachAttendantGetFoodLock[myCoachAttendant] );
					} while (!myTicket.validFoodOrder);
					/*i've been given the food, so I tip the CA*/
					Signal(coachAttendantGetFood[myCoachAttendant], coachAttendantGetFoodLock[myCoachAttendant]);
					myTicket.hasEaten=true;
					Release(coachAttendantGetFoodLock[myCoachAttendant]);/*end 1stclass/CA	*/
				}
				else {
					/*Do Waiter/Passenger food interaction*/
					Acquire(stewardLock);
					if(stewardAvailable){
						stewardAvailable=false;
						Signal(steward, stewardLock);
					}
					stewardLineLength += 1;
					Wait(stewardLine, stewardLock); /* now no matter what I have the steward's attention*/
					Acquire(stewardGiveWaiterLock); /* i'm not sure we need this lock*/
					stewardTicketToCheck=&myTicket;
					Signal(steward, stewardLock);
					Release(stewardLock);
					Wait(stewardGiveWaiter, stewardGiveWaiterLock);/*definitely need this cv*/
					/*I either have a waiter or not*/
					if(myTicket.waiter==-1){
						Release(stewardGiveWaiterLock);
						/*printf("passenger%d couldn't get into the dining car\n",myID);*/
						/*go wait around until you can get in :'[*/
						one[0] = myID;
						Print("Passenger %d of Train 0 is informed by Stewart-the Dining Car is full\n",sizeof("Passenger %d of Train 0 is informed by Stewart-the Dining Car is full\n"),one,1);

					}
					else{
						one[0] = myID;
						Print("Passenger %d of Train 0 is informed by Stewart-the Dining Car is not full\n",sizeof("Passenger %d of Train 0 is informed by Stewart-the Dining Car is not full\n"),one,1);
						/*i actually have a waiter*/
						/*printf("passenger%d was put into the care of waiter%d\n",myID,myTicket.waiter);*/
						Acquire(waiterLineLock[myTicket.waiter]);
						Release(stewardGiveWaiterLock);
						waiterLineLength[myTicket.waiter] += 1;
						Acquire(stewardLock);
						Signal(steward, stewardLock);
						/*stewardLineLength -= 1;*/
						Release(stewardLock);/*if i get context switched here.. i lose*/
						/*two possibilities - one, i get to wait first, and things are fine...*/
						/*two, waiter goes first, sees someone was in line, signals, but i'm not waiting*/
							/*THIS IS NOW FIXED- steward waits for you to signal him that you have your waiter*/
						/*printf("passenger%d dies here\n",myID);*/
						Wait(waiterLine[myTicket.waiter], waiterLineLock[myTicket.waiter]);
						/*printf("passenger%d didn't die there\n", myID);*/
						Acquire(waiterGetFoodLock[myTicket.waiter]);
						Acquire(stewardLock);
						stewardLineLength -= 1;
						Signal(finalConfirmation, stewardLock);
						Release(stewardLock);/*end passenger/steward*/
						/*printf("passenger%d got the food lock\n",myID);*/
						Release(waiterLineLock[myTicket.waiter]);
						/*i've been woken up so i give my order*/
						
						do{
							myTicket.foodOrder = Rand()%4;
							passengerFood[myTicket.waiter] = &myTicket;
							Signal(waiterGetFood[myTicket.waiter], waiterGetFoodLock[myTicket.waiter] );
							Wait(waiterGetFood[myTicket.waiter], waiterGetFoodLock[myTicket.waiter] );
						} while (!myTicket.validFoodOrder);
						two[0] = myID;
						two[1] = myTicket.waiter;
						Print("Passenger %d of Train 0 is served by Waiter %d\n",sizeof("Passenger %d of Train 0 is served by Waiter %d\n"),two,2);
						/*simulate a random amount of time to actually eat the food*/
						for(i=0; i<(Rand()%13)+12;i += 1){
							Yield();
						}

						/*let the waiter know i'm done eating*/
						Signal(waiterGetFood[myTicket.waiter], waiterGetFoodLock[myTicket.waiter]);						
						/*wait for the waiter to give me the bill*/
						Wait(waiterGetFood[myTicket.waiter], waiterGetFoodLock[myTicket.waiter]);
						/*let the waiter see i'm done paying the bill*/
						Signal(waiterGetFood[myTicket.waiter], waiterGetFoodLock[myTicket.waiter]);
						myTicket.hasEaten = true;

						Release(waiterGetFoodLock[myTicket.waiter]);/*end passenger/waiter*/
					}
				}
			}
		}

		if(!myTicket.hasSlept){
			if(myTicket.hasBedding){
				/*Sleep*/
				for( i = 0; i < 5; i += 1){
					Yield();
				}
				myTicket.hasSlept = true;
			} 
			else {
				/*Randomly decide to order bedding*/
				if(Rand()%4 == 0){
					/*printf("Passenger %d calls for bedding\n", myID);*/
					/*Do porter stuff*/
					/*Check for any available porters first*/
					for(i = 0; i < PORTERS; i += 1){
						Acquire(porterLock[i]);
						if(porterAvailable[i]){
							porterAvailable[i] = false;
							/*passengerGotInLine = false;*/
							myPorter= i;
							/*my porter was waiting for work, so i come up and signal him*/
							Signal(porter[myPorter], porterLock[myPorter]);
							break;
						}
						else{
							Release(porterLock[i]);
						}
					}
					/*No Porter was available, so I have to get in line*/
					if(myPorter == -1 && (!isFirstClass)){
						myPorter = 0;
						Acquire(porterLock[0]);
						shortestLength = porterBeddingLineLength[0];
						for(i = 1; i < PORTERS; i += 1){
							Acquire(porterLock[i]);
							if(porterBeddingLineLength[i] < shortestLength){
								shortestLength = porterBeddingLineLength[i];
								Release(porterLock[myPorter]);
								myPorter = i;
							}
							else{
								Release(porterLock[i]);
							}				
						}

						Signal(porter[myPorter], porterLock[myPorter]); 
					}
					else if(myPorter==-1 && isFirstClass){
						myPorter = 0;
						Acquire(porterLock[0]);
						shortestLength = porterFirstClassBeddingLineLength[0];
						for(i = 1; i < PORTERS; i += 1){
							Acquire(porterLock[i]);
							if(porterFirstClassBeddingLineLength[i] < shortestLength){
								shortestLength = porterFirstClassBeddingLineLength[i];
								Release(porterLock[myPorter]);
								myPorter = i;
							}
							else{
								Release(porterLock[i]);
							}				
						}

						Signal(porter[myPorter], porterLock[myPorter]); /*QUESTIONABLE*/

					}
					Acquire(porterBeddingLineLock[myPorter]);
					if(isFirstClass){
						porterFirstClassBeddingLineLength[myPorter] += 1;
					}
					else{
						porterBeddingLineLength[myPorter] += 1;
					}
					Release(porterLock[myPorter]);
					
					/*printf("passenger%d waiting in porter%d\'s bedding line\n", myID, myPorter);*/
					one[0] = myID;
					Print("Passenger %d calls for bedding\n",sizeof("Passenger %d calls for bedding\n"),one,1);
					isFirstClass?
						Wait(porterFirstClassBeddingLine[myPorter], porterBeddingLineLock[myPorter]):
						Wait(porterBeddingLine[myPorter], porterBeddingLineLock[myPorter]);
				
					/*show the porter my name*/
					passengerBedding[myPorter]=&myTicket;
					Signal(porterLine[myPorter], porterBeddingLineLock[myPorter] );
					Wait(porterLine[myPorter], porterBeddingLineLock[myPorter] );
					/*printf("passenger%d got bedding from porter%d\n", myID, myPorter);*/
					Release(porterBeddingLineLock[myPorter]);
				}
			}	
		}
	}
	/*I got on, sat down, ate and slept, so now it's time to see if i should get off*/
	while(!disembarked){
		Acquire(atAStopLock);
		/*if(nowStopped){*/
			/*printf("%d, %d\n",currentStop,myTicket.getOffStop);*/
			if(TEST_5){
				myTicket.getOffStop=0;
				/*passengers always get off at a bad stop*/
			}
			if(currentStop==myTicket.getOffStop){
				Acquire(passengerCountLock);
				passengersOnTrain -= 1;
				/*printf("a passenger got themselves off got down\n");*/
				two[0] = myID;
				two[1] = currentStop;
				Print("Passenger %d of Train 0 is getting off the Train at stop number %d\n",sizeof("Passenger %d of Train 0 is getting off the Train at stop number %d\n"),two,2);
				Release(passengerCountLock);
				Release(atAStopLock);
				disembarked=true;
			}
			else if(currentStop>myTicket.getOffStop){
				/*i am getting off late*/
				Acquire(passengerCountLock);
				passengersOnTrain -= 1;
				/*printf("a passenger got off late... most disappointing\n");*/
				/*I pay a fine for getting of late- DOH*/
				Acquire(ticketRevenueLock);
				if(TEST_5){
					one[0] = myID;
					Print("Passenger %d of Train 0 finds that they waited too long to get off\n and are paying a fine of 2 bucks\n",sizeof("Passenger %d of Train 0 finds that they waited too long to get off\n and are paying a fine of 2 bucks\n"),one,1);
				}
				ticketRevenue+=2;
				Release(ticketRevenueLock);
				two[0] = myID;
				two[1] = currentStop;
				Print("Passenger %d of Train 0 is getting off the Train at stop number %d\n",sizeof("Passenger %d of Train 0 is getting off the Train at stop number %d\n"),two,2);
				Release(passengerCountLock);
				Release(atAStopLock);
				disembarked=true;
			}
			else{
				Release(atAStopLock);
				/*wait for OUR stop- conductor will inform us to wakeup at the next stop*/
				Acquire(getOffAtThisStopLock);
				Wait(getOffAtThisStop, getOffAtThisStopLock);
				Release(getOffAtThisStopLock);
			}
	}
		/*}
		else{
			atAStopLock->Release();
			currentThread->Yield();
		}*/
	
/*
		else {			
			//I'm either not at a stop, or the stop is less than the one I want
			getOffAtThisStopLock->Acquire();
			do{
				atAStopLock->Release();
				getOffAtThisStop->Wait(getOffAtThisStopLock);
				atAStopLock->Acquire();
			}while(currentStop<myTicket.getOffStop);
			getOffAtThisStopLock->Release();
			passengerCountLock->Acquire();
			passengersOnTrain -= 1;
			if(currentStop==myTicket.getOffStop){
				printf("a passenger got themselves off// got down\n");
				passengerCountLock->Release();
				atAStopLock->Release();
			}
			else{
				printf("a passenger got off late... most disappointing\n");
				passengerCountLock->Release();
				atAStopLock->Release();
			}
		}
*/	
	
	/*printf("PASSENGER%d finished!\n", myID);*/
	passengersFinished += 1;
	if(myTicket.hasEaten)
		passengersEaten += 1;
	if(myTicket.hasSlept)
		passengersSlept += 1;
	Exit(0);
}/*end of passenger */

void Chef(){
	unsigned int myID;
	int i = 0;
	
	int one[1];
	int two[2];
	int three[3];
	int four[4];
	
	Acquire(IDLock);
	myID = chefID;
	chefID += 1;
	Release(IDLock);
	
	/*printf("chef%d is alive\n",myID);*/
	
	while(true){
		Acquire(chefLock[myID]);
		if(chefFirstClassOrdersLeft[myID]!=0){
			/*do a first class order*/
			Acquire(chefOrdersLineLock[myID]);
			Release(chefLock[myID]);
			/*yield to simulate cooking their order*/
			one[0] = myID;
			Print("Chef %d of Train 0 is preparing food\n",sizeof("Chef %d of Train 0 is preparing food\n"),one,1);
			for(i=0; i<(Rand()%13)+12; i += 1){
				Yield();
			}
			chefFirstClassOrdersLeft[myID] -= 1;
			/*printf("chef%d cooked the order and now has %d left in his 1st class\n",myID,chefFirstClassOrdersLeft[myID]);*/
			Signal(chefFirstClassOrdersLine[myID], chefOrdersLineLock[myID]);
			Release(chefOrdersLineLock[myID]);

			
		}
		else if(chefRegularOrdersLeft[myID]!=0){
			/*do a regular order*/
			Acquire(chefOrdersLineLock[myID]);
			Release(chefLock[myID]);
			one[0] = myID;
			Print("Chef %d of Train 0 is preparing food\n",sizeof("Chef %d of Train 0 is preparing food\n"),one,1);
			for(i=0; i<(Rand()%13)+12; i += 1){
				Yield();
			}
			chefRegularOrdersLeft[myID] -= 1;
			/*printf("chef%d cooked the order and now has %d left in his reg class\n",myID,chefFirstClassOrdersLeft[myID]);*/
			Signal(chefRegularOrdersLine[myID], chefOrdersLineLock[myID]);
			Release(chefOrdersLineLock[myID]);
		}
		else{	
			/*check food stock levels*/
			Acquire(globalFoodLock);
			for(i=0;i<4;i += 1){
				if(foodStock[i]<= MIN_STOCK){
					one[0] = myID;
					Print("Chef %d of Train 0 calls for more inventory\n",sizeof("Chef %d of Train 0 calls for more inventory\n"),one,1);
					foodStockLow[i]=true;
				}
			}
			Release(globalFoodLock);
			/*go on break*/
			/*printf("chef%d has nothing requiring his immediate attention\n", myID);*/
			chefOnBreak[myID]=true;
			one[0] = myID;
			Print("Chef %d of Train 0 is going for a break\n",sizeof("Chef %d of Train 0 is going for a break\n"),one,1);
			Print("Chef %d of Train 0 returned from break\n",sizeof("Chef %d of Train 0 returned from break\n"),one,1);
			Wait(chef[myID], chefLock[myID]);
		}
	}
	Exit(0);
}/*--end of CHEF*/

void Porter(){
	unsigned int myID;
	/*printf("porter %d is alive\n", myID);*/
	
	int one[1];
	int two[2];
	int three[3];
	int four[4];
	
	Acquire(IDLock);
	myID = porterID;
	porterID += 1;
	Release(IDLock);

	while(true){
		Acquire(porterLock[myID]);
		if(porterBaggageLineLength[myID]!=0){
			Acquire(porterBaggageLineLock[myID]);
			/*printf("porter%d has %d people in baggage line and is signalling the first to come up\n", myID, porterBaggageLineLength[myID]);*/
			/*I process a passenger*/
			Signal(porterBaggageLine[myID], porterBaggageLineLock[myID]);
			/*wait for passenger to tell me their name :D*/
			Wait(porterLine[myID], porterBaggageLineLock[myID]);
			two[0] = myID;
			two[1] = passengerBaggage[myID]->holder;
			Print("Porter %d of Train 0 picks up bags of 1st class Passenger %d\n",sizeof("Porter %d of Train 0 picks up bags of 1st class Passenger %d\n"),two,2);
			porterBaggageLineLength[myID] -= 1;
			Release(porterBaggageLineLock[myID]);
		}
		else if(porterFirstClassBeddingLineLength[myID]!=0){
			Acquire(porterBeddingLineLock[myID]);
			/*printf("porter%d has %d people in 1st class bedding line and is signalling the first to come up\n", myID, porterBeddingLineLength[myID]);*/
			/*I process a passenger*/
			Signal(porterFirstClassBeddingLine[myID], porterBeddingLineLock[myID]);
			/*wait for passenger to tell me their name C|:-D*/
			Wait(porterLine[myID], porterBeddingLineLock[myID]);
			two[0] = myID;
			two[1] = passengerBedding[myID]->holder;
			Print("Porter %d of Train 0 gives bedding to Passenger %d\n",sizeof("Porter %d of Train 0 gives bedding to Passenger %d\n"),two,2);
			porterFirstClassBeddingLineLength[myID] -= 1;
			passengerBedding[myID]->hasBedding = true;
			Signal(porterLine[myID], porterBeddingLineLock[myID] );
			Release(porterBeddingLineLock[myID]);
		}
		else if(porterBeddingLineLength[myID]!=0){
			Acquire(porterBeddingLineLock[myID]);
			/*printf("porter%d has %d people in bedding line and is signalling the first to come up\n", myID, porterBeddingLineLength[myID]);*/
			/*I process a passenger*/
			Signal(porterBeddingLine[myID], porterBeddingLineLock[myID]);
			/*wait for passenger to tell me their name C|:-D*/
			Wait(porterLine[myID], porterBeddingLineLock[myID]);
			two[0] = myID;
			two[1] = passengerBedding[myID]->holder;
			Print("Porter %d of Train 0 gives bedding to Passenger %d\n",sizeof("Porter %d of Train 0 gives bedding to Passenger %d\n"),two,2);
			porterBeddingLineLength[myID] -= 1;
			passengerBedding[myID]->hasBedding = true;
			Signal( porterLine[myID], porterBeddingLineLock[myID] );
			Release(porterBeddingLineLock[myID]);
		}
		else{
			/*printf("porter%d has nothing requiring his immediate attention\n", myID);*/
			porterAvailable[myID] = true;
			Wait(porter[myID], porterLock[myID]);
		}
	}
	Exit(0);
}/*end of porter*/
		
	

void CoachAttendant(){
	unsigned int myID;
	int myChef=-1;
	int i = 0;
	int shortestLength;
	
	int one[1];
	int two[2];
	int three[3];
	int four[4];
	
	Acquire(IDLock);
	myID = coachAttendantID;
	coachAttendantID += 1;
	Release(IDLock);

	/*printf("coach attendant%d is alive\n", myID);*/

	while(true){
		myChef=-1;
		Acquire(coachAttendantLock[myID]);
		/*printf("coachAttendant%d acquired a lock on themselves\n", myID);*/
		if(coachAttendantFirstLineLength[myID]!=0){
			Acquire(coachAttendantLineLock[myID]);
			/*printf("coachAttendant%d has %d people in FC line\n and is signalling the first to come up\n", myID, coachAttendantFirstLineLength[myID]);*/
			/*I process a fc. passenger in line*/
			Signal(coachAttendantFirstLine[myID], coachAttendantLineLock[myID]);
			/*wait for a passenger to hand over their ticket*/
			Wait(coachAttendantLine[myID], coachAttendantLineLock[myID]);
			Acquire(getSeatLock[myID]);
			Release(coachAttendantLineLock[myID]);

			/*find a seat for this passenger*/
			Acquire(globalSeatLock);
			/*side effect---- seat is incremented*/
			passengerTickets[myID]->seatNumber=nextSeat += 1;
			Release(globalSeatLock);
			/*I wake up the passenger and wait for them to have a porter*/
			Signal(getSeat[myID], getSeatLock[myID]);
			Acquire(CAWaitingForPorterLock[myID]);
			Release(getSeatLock[myID]);
			Wait(CAWaitingForPorter[myID], CAWaitingForPorterLock[myID]);
			/*passenger has signalled me that they have a porter and I can seat them now!*/
			three[0] = myID;
			three[1] = passengerTickets[myID]->seatNumber;
			three[2] = passengerTickets[myID]->holder;
			Print("Coach Attendant %d of Train 0 gives seat number %d to Passenger %d\n",sizeof("Coach Attendant %d of Train 0 gives seat number %d to Passenger %d\n"),three,3);
			coachAttendantFirstLineLength[myID] -= 1;
			Release(CAWaitingForPorterLock[myID]);
			Acquire(boardingPassengersCountLock);
			passengersBoardedAtThisStop[currentStop] += 1;
			Release(boardingPassengersCountLock);
			if(passengersBoardedAtThisStop[currentStop]==passengersWaitingToBoard[currentStop]){
				Acquire(waitingForBoardingCompletionLock);
				/*printf("CA%d finds that all passengers have boarded for this stop and is telling conductor\n",myID);*/
				Signal(waitingForBoardingCompletion, waitingForBoardingCompletionLock);
				Release(waitingForBoardingCompletionLock);
			}
		}
		else if(coachAttendantRegularLineLength[myID]!=0){
			Acquire(coachAttendantLineLock[myID]);
			/*printf("coachAttendant%d has %d people in reg line\n and is signalling the first to come up\n", myID, coachAttendantRegularLineLength[myID]);*/
			/*I process a reg. passenger in line*/
			Signal(coachAttendantRegularLine[myID], coachAttendantLineLock[myID]);
			/*wait for a passenger to hand over their ticket*/
			Wait(coachAttendantLine[myID], coachAttendantLineLock[myID]);
			Acquire(getSeatLock[myID]);
			Release(coachAttendantLineLock[myID]);
			/*find a seat for this passenger*/
			Acquire(globalSeatLock);
			passengerTickets[myID]->seatNumber=nextSeat;
			/*side effect seat is incremented after print*/
			three[0] = myID;
			three[1] = nextSeat;
			three[2] = passengerTickets[myID]->holder;
			Print("Coach Attendant %d of Train 0 gives seat number %d to Passenger %d\n",sizeof("Coach Attendant %d of Train 0 gives seat number %d to Passenger %d\n"),three,3);
			nextSeat += 1;
			/*/decrement my line length*/
			coachAttendantRegularLineLength[myID] -= 1;
			Release(globalSeatLock);
			Signal(getSeat[myID], getSeatLock[myID]);
			Release(getSeatLock[myID]);
			Acquire(boardingPassengersCountLock);
			passengersBoardedAtThisStop[currentStop] += 1;
			Release(boardingPassengersCountLock);
			if(passengersBoardedAtThisStop[currentStop]==passengersWaitingToBoard[currentStop]){
				Acquire(waitingForBoardingCompletionLock);
				/*printf("CA%d finds that all passengers have boarded for this stop and is telling conductor\n",myID);*/
				Signal(waitingForBoardingCompletion, waitingForBoardingCompletionLock);
				Release(waitingForBoardingCompletionLock);
			}
				
		}
		else if(coachAttendantFoodLineLength[myID] != 0){
			/*do food orders*/
			Acquire(coachAttendantLineLock[myID]);
			/*printf("coachAttendant%d has %d people in food line\n and is signalling the first to come up\n", myID, coachAttendantFoodLineLength[myID]);*/
			Signal(coachAttendantFoodLine[myID], coachAttendantLineLock[myID]);

			Acquire(coachAttendantGetFoodLock[myID]);
			Release(coachAttendantLineLock[myID]);

			/*validate food orders until what they want is in stock*/
			do{
				Wait(coachAttendantGetFood[myID], coachAttendantGetFoodLock[myID]);
				Acquire(globalFoodLock);

				if(foodStock[coachAttendantOrder[myID]->foodOrder] == 0){
					coachAttendantOrder[myID]->validFoodOrder = false;
				}
				else {
					coachAttendantOrder[myID]->validFoodOrder = true;
					foodStock[coachAttendantOrder[myID]->foodOrder] -= 1;
				}
				Release(globalFoodLock);

				Signal(coachAttendantGetFood[myID], coachAttendantGetFoodLock[myID]);
			} while (!coachAttendantOrder[myID]->validFoodOrder);
			two[0] = myID;
			two[1] = coachAttendantOrder[myID]->holder;
			Print("Coach Attendant %d of Train 0 takes food order of 1st class Passenger %d\n",sizeof("Coach Attendant %d of Train 0 takes food order of 1st class Passenger %d\n"),two,2);
			for(i = 0; i < CHEFS; i += 1){
				Acquire(chefLock[i]);
				if(chefOnBreak[i]){
					chefOnBreak[i] = false;
					myChef= i;
					/*my chef was waiting for work, so i come up and signal him*/
					Signal(chef[myChef], chefLock[myChef]);
					break;
				}
				else{
					Release(chefLock[i]);
				}
			}
			/*No chef was available, so I have to get in line*/
			if(myChef==-1){
				myChef = 0;
				Acquire(chefLock[0]);
				shortestLength = chefFirstClassOrdersLeft[0];
				for(i = 1; i < CHEFS; i += 1){
					Acquire(chefLock[i]);
					if(chefFirstClassOrdersLeft[i] < shortestLength){
						shortestLength = chefFirstClassOrdersLeft[i];
						Release(chefLock[myChef]);
						myChef = i;
					}
					else{
						Release(chefLock[i]);
					}				
				}
				Signal(chef[myChef], chefLock[myChef]); /*QUESTIONABLE*/
			}
			/*now we definitely have a chef that will eventually want to see our order*/
			/*printf("CA%d acquires chef%d's lock\n",myID,myChef);*/
			Acquire(chefOrdersLineLock[myChef]);
			chefFirstClassOrdersLeft[myChef] += 1;
			Release(chefLock[myChef]);
			/*wait for the chef to cook my food*/
			/*printf("CA%d is waiting on chef%d to cook\n",myID,myChef);*/
			Wait(chefFirstClassOrdersLine[myChef], chefOrdersLineLock[myChef]);
			Release(chefOrdersLineLock[myChef]);
			three[0] = myID;
			three[1] = myChef;
			three[2] = coachAttendantOrder[myID]->holder;
			Print("Coach Attendant %d of Train 0 takes food prepared by Chef %d to the 1st class Passenger %d\n", sizeof("Coach Attendant %d of Train 0 takes food prepared by Chef %d to the 1st class Passenger %d\n"),three,3);
			/*printf("CA%d has delivered the food from the chef, and is waiting for a tip\n",myID);*/
			/*wait for your tip*/
			Wait(coachAttendantGetFood[myID], coachAttendantGetFoodLock[myID]);
			/*I got my tip and I'm done with this guy*/
			coachAttendantFoodLineLength[myID] -= 1;
			Release(coachAttendantGetFoodLock[myID]);			
		}
		else{
			/*printf("coachAttendant%d has nothing requiring his immediate attention\n", myID);*/
			coachAttendantAvailable[myID] = true;
			Wait(coachAttendant[myID], coachAttendantLock[myID]);
		}
		/*FOR NOW coachAttendantLock[myID]->Release();*/
	}
	Exit(0);
} /*end of CoachAttendant*/

void TicketChecker(){
        const unsigned int myID = 0;
        int i = 0;
        
        int one[1];
	int two[2];
	int three[3];
	int four[4];

                while(currentStop<MAX_STOPS){
                        Acquire(waitingForStopLock); /*TC will try to acquire a lock he owns, but that's OK*/
                        if(conductorWaiting){
                                Signal(waitingForStop, waitingForStopLock);
                       /* printf("the ticket checker is asleep, waiting for the next stop\n");*/
                                Wait(waitingForStop, waitingForStopLock);
                        }
                        else{
                                ticketCheckerWaiting=true;
                                Wait(waitingForStop, waitingForStopLock);
                        }
                        ticketCheckerWaiting=false;
                        /*FOR NOW DONT DO THIS waitingForStopLock->Release();*/


                        /*wakes up all passengers waiting at the current stop*/
                        Acquire(waitingForTrainLock[currentStop]);
                        Broadcast(waitingForTrain[currentStop], waitingForTrainLock[currentStop]);
                        /*printf("the ticket checker has told all the passengers it's time to board\n");*/
                        /*and acquires a lock on himself so that he can set his status and begin helping customers*/
                        Acquire(ticketCheckerLock);
                        Release(waitingForTrainLock[currentStop]);
                        /*by law, no one can be in line yet, so I am available*/
                        ticketCheckerAvailable=true;

                        for(i = 0; i < passengersWaitingToBoard[currentStop]; i += 1){
                                /*wait for passengers to come up and give me their ticket*/
                                /*printf("ticket checker is waiting for the next passenger\n");*/
                                Wait(ticketChecker, ticketCheckerLock);
                                /*printf("ticket checker is validating ticket for passenger%d \n", ticketToCheck->holder);*/
                                /*validate the ticket i have just received*/
                                Acquire(ticketLock);
if(! TEST_1 ) {
                                if(ticketToCheck->getOnStop==currentStop){
                                        ticketToCheck->okToBoard=true;
                                        /*increment passenger count?*/
                                        two[0] = ticketToCheck->holder;
                                        two[1] = currentStop;
					Print("Ticket Checker validates the ticket of passenger %d of Train 0 at stop number %d\n",sizeof("Ticket Checker validates the ticket of passenger %d of Train 0 at stop number %d\n"),two,2);
                                        Acquire(passengerCountLock);
                                        passengersOnTrain += 1;
                                        Release(passengerCountLock);
                                }
                                else {
                                        ticketToCheck->okToBoard=false;
                                        two[0] = ticketToCheck->holder;
                                        two[1] = currentStop;
					Print("Ticket Checker invalidates the ticket of passenger %d of Train 0 at stop number %d\n",sizeof("Ticket Checker invalidates the ticket of passenger %d of Train 0 at stop number %d\n"),two,2);
                                }
} else{
	ticketToCheck->okToBoard=false;
}                      
                                /*let the person know their ticket has been checked and that they*/
                                /*can move on*/
                                Signal(ticketCheck, ticketLock);
                                Release(ticketLock);                          

                                /*move to the next person in line, or wait for someone to come up*/
                                if(ticketCheckerLineLength>0){

                                        /*signal on ticketCheckerLine?*/
                                        Signal(ticketCheckerLine, ticketCheckerLock);
	
                                }
                                else {
                                        ticketCheckerAvailable=true;
                                }
                                /*two[0] = ticketCheckerLineLength;
                                two[1] = ticketCheckerAvailable;
                                Print("the ticket checker has %d passengers waiting, and has status %d \n",sizeof("the ticket checker has %d passengers waiting, and has status %d \n"),two,2);*/
                        }
                        /*necessary?*/
                        Release(ticketCheckerLock);
                        /*all passengers at this stop have been checked- inform conductor and wait for the next stop*/
                        Acquire(waitingForGoLock);
                        Signal(waitingForGo, waitingForGoLock);
                        Release(waitingForGoLock);
                }
	Exit(0);                  
}/*end of ticketChecker*/

void Conductor(){
        const unsigned int myID = 0;
        int i = 0;
        
        int one[1];
	int two[2];
	int three[3];
	int four[4];

        while(currentStop<MAX_STOPS-1){
                Acquire(waitingForStopLock);

                if(ticketCheckerWaiting){
                        /*conductor yields 25-50 times before he decides the next stop has arrived      */
                        for(i = 0; i<(randomRange(0,26)+25); i += 1){
                        	Yield();
                        }

                }
                else{
                        conductorWaiting=true;
                        Wait(waitingForStop, waitingForStopLock);
                        /*conductor yields 25-50 times before he decides the next stop has arrived      */
                        for(i = 0; i<(randomRange(0,26)+25); i += 1){
                        	Yield();
                        }
                        
                }
		Acquire(atAStopLock);
                currentStop += 1;
                one[0] = currentStop;
		Print("Conductor announces exit for Passengers at stop number %d\n",sizeof("Conductor announces exit for Passengers at stop number %d\n"),one,1);
		nowStopped=true;
		Release(atAStopLock);
                /*printf("WOO WOO, train has just pulled into stop %d\n", currentStop);*/

		/*tell people to check the current stop*/
                Broadcast(waitingForStop, waitingForStopLock);
                Acquire(waitingForGoLock);
                Release(waitingForStopLock);
             
		conductorWaiting=false;
                Wait(waitingForGo, waitingForGoLock);

                /*i have been cleared to go, so i do my bookeeping with a random chance and then move on*/
                /*print the number of passengers in the train*/
		Acquire(boardingPassengersCountLock);
                Release(waitingForGoLock);
		/*check to see if the kitchen requested more stock at this stop*/
		Acquire(globalFoodLock);
		for(i=0;i<4;i += 1){
			if(foodStockLow[i]){
				foodStock[i]+=RESTOCK_AMOUNT;
			}
		}
		Release(globalFoodLock);

                /*randomly check ticket value*/
                /*randomly check dining car safe value*/
		/*wait for coach attendant to let me know that everyone has gotten on...*/

		/*tell everyone waiting to get off that they should get off*/
		Acquire(getOffAtThisStopLock);
		Broadcast(getOffAtThisStop, getOffAtThisStopLock);
		Release(getOffAtThisStopLock);

		if(passengersBoardedAtThisStop[currentStop]!=passengersWaitingToBoard[currentStop] && !TEST_1){
			Acquire(waitingForBoardingCompletionLock);
			Release(boardingPassengersCountLock);
			/*printf("conductor has found that not everyone has boarded, so he's going to sleep\n");*/
			Wait(waitingForBoardingCompletion, waitingForBoardingCompletionLock);
			/*printf("Conductor finds that everyone has boarded on this stop, time to loop againXD\n");*/
			Release(waitingForBoardingCompletionLock);
		}
		else{
			/*printf("Conductor finds that everyone has boarded on this stop, time to loop againXD\n");*/
			Release(boardingPassengersCountLock);
		}
		/*randomly do bookeeping with a 1/3 chance */
		if(Rand()%3==0){
			Acquire(diningCarRevenueLock);
			Acquire(ticketRevenueLock);
			one[0] = diningCarRevenue;
			Print("Train 0 Dining Car revenue = %d\n",sizeof("Train 0 Dining Car revenue = %d\n"),one,1);
			one[0] = ticketRevenue;
			Print("Train 0 Ticket Revenue = %d\n",sizeof("Train 0 Ticket Revenue = %d\n"),one,1);
			totalRevenue= diningCarRevenue+ticketRevenue;
			one[0] = totalRevenue;
			Print("Total Train 0 Revenue = %d\n",sizeof("Total Train 0 Revenue = %d\n"),one,1);
			Release(diningCarRevenueLock);
			Release(ticketRevenueLock);
		}


		/*passengerCountLock->Acquire();*/
		one[0] = passengersOnTrain;
		Print("Passengers in the train 0 = %d\n",sizeof("Passengers in the train 0 = %d\n"),one,1);
		/*passengerCountLock->Release();*/
		/*I start the train up again*/
/*
		printf("conductor tries to acquire at a stop lock\n");
		if(currentStop!=MAX_STOPS-1){
			atAStopLock->Acquire();
			nowStopped=false;
			atAStopLock->Release();
			printf("conductor set atstop to false\n");
		}
*/

        }
	/*we've made it to the last stop*/
		/*but we aren't guaranteed that everyone has gotten off yet */
	/*if(passengersOnTrain != PASSENGERS)*/
		/*throwError(true, "Not all passengers on the train");*/
	while(passengersOnTrain!=0){
		Acquire(getOffAtThisStopLock);
		Broadcast(getOffAtThisStop, getOffAtThisStopLock);
		Release(getOffAtThisStopLock);
		for(i=0; i<30; i += 1){
			Yield(); /*give the lingering passengers a chance to finish eating/sleeping*/
		}
	}
	/*do a final set of prints to summarize*/
	
	Acquire(diningCarRevenueLock);
	Acquire(ticketRevenueLock);
	one[0] = diningCarRevenue;
	Print("Train 0 Dining Car revenue = %d\n",sizeof("Train 0 Dining Car revenue = %d\n"),one,1);
	one[0] = ticketRevenue;
	Print("Train 0 Ticket Revenue = %d\n",sizeof("Train 0 Ticket Revenue = %d\n"),one,1);
	totalRevenue= diningCarRevenue+ticketRevenue;
	one[0] = totalRevenue;
	Print("Total Train 0 Revenue = %d\n",sizeof("Total Train 0 Revenue = %d\n"),one,1);
	Release(diningCarRevenueLock);
	Release(ticketRevenueLock);
	Acquire(passengerCountLock);
	one[0] = passengersOnTrain;
	Print("Passengers in the train 0 = %d\n",sizeof("Passengers in the train 0 = %d\n"),one,1);
	Release(passengerCountLock);

	Exit(0);
}/*end of conductor*/

void Steward(){
	const unsigned int myID = 0;
	int i = 0;
	int min;
	bool foundAWaiter;
	
	int one[1];
	int two[2];
	int three[3];
	int four[4];
	
	while(true){
		/*randomly do bookeeping*/
		if(Rand()%5){
			/*look at waiter earnings, add them to dining car earnings, clear waiter earnings*/
			/*diningCarRevenueLock->Acquire();*/
			/*waiterRevenueLock->Acquire();*/
			diningCarRevenue+=waiterRevenue;
			waiterRevenue=0;
			one[0] = diningCarRevenue;
			Print("Steward gives the Dining Car revenue %d to the Conductor\n",sizeof("Steward gives the Dining Car revenue %d to the Conductor\n"),one,1);
			/*waiterRevenueLock->Release();*/
			/*diningCarRevenueLock->Release();*/
		}
		Acquire(stewardLock);
		if(stewardLineLength!=0){
			/*serve passenger */

			Signal(stewardLine, stewardLock);
			Wait(steward, stewardLock);
			/*passenger has set his ticket value*/
			Acquire(stewardGiveWaiterLock);
			Release(stewardLock);
			Acquire(diningCarCapacityLock);
			if(diningCarCapacity==0){
				/*printf("steward can't fit any more passengers into the dining car\n");*/
				one[0] = stewardTicketToCheck->holder;
				Print("Steward 0 of Train 0 informs Passenger %d-the dining car is full\n",sizeof("Steward 0 of Train 0 informs Passenger %d-the dining car is full\n"),one,1); 
				Release(diningCarCapacityLock);
				/*give them a null waiter so they can do nothing*/
				stewardTicketToCheck->waiter=-1;
				stewardLineLength -= 1;
				Signal(stewardGiveWaiter, stewardGiveWaiterLock);
				Release(stewardGiveWaiterLock);
			}/*steward is done with this passenger*/

			else{
				one[0] = stewardTicketToCheck->holder;
				Print("Steward 0 of Train 0 informs Passenger %d-the dining car is not full\n",sizeof("Steward 0 of Train 0 informs Passenger %d-the dining car is not full\n"),one,1); 
				foundAWaiter = false;
				/*There was space for them, so take away a seat*/
				diningCarCapacity -= 1;
				Release(diningCarCapacityLock);
				for(i=0; i<WAITERS;i += 1){
					/*look for waiters on break*/
					Acquire(waiterLock[i]);
					if(waiterOnBreak[i]==true){
						waiterOnBreak[i]=false;
						stewardTicketToCheck->waiter=i;
						Signal(waiter[i], waiterLock[i]);
						/*printf("steward is giving waiter%d to passenger%d\n",i,stewardTicketToCheck->holder);*/
						one[0] = i;
						Print("Steward calls Waiters %d from break\n",sizeof("Steward calls Waiters %d from break\n"),one,1);
						foundAWaiter = true;
						break;
					}
				}
				if(!foundAWaiter){
					/*no waiters were on break, so I pick the shortest line?*/
					Acquire(waiterLock[0]);
					min=waiterLineLength[0];
					stewardTicketToCheck->waiter = 0;
					for(i=1; i<WAITERS;i += 1){
						Acquire(waiterLock[i]);
						if(waiterLineLength[i]<min){
							Release(waiterLock[stewardTicketToCheck->waiter]);
							min = waiterLineLength[i];
							stewardTicketToCheck->waiter=i;
						}
						else{
							Release(waiterLock[i]);
						}
					}
				}
				/*now we are guaranteed to have a waiter on our ticket*/
				Signal(stewardGiveWaiter, stewardGiveWaiterLock);
				Acquire(stewardLock);
				Release(stewardGiveWaiterLock);
				Wait(steward, stewardLock);
				Print("Steward didn't wake up a waiter...\n",sizeof("Steward didn't wake up a waiter...\n"),one,0);
				Release(waiterLock[stewardTicketToCheck->waiter]);	
				Wait(finalConfirmation, stewardLock);
				/*release steward lock?	*/		
			}	
		}
		else{
		/*steward always gives updated revenue when he has nothing to do- this way when*/
		/*the last passenger is done eating, the total revenue of the train should be accurate*/
			Acquire(diningCarRevenueLock);
			Acquire(waiterRevenueLock);
			diningCarRevenue+=waiterRevenue;
			waiterRevenue=0;
			one[0] = diningCarRevenue;
			Print("Steward gives the Dining Car revenue %d to the Conductor\n",sizeof("Steward gives the Dining Car revenue %d to the Conductor\n"),one,1);
			Release(waiterRevenueLock);
			Release(diningCarRevenueLock);

			/*wait for something to do*/
			stewardAvailable=true;
			Wait(steward, stewardLock);
		}
	}
	Exit(0);
}/*end of Steward*/

void Waiter(){
	unsigned int myID;
	int myChef;
	int i = 0;
	int shortestLength;
	
	int one[1];
	int two[2];
	int three[3];
	int four[4];
	
	Acquire(IDLock);
	myID = waiterID;
	waiterID += 1;
	Release(IDLock);
	
	while(true){
		myChef=-1;
		Acquire(waiterLock[myID]);
		Acquire(waiterLineLock[myID]);
		if(waiterLineLength[myID]!=0){
			Acquire(waiterGetFoodLock[myID]);
			Release(waiterLock[myID]);  /*maybe wrong??!*/
			Signal(waiterLine[myID], waiterLineLock[myID]);
			Release(waiterLineLock[myID]);
			/*process a waiting passenger*/
			do{
				Wait(waiterGetFood[myID], waiterGetFoodLock[myID]);
				Acquire(globalFoodLock);

				if(foodStock[passengerFood[myID]->foodOrder] == 0){
					passengerFood[myID]->validFoodOrder = false;
				}
				else {
					passengerFood[myID]->validFoodOrder = true;
					foodStock[passengerFood[myID]->foodOrder] -= 1;
				}
				Release(globalFoodLock);

				Signal(waiterGetFood[myID], waiterGetFoodLock[myID]);
			} while (!passengerFood[myID]->validFoodOrder);
			two[0] = myID;
			two[1] = passengerFood[myID]->holder;
			Print("Waiter %d is serving Passenger %d of Train 0\n",sizeof("Waiter %d is serving Passenger %d of Train 0\n"),two,2);

			for(i = 0; i < CHEFS; i += 1){
				Acquire(chefLock[i]);
				if(chefOnBreak[i]){
					chefOnBreak[i] = false;
					myChef= i;
					/*my chef was waiting for work, so i come up and signal him*/
					Signal(chef[myChef], chefLock[myChef]);
					break;
				}
				else{
					Release(chefLock[i]);
				}
			}
			/*No Porter was available, so I have to get in line*/
			if(myChef==-1){
				myChef = 0;
				Acquire(chefLock[0]);
				shortestLength = chefRegularOrdersLeft[0];
				for(i = 1; i < CHEFS; i += 1){
					Acquire(chefLock[i]);
					if(chefRegularOrdersLeft[i] < shortestLength){
						shortestLength = chefRegularOrdersLeft[i];
						Release(chefLock[myChef]);
						myChef = i;
					}
					else{
						Release(chefLock[i]);
					}				
				}
				Signal(chef[myChef], chefLock[myChef]); /*QUESTIONABLE*/
			}
			/*now we definitely have a chef that will eventually want to see our order*/
			/*printf("waiter%d acquires chef%d's lock\n",myID,myChef);*/
			Acquire(chefOrdersLineLock[myChef]);
			chefRegularOrdersLeft[myChef] += 1;
			Release(chefLock[myChef]);
			/*wait for the chef to cook my food*/
			/*printf("waiter%d is waiting on chef%d to cook\n",myID,myChef);*/
			Wait(chefRegularOrdersLine[myChef], chefOrdersLineLock[myChef]);
			Release(chefOrdersLineLock[myChef]);
			/*one[0] = myID;*/
			/*Print("waiter%d has the order from the chef\n",sizeof("waiter%d has the order from the chef\n"),one,1);*/
			/*now I give the food to the passenger, and wait for him to eat,*/
			/*Print("waiter%d is waiting for the passenger to eat\n",sizeof("waiter%d is waiting for the passenger to eat\n"),one,1);*/
			
			/*waiterWaitingForEatingPassenger[myID]->Wait(waiterGetFoodLock[myID]);*/
			Wait(waiterGetFood[myID], waiterGetFoodLock[myID]);

			/*passenger is done eating and wating for a bill*/
			/*one[0] = myID;*/
			/*Print("waiter%d is giving passenger the bill\n",sizeof("waiter%d is giving passenger the bill\n"),one,1);*/

			/*passengerWaitingForBill[passengerFood[myID]->holder]->Signal(waiterGetFoodLock[myID]);*/
			/*wait for payment*/
			Signal(waiterGetFood[myID], waiterGetFoodLock[myID]);
			
			/*one[0] = myID;*/
			/*Print("waiter%d is waiting for passenger to pay\n",sizeof("waiter%d is waiting for passenger to pay\n"),one,1);*/

			/*waiterWaitingForMoney[myID]->Wait(waiterGetFoodLock[myID]);*/
			Wait(waiterGetFood[myID], waiterGetFoodLock[myID]);

			/*printf("waiter%d was paid by passenger%d!\n",myID, passengerFood[myID]->holder);*/
		
			/*update steward about the dining car safe*/
			Acquire(waiterRevenueLock);
			waiterRevenue+=2;
			Release(waiterRevenueLock);

			waiterLineLength[myID] -= 1;
			Acquire(diningCarCapacityLock);
			diningCarCapacity += 1;  
			Release(diningCarCapacityLock);
			Release(waiterGetFoodLock[myID]);
		}
		else{
			one[0] = myID;
			Print("Waiter %d of Train 0 is going for a break\n",sizeof("Waiter %d of Train 0 is going for a break\n"),one,1);
			waiterOnBreak[myID]=true;
			Release(waiterLineLock[myID]);
			Wait(waiter[myID], waiterLock[myID]);
			one[0] = myID;
			Print("Waiter %d of Train 0 returned from break\n",sizeof("Waiter %d of Train 0 returned from break\n"),one,1);
		}
	}
	Exit(0);
}/*end of Waiter*/


void main(){
	int i = 0;
	char * name;
	int one[1];

	IDLock = CreateLock("IDLock");
	atAStopLock = CreateLock("atAStopLock");
	getOffAtThisStopLock = CreateLock("getOffAtThisStopLock");
	getOffAtThisStop = CreateCondition("getOffAtThisStopCV");
	ticketRevenueLock = CreateLock("ticketRevenueLock");
	diningCarRevenueLock = CreateLock("diningCarRevenueLock");
	waiterRevenueLock = CreateLock("waiterRevenueLock");
	ticketLock = CreateLock("ticketLock");
	ticketCheck = CreateCondition("ticketCheck");
	globalSeatLock = CreateLock("globalSeatLock");
	globalFoodLock = CreateLock("globalFoodLock");
	waitingForBoardingCompletionLock = CreateLock("waitingForBoardingCompletionLock");
	waitingForBoardingCompletion = CreateCondition("WaitingForBoardingCompletionCV");
	boardingPassengersCountLock = CreateLock("BoardingPassengersCountLock");
	stewardLock = CreateLock("stewardLock");
	steward = CreateCondition("stewardCV");
	diningCarCapacityLock = CreateLock("diningCarCapacityLock");
	stewardGiveWaiterLock = CreateLock("stewardGiveWaiterLock");
	stewardGiveWaiter = CreateCondition("stewardGiveWaiterCV");
	stewardLine = CreateCondition("stewardLineCV");
	finalConfirmation = CreateCondition("finalConfirmationCV");

	/*pre-simulation prints*/
	
	one[0] = COACH_ATTENDANTS;
	Print("Number of Coach Attendants per Train = %d\n",sizeof("Number of Coach Attendants per Train = %d\n"),one,1);
	Print("Number of Trains per Train = ??\n",sizeof("Number of Trains per Train = ??\n"),one,0);
	one[0] = CHEFS;
	Print("Number of Chefs per Train = %d\n",sizeof("Number of Chefs per Train = %d\n"),one,1);
	one[0] = WAITERS;
	Print("Number of Waiters per Train = %d\n",sizeof("Number of Waiters per Train = %d\n"),one,1);
	one[0] = PORTERS;
	Print("Number of Porters per Train = %d\n",sizeof("Number of Porters per Train = %d\n"),one,1);
	one[0] = PASSENGERS;
	Print("Total number of passengers = %d\n",sizeof("Total number of passengers = %d\n"),one,1);
	Print("Number of passengers for Train 0 = %d\n",sizeof("Number of passengers for Train 0 = %d\n"),one,1);



	/*setup bullhockey*/

	for(i=0; i<MAX_STOPS; i += 1){
	        name = "waitingForTrainCV  ";
        	name[17] = (char)(i/10+48);
        	name[18] = (char)(i%10+48);
	        waitingForTrain[i] = CreateCondition(name);
		name = "waitingForTrainLock  ";
        	name[19] = (char)(i/10+48);
        	name[20] = (char)(i%10+48);
	        waitingForTrainLock[i] = CreateLock(name);
	        passengersWaitingToBoard[i]=0;
		passengersBoardedAtThisStop[i]=0;
	}

	ticketChecker = CreateCondition("ticketChecker");
	ticketCheckerLine = CreateCondition("ticketCheckerLine");
	ticketCheckerLock = CreateLock("ticketCheckerLock");
	passengerCountLock = CreateLock("passengerCountLock");

	waitingForGo = CreateCondition("waitingForGoCV");
	waitingForGoLock = CreateLock("waitingForGoLock");
	waitingForStop = CreateCondition("waitingForStopCV");
	waitingForStopLock = CreateLock("waitingForStopLock");

	for(i=0; i<COACH_ATTENDANTS; i += 1){
		name = "coachAttendantCV  ";
        	name[16] = (char)(i/10+48);
        	name[17] = (char)(i%10+48);
		coachAttendant[i] = CreateCondition(name);
		name = "coachAttendantFirstLineCV  ";
        	name[25] = (char)(i/10+48);
        	name[26] = (char)(i%10+48);
		coachAttendantFirstLine[i] = CreateCondition(name);
		name = "coachAttendantRegularLineCV  ";
        	name[27] = (char)(i/10+48);
        	name[28] = (char)(i%10+48);
		coachAttendantRegularLine[i] = CreateCondition(name);
		name = "coachAttendantLineCV  ";
        	name[20] = (char)(i/10+48);
        	name[21] = (char)(i%10+48);
		coachAttendantLine[i] = CreateCondition(name);
		name = "coachAttendantLock  ";
        	name[18] = (char)(i/10+48);
        	name[19] = (char)(i%10+48);
		coachAttendantLock[i] = CreateLock(name);
		name = "coachAttendantLineLock  ";
        	name[22] = (char)(i/10+48);
        	name[23] = (char)(i%10+48);
		coachAttendantLineLock[i] = CreateLock(name);
		name = "getSeatLock  ";
        	name[11] = (char)(i/10+48);
        	name[12] = (char)(i%10+48);
		getSeatLock[i] = CreateLock(name);
		name = "getSeat  ";
        	name[7] = (char)(i/10+48);
        	name[8] = (char)(i%10+48);
		getSeat[i] = CreateCondition(name);
		name = "CAWaitingForPorterCV  ";
        	name[20] = (char)(i/10+48);
        	name[21] = (char)(i%10+48);
		CAWaitingForPorter[i] = CreateCondition(name);
		name = "CAWaitingForPorterLock  ";
        	name[22] = (char)(i/10+48);
        	name[23] = (char)(i%10+48);
		CAWaitingForPorterLock[i] = CreateLock(name);
		name = "coachAttendantGetFoodCV  ";
        	name[23] = (char)(i/10+48);
        	name[24] = (char)(i%10+48);
		coachAttendantGetFood[i] = CreateCondition(name);
		name = "coachAttendantGetFoodLock  ";
        	name[25] = (char)(i/10+48);
        	name[26] = (char)(i%10+48);
		coachAttendantGetFoodLock[i] = CreateLock(name);
		name = "coachAttendantFoodLineCV  ";
        	name[24] = (char)(i/10+48);
        	name[25] = (char)(i%10+48);
		coachAttendantFoodLine[i] = CreateCondition(name);

		coachAttendantFoodLineLength[i]=0;
		coachAttendantFirstLineLength[i]=0;
		coachAttendantRegularLineLength[i]=0;
		coachAttendantAvailable[i]=false;
		foodOrdersWaiting[i]=0;
	}

	for(i=0; i<PORTERS;i += 1){
		porterBaggageLineLength[i]=0;
		porterBeddingLineLength[i]=0;
		porterFirstClassBeddingLineLength[i]=0;
		porterAvailable[i]=0;

		name = "porterCV  ";
        	name[8] = (char)(i/10+48);
        	name[9] = (char)(i%10+48);
		porter[i] = CreateCondition(name);
		name = "porterBaggageLineCV  ";
        	name[19] = (char)(i/10+48);
        	name[20] = (char)(i%10+48);
		porterBaggageLine[i] = CreateCondition(name);
		name = "porterLineCV  ";
        	name[12] = (char)(i/10+48);
        	name[13] = (char)(i%10+48);
		porterLine[i] = CreateCondition(name);
		name = "porterLock  ";
        	name[10] = (char)(i/10+48);
        	name[11] = (char)(i%10+48);
		porterLock[i] = CreateLock(name);
		name = "porterBaggageLineLock  ";
        	name[21] = (char)(i/10+48);
        	name[22] = (char)(i%10+48);
		porterBaggageLineLock[i] = CreateLock(name);
		name = "porterBeddingLineCV  ";
        	name[19] = (char)(i/10+48);
        	name[20] = (char)(i%10+48);
		porterBeddingLine[i] = CreateCondition(name);
		name = "porterFirstClassBeddingLineCV  ";
        	name[29] = (char)(i/10+48);
        	name[30] = (char)(i%10+48);
		porterFirstClassBeddingLine[i] = CreateCondition(name);
		name = "porterBeddingLineLock  ";
        	name[21] = (char)(i/10+48);
        	name[22] = (char)(i%10+48);
		porterBeddingLineLock[i] = CreateLock(name);
	}


	for(i = 0; i < WAITERS; i += 1){
		name = "waiterLock  ";
        	name[10] = (char)(i/10+48);
        	name[11] = (char)(i%10+48);
		waiterLock[i] = CreateLock(name);
		name = "waiterCV  ";
        	name[8] = (char)(i/10+48);
        	name[9] = (char)(i%10+48);
		waiter[i] = CreateCondition(name);
		name = "waiterLineLock  ";
        	name[14] = (char)(i/10+48);
        	name[15] = (char)(i%10+48);
		waiterLineLock[i] = CreateLock(name);
		name = "waiterLineCV  ";
        	name[12] = (char)(i/10+48);
        	name[13] = (char)(i%10+48);
		waiterLine[i] = CreateCondition(name);
		name = "getFoodLock  ";
        	name[11] = (char)(i/10+48);
        	name[12] = (char)(i%10+48);
		waiterGetFoodLock[i] = CreateLock(name);
		name = "getFoodCV  ";
        	name[9] = (char)(i/10+48);
        	name[10] = (char)(i%10+48);
		waiterGetFood[i] = CreateCondition(name);

		waiterLineLength[i] = 0;
		waiterOnBreak[i] = false;
	}

	for(i=0; i<CHEFS;i += 1){
		chefOnBreak[i]=false;
		chefFirstClassOrdersLeft[i]=0;
		chefRegularOrdersLeft[i]=0;

		name = "chefLock  ";
        	name[8] = (char)(i/10+48);
        	name[9] = (char)(i%10+48);
		chefLock[i] = CreateLock(name);
		name = "chefCV  ";
        	name[6] = (char)(i/10+48);
        	name[7] = (char)(i%10+48);
		chef[i] = CreateCondition(name);
		name = "chefOrdersLineCV  ";
        	name[16] = (char)(i/10+48);
        	name[17] = (char)(i%10+48);
		chefRegularOrdersLine[i] = CreateCondition(name);
		name = "chefFirstClassOrdersLineCV  ";
        	name[26] = (char)(i/10+48);
        	name[27] = (char)(i%10+48);
		chefFirstClassOrdersLine[i] = CreateCondition(name);
		name = "chefOrdersLineLock  ";
        	name[18] = (char)(i/10+48);
        	name[19] = (char)(i%10+48);
 		chefOrdersLineLock[i] = CreateLock(name);

	}
	for(i=0; i<PASSENGERS; i += 1){
	        /*a more descriptive name is not given because each thread will have a myID var.*/
	        Fork(Passenger);
	}

	Fork(TicketChecker);

	Fork(Steward);
							if(!TEST_2){
	for(i=0; i<COACH_ATTENDANTS; i += 1){
		Fork(CoachAttendant);
	}
								}

	for(i=0; i<PORTERS; i += 1){
		Fork(Porter);
	}

	for(i = 0; i < WAITERS; i += 1){
		Fork(Waiter);
	}

	for(i = 0; i < CHEFS; i += 1){
		Fork(Chef);
	}

	Fork(Conductor);

}/*end simulation*/


