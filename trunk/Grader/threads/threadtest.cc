// threadtest.cc
//      Simple test case for the threads assignment.
//
//      Create two threads, and have them context switch
//      back and forth between themselves by calling Thread::Yield,
//      to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"
//added for rand purposes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

//----------------------------------------------------------------------
// SimpleThread
//      Loop 5 times, yielding the CPU to another ready thread
//      each iteration.
//
//      "which" is simply a number identifying the thread, for debugging
//      purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
   
    for (num = 0; num < 5; num++) {
        printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
//      Set up a ping-pong between two threads, by forking a thread
//      to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// TrainSIM
//      Setup and run a train simulation
//              -josh
//----------------------------------------------------------------------

       
//------------------
//INTERNAL STRUCTS      
//------------------

//a simple structure for holding a passenger's ticket info

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


	

//---------------------
// GLOBAL DATA
//---------------------
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
#define PASSENGERS 10
#define COACH_ATTENDANTS 3
#define PORTERS 3
#define WAITERS 3
#define CHEFS 2

#define MIN_STOCK 2
#define RESTOCK_AMOUNT 5

int totalRevenue=0;
int ticketRevenue=0;
int diningCarRevenue=0;
int waiterRevenue=0;

int currentStop = -1;
int passengersOnTrain = 0;

int foodStock[4] = {10,10,10,10};
bool foodStockLow[4] = {false,false,false,false};

bool nowStopped=false;
Lock* atAStopLock=new Lock("atAStopLock");
Lock* getOffAtThisStopLock=new Lock("getOffAtThisStopLock");
Condition* getOffAtThisStop=new Condition("getOffAtThisStopCV");

Lock* ticketRevenueLock=new Lock("ticketRevenueLock");
Lock* diningCarRevenueLock=new Lock("diningCarRevenueLock");
Lock* waiterRevenueLock=new Lock("waiterRevenueLock");
//no need for total revenue lock as only conductor will total these

// data for passenger/ticketChecker interactions
int passengersWaitingToBoard[MAX_STOPS];
int ticketCheckerLineLength = 0;
int ticketCheckerAvailable = false;
Condition* waitingForTrain[MAX_STOPS];//should this be MAX_STOPS-1?
Condition* ticketChecker; //TC waits on this for a passenger
Condition* ticketCheckerLine; //passengers wait on this when TC is unavailable
Lock* waitingForTrainLock[MAX_STOPS];//same issue?
Lock* ticketCheckerLock;
Ticket* ticketToCheck;
Lock* ticketLock=new Lock("ticketLock");
Condition* ticketCheck=new Condition("ticketCheck");
//--end passenger/ticketChecker data

//data for CoachAttendant/Passenger interactions
int coachAttendantRegularLineLength[COACH_ATTENDANTS];
int coachAttendantFirstLineLength[COACH_ATTENDANTS];
int coachAttendantAvailable[COACH_ATTENDANTS];
Condition* coachAttendant[COACH_ATTENDANTS];
Condition* coachAttendantRegularLine[COACH_ATTENDANTS];
Condition* coachAttendantFirstLine[COACH_ATTENDANTS];
Condition* coachAttendantLine[COACH_ATTENDANTS];
Lock* coachAttendantLock[COACH_ATTENDANTS];
Lock* coachAttendantLineLock[COACH_ATTENDANTS];
Lock* globalSeatLock=new Lock("globalSeatLock");
int nextSeat = 0;
Lock* getSeatLock[COACH_ATTENDANTS];
Condition* getSeat[COACH_ATTENDANTS];
Ticket* passengerTickets[COACH_ATTENDANTS];
int foodOrdersWaiting[COACH_ATTENDANTS];
Condition* CAWaitingForPorter[COACH_ATTENDANTS];
Lock* CAWaitingForPorterLock[COACH_ATTENDANTS];
int coachAttendantFoodLineLength[COACH_ATTENDANTS];
Condition* coachAttendantFoodLine[COACH_ATTENDANTS];
Condition* coachAttendantGetFood[COACH_ATTENDANTS];
Lock* coachAttendantGetFoodLock[COACH_ATTENDANTS];
Ticket* coachAttendantOrder[COACH_ATTENDANTS];
//--end CoachAttendant/Passenger interactions

//data for Porter/Passenger interactions
int porterBaggageLineLength[PORTERS];
int porterAvailable[PORTERS];
Condition* porter[PORTERS];
Condition* porterBaggageLine[PORTERS];
Condition* porterLine[PORTERS];
Lock* porterLock[PORTERS];
Lock* porterBaggageLineLock[PORTERS];
Ticket* passengerBaggage[PORTERS];
int porterBeddingLineLength[PORTERS];
int porterFirstClassBeddingLineLength[PORTERS];
Condition* porterFirstClassBeddingLine[PORTERS];
Condition* porterBeddingLine[PORTERS];
Lock* porterBeddingLineLock[PORTERS];
Ticket* passengerBedding[PORTERS];
//--end Porter/Passenger data

//data for Waiter/Passenger interactions
int waiterLineLength[WAITERS];
Ticket* passengerFood[WAITERS];
Lock* waiterLock[WAITERS];
Condition* waiter[WAITERS];
Lock* waiterLineLock[WAITERS];
Condition* waiterLine[WAITERS];
Lock* waiterGetFoodLock[WAITERS];
Condition* waiterGetFood[WAITERS];
Lock* globalFoodLock = new Lock("globalFoodLock");
//--end Waiter/Passenger data

//data for ticketChecker/Conductor interactions
Lock* passengerCountLock;
Lock* waitingForGoLock;
Lock* waitingForStopLock;
Condition* waitingForGo;
Condition* waitingForStop;
bool ticketCheckerWaiting =false;
bool conductorWaiting =false;
//--end ticketChecker/Conductor data

//data for Conductor/CoachAttendant interactions
Lock* waitingForBoardingCompletionLock=new Lock("waitingForBoardingCompletionLock");
Condition* waitingForBoardingCompletion=new Condition("WaitingForBoardingCompletionCV");
Lock* boardingPassengersCountLock=new Lock("BoardingPassengersCountLock");
int passengersBoardedAtThisStop[MAX_STOPS];
//--end conductor/CA interactions

//data for steward/passenger interactions
Lock* stewardLock = new Lock("stewardLock");
Condition* steward = new Condition("stewardCV");
int stewardLineLength = 0;
int diningCarCapacity = DINING_SEATS;
Lock* diningCarCapacityLock = new Lock("diningCarCapacityLock");
bool stewardAvailable = false;
Ticket* stewardTicketToCheck;
Lock* stewardGiveWaiterLock = new Lock("stewardGiveWaiterLock");
Condition* stewardGiveWaiter = new Condition("stewardGiveWaiterCV");
Condition* stewardLine = new Condition("stewardLineCV");
Condition* finalConfirmation = new Condition("finalConfirmationCV");
//--end steward/passenger data

//data for steward/waiter interactions
bool waiterOnBreak[WAITERS];
//--end steward/waiter data

//data for chef/ANYONE interactions
Lock* chefLock[CHEFS];
Lock* chefOrdersLineLock[CHEFS];
Condition* chef[CHEFS];
Condition* chefRegularOrdersLine[CHEFS];
Condition* chefFirstClassOrdersLine[CHEFS];
bool chefOnBreak[CHEFS];
int chefFirstClassOrdersLeft[CHEFS];
int chefRegularOrdersLeft[CHEFS];
//--end chef/ANYONE shared data

//HELPER FUNCTIONS
//-------------------

//a function to generate a random greater than the start value and less than the range value
int randomRange(unsigned int start, unsigned int range){
          /* Simple "srand()" seed: just use "time()" */
        //unsigned int iseed = (unsigned int)time(NULL);
        //srand (iseed);
        //reuse of iseed for brevity
        if(range<=start){
                return NULL;
        } else{
                unsigned int iseed = (rand()%range);
                //generate random numbers until we have one greater than our start
                        while(iseed<=start){
                                iseed = (rand()%range);
                        }
                return iseed;
        }
}

//We can use this function to detect abnormal simulation behavior
void throwError(bool X, char * message){
	puts(message);	
	int error = 0 / (X ? 0 : 1);
}

//----------------
// THREAD TYPES
//----------------

void Passenger(int number){
        const unsigned int myID = number;
	int myPorter=-1;
	int myCoachAttendant = -1; // initially no coach attendant associated
										// with this passenger
	//decide if passenger is first class or not
	bool isFirstClass = rand()%2;
	ticketRevenue+= 20 +(10*(int)isFirstClass);
        //passengers have to remember whether they simply walked up to the ticketChecker, or waited in line
        //this affects if they decrement the line count or not
        bool passengerGotInLine = NULL;
        //initialization of ticket structure
	bool disembarked =false;
        Ticket myTicket;
        //assumes passengers only get off at the last stop(for now)
        myTicket.getOnStop = (rand()%(MAX_STOPS-1));
        myTicket.getOffStop = randomRange(myTicket.getOnStop,MAX_STOPS);
        myTicket.holder=myID;//FOR DEBUGGING
       	myTicket.hasEaten = false;
	myTicket.hasSlept = false;
	myTicket.hasBedding = false;
	myTicket.foodOrder = -1;
	myTicket.validFoodOrder = false;
	myTicket.waiter = -1;
	printf("Passenger %d belongs to Train 0\n",myID);
	printf("Passenger %d is going to get on the Train at stop number %d\n",myID,myTicket.getOnStop);
	printf("Passenger %d is going to get off the Train at stop number %d\n",myID,myTicket.getOffStop);

        //passenger is now ready to go to a stop and wait to board the train
        waitingForTrainLock[myTicket.getOnStop]->Acquire();
        //so he increments the number of people at the stop and waits
        passengersWaitingToBoard[myTicket.getOnStop]++;
        //printf("passenger%d is in line at stop %d\n",myID, myTicket.getOnStop);
        waitingForTrain[myTicket.getOnStop]->Wait(waitingForTrainLock[myTicket.getOnStop]);
        waitingForTrainLock[myTicket.getOnStop]->Release();

        //and then checks on the ticketChecker
        ticketCheckerLock->Acquire();
        if(!ticketCheckerAvailable){
                //passenger gets in line
                passengerGotInLine=true;
                ticketCheckerLineLength++;
                ticketCheckerLine->Wait(ticketCheckerLock);
        }
        else {
                //ticket checker is free so passenger goes up to him
                ticketCheckerAvailable = false;
                passengerGotInLine=false;
        }
        //hand TC our ticket and wait to see what he says
        ticketLock->Acquire();
        ticketToCheck = &myTicket;
        ticketChecker->Signal(ticketCheckerLock);
        ticketCheckerLock->Release();
        //printf("Passenger%d has signaled and is now waiting on ticketChecker\n", myID);
        ticketCheck->Wait(ticketLock);
        //let the ticketChecker go on to do other things, and see if we should get on the train
        if(passengerGotInLine)
                ticketCheckerLineLength--;
        if(!myTicket.okToBoard){
                //i had a bad ticket
                //print msg and return
                //printf("passenger%d had a bad ticket and is leaving\n", myID);
		printf("Passenger %d of Train 0 has an invalid ticket\n",myID);
		ticketLock->Release();
                return;
        }
		printf("Passenger %d of Train 0 has a valid ticket\n",myID);
        //I can get on the train and wait for my next thing
        //printf("passenger%d got on the train\n", myID);
        //ticketCheckerLock->Release();
        ticketLock->Release();

	//Passenger is now on the train and checking on the CoachAttendants
	passengerGotInLine = NULL;  //Reusing this since it's not needed anymore for P/TC interaction
	
	//Check for any available CA's first
	for(int i = 0; i < COACH_ATTENDANTS; i++){
		coachAttendantLock[i]->Acquire();
		if(coachAttendantAvailable[i]){		
			coachAttendantAvailable[i] = false;
			//passengerGotInLine = false;
			myCoachAttendant= i;
			//my coach attendant was waiting for work, so i come up and signal him
			coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]);
			break;
		}
		else{
			coachAttendantLock[i]->Release();
		}
	}
	//If no available CA could be found, then get in the shortest line
	if(myCoachAttendant == -1){
		myCoachAttendant = 0;
		coachAttendantLock[0]->Acquire();
		int shortestLength = isFirstClass ? coachAttendantFirstLineLength[0] : coachAttendantRegularLineLength[0];
		
		for(int i = 1; i < COACH_ATTENDANTS; i++){
			coachAttendantLock[i]->Acquire();
			if(isFirstClass){
				if(coachAttendantFirstLineLength[i] < shortestLength){
					shortestLength = coachAttendantFirstLineLength[i];
					coachAttendantLock[myCoachAttendant]->Release();
					myCoachAttendant = i;
					
				}
				else{
					coachAttendantLock[i]->Release();
				}
			}
			else{
				if(coachAttendantRegularLineLength[i] < shortestLength){
					shortestLength = coachAttendantRegularLineLength[i];
					coachAttendantLock[myCoachAttendant]->Release();
					myCoachAttendant = i;
				}
				else{
					coachAttendantLock[i]->Release();
				}
			}
		}
		//signal our coach attendant
		//printf("passenger%d getting in coachAttendant%d\'s line\n", myID, myCoachAttendant);
		coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]); //QUESTIONABLE
	}
	
	coachAttendantLineLock[myCoachAttendant]->Acquire();
	//tell CA what kind of passenger you are
	isFirstClass ? 
		coachAttendantFirstLineLength[myCoachAttendant]++ :
		coachAttendantRegularLineLength[myCoachAttendant]++;
	
	coachAttendantLock[myCoachAttendant]->Release();
	
	//printf("passenger%d waiting in coachAttendant%d\'s line\n", myID, myCoachAttendant);
	isFirstClass ? 
		coachAttendantFirstLine[myCoachAttendant]->Wait( coachAttendantLineLock[myCoachAttendant] ) : 
		coachAttendantRegularLine[myCoachAttendant]->Wait( coachAttendantLineLock[myCoachAttendant] );
	//printf("passenger%d is moving up to coachAttendant%d\n", myID, myCoachAttendant);
	
	
	//Passenger is ready to get a seat now
	getSeatLock[myCoachAttendant]->Acquire();

	//now that coach attendant is ready for me i hand him my ticket so he knows my name
	passengerTickets[myCoachAttendant] = &myTicket;
	coachAttendantLine[myCoachAttendant]->Signal( coachAttendantLineLock[myCoachAttendant] );
	coachAttendantLineLock[myCoachAttendant]->Release();
	//printf("Passenger%d has signaled and is now waiting on coachAttendant%d\n", myID, myCoachAttendant);
	
	getSeat[myCoachAttendant]->Wait( getSeatLock[myCoachAttendant] );

	if(!isFirstClass)
		printf("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n", myID, myTicket.seatNumber, myCoachAttendant);

	if(isFirstClass)
		CAWaitingForPorterLock[myCoachAttendant]->Acquire();

	getSeatLock[myCoachAttendant]->Release();//end of regular class passenger/CA

	if(isFirstClass){
	//Do porter stuff
		//since it's a first class passenger, get a porter
		//Check for any available porters first
		for(int i = 0; i < PORTERS; i++){
			porterLock[i]->Acquire();
			if(porterAvailable[i]){			
				porterAvailable[i] = false;
				//passengerGotInLine = false;
				myPorter= i;
				//my porter was waiting for work, so i come up and signal him
				porter[myPorter]->Signal(porterLock[myPorter]);
				break;
			}
			else{
				porterLock[i]->Release();
			}
		}
		//No Porter was available, so I have to get in line
		if(myPorter == -1){
			myPorter = 0;
			porterLock[0]->Acquire();
			int shortestLength = porterBaggageLineLength[0];
		
			for(int i = 1; i < PORTERS; i++){
				porterLock[i]->Acquire();
				if(porterBaggageLineLength[i] < shortestLength){
					shortestLength = porterBaggageLineLength[i];
					porterLock[myPorter]->Release();
					myPorter = i;
					
				}
				else{
					porterLock[i]->Release();
				}				
			}

			//printf("passenger%d getting in porter%d\'s line\n", myID, myPorter);
			porter[myPorter]->Signal(porterLock[myPorter]); //QUESTIONABLE
		}
		porterBaggageLineLock[myPorter]->Acquire();
		porterBaggageLineLength[myPorter]++;
		porterLock[myPorter]->Release();
		
		//printf("passenger%d waiting in porter%d\'s line\n", myID, myPorter);
		porterBaggageLine[myPorter]->Wait(porterBaggageLineLock[myPorter]);
		//printf("passenger%d is giving bags up to porter%d\n", myID, myPorter);
	
		//show the porter my name
		passengerBaggage[myPorter]=&myTicket;
		porterLine[myPorter]->Signal( porterBaggageLineLock[myPorter] );
		porterBaggageLineLock[myPorter]->Release();

		CAWaitingForPorter[myCoachAttendant]->Signal(CAWaitingForPorterLock[myCoachAttendant]);
		CAWaitingForPorterLock[myCoachAttendant]->Release();
		printf("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n", myID, myTicket.seatNumber, myCoachAttendant);
		printf("1st class Passenger %d of Train 0 is served by Porter %d\n", myID, myPorter); 
				
	}
	//HAVE TO SET THESE BACK, due to bullhockey- they might be used later for ordering food or bedding
	myPorter=-1;
        myCoachAttendant=-1;

	while(!myTicket.hasEaten || !myTicket.hasSlept){
		if(!myTicket.hasEaten){
			//Randomly decide to get hungry
			if(rand()%4 == 0){
				//printf("passenger%d is now hungry\n",myID);
				//Acquire food somehow (depends on passenger class)
				if(isFirstClass){
					//Do CA/Passenger food interaction
					for(int i = 0; i < COACH_ATTENDANTS; i++){
						coachAttendantLock[i]->Acquire();
						if(coachAttendantAvailable[i]){			
							coachAttendantAvailable[i] = false;

							myCoachAttendant= i;
							//my CA was waiting for work, so i come up and signal him
							coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]);
							break;
						}
						else{
							coachAttendantLock[i]->Release();
						}
					}
					//No CA was available, so I have to get in line
					if(myCoachAttendant == -1){
						myCoachAttendant = 0;
						coachAttendantLock[0]->Acquire();
						int shortestLength = coachAttendantFoodLineLength[0];
					
						for(int i = 1; i < COACH_ATTENDANTS; i++){
							coachAttendantLock[i]->Acquire();
							if(coachAttendantFoodLineLength[i] < shortestLength){
								shortestLength = coachAttendantFoodLineLength[i];
								coachAttendantLock[myCoachAttendant]->Release();
								myCoachAttendant = i;
								
							}
							else{
								coachAttendantLock[i]->Release();
							}				
						}

						//printf("passenger%d getting in porter%d\'s line\n", myID, myPorter);
						coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]); //QUESTIONABLE
					}
					coachAttendantLineLock[myCoachAttendant]->Acquire();
					coachAttendantFoodLineLength[myCoachAttendant]++;
					coachAttendantLock[myCoachAttendant]->Release();
		
					//printf("passenger%d waiting in coachAttendant%d\'s food line\n", myID, myCoachAttendant);
					coachAttendantFoodLine[myCoachAttendant]->Wait(coachAttendantLineLock[myCoachAttendant]);
					//now I have my CA's attention, so I try to order
					coachAttendantGetFoodLock[myCoachAttendant]->Acquire();
					coachAttendantLineLock[myCoachAttendant]->Release();

					do{
						myTicket.foodOrder = rand()%4;
						coachAttendantOrder[myCoachAttendant] = &myTicket;
						coachAttendantGetFood[myCoachAttendant]->Signal( coachAttendantGetFoodLock[myCoachAttendant] );
						coachAttendantGetFood[myCoachAttendant]->Wait( coachAttendantGetFoodLock[myCoachAttendant] );
					} while (!myTicket.validFoodOrder);
					//i've been given the food, so I tip the CA
					coachAttendantGetFood[myCoachAttendant]->Signal(coachAttendantGetFoodLock[myCoachAttendant]);
					myTicket.hasEaten=true;
					coachAttendantGetFoodLock[myCoachAttendant]->Release();//end 1stclass/CA	
				}
				else {
					//Do Waiter/Passenger food interaction
					stewardLock->Acquire();
					if(stewardAvailable){
						stewardAvailable=false;
						steward->Signal(stewardLock);
					}
					stewardLineLength++;
					stewardLine->Wait(stewardLock); // now no matter what I have the steward's attention
					stewardGiveWaiterLock->Acquire(); // i'm not sure we need this lock
					stewardTicketToCheck=&myTicket;
					steward->Signal(stewardLock);
					stewardLock->Release();
					stewardGiveWaiter->Wait(stewardGiveWaiterLock);//definitely need this cv
					//I either have a waiter or not
					if(myTicket.waiter==-1){
						stewardGiveWaiterLock->Release();
						//printf("passenger%d couldn't get into the dining car\n",myID);
						//go wait around until you can get in :'[
						printf("Passenger %d of Train 0 is informed by Stewart-the Dining Car is full\n",myID);

					}
					else{
						printf("Passenger %d of Train 0 is informed by Stewart-the Dining Car is not full\n",myID);
						//i actually have a waiter
						//printf("passenger%d was put into the care of waiter%d\n",myID,myTicket.waiter);
						waiterLineLock[myTicket.waiter]->Acquire();
						stewardGiveWaiterLock->Release();
						waiterLineLength[myTicket.waiter]++;
						stewardLock->Acquire();
						steward->Signal(stewardLock);
						//stewardLineLength--;
						stewardLock->Release();//if i get context switched here.. i lose
						//two possibilities - one, i get to wait first, and things are fine...
						//two, waiter goes first, sees someone was in line, signals, but i'm not waiting
							//THIS IS NOW FIXED- steward waits for you to signal him that you have your waiter
						//printf("passenger%d dies here\n",myID);
						waiterLine[myTicket.waiter]->Wait(waiterLineLock[myTicket.waiter]);
						//printf("passenger%d didn't die there\n", myID);
						waiterGetFoodLock[myTicket.waiter]->Acquire();
						stewardLock->Acquire();
						stewardLineLength--;
						finalConfirmation->Signal(stewardLock);
						stewardLock->Release();//end passenger/steward
						//printf("passenger%d got the food lock\n",myID);
						waiterLineLock[myTicket.waiter]->Release();
						//i've been woken up so i give my order
						
						do{
							myTicket.foodOrder = rand()%4;
							passengerFood[myTicket.waiter] = &myTicket;
							waiterGetFood[myTicket.waiter]->Signal( waiterGetFoodLock[myTicket.waiter] );
							waiterGetFood[myTicket.waiter]->Wait( waiterGetFoodLock[myTicket.waiter] );
						} while (!myTicket.validFoodOrder);
						printf("Passenger %d of Train 0 is served by Waiter %d\n", myID, myTicket.waiter);
						//simulate a random amount of time to actually eat the food
						for(int i=0; i<(rand()%13)+12;i++){
							currentThread->Yield();
						}

						//let the waiter know i'm done eating
						waiterGetFood[myTicket.waiter]->Signal(waiterGetFoodLock[myTicket.waiter]);						
						//wait for the waiter to give me the bill
						waiterGetFood[myTicket.waiter]->Wait(waiterGetFoodLock[myTicket.waiter]);
						//let the waiter see i'm done paying the bill
						waiterGetFood[myTicket.waiter]->Signal(waiterGetFoodLock[myTicket.waiter]);
						myTicket.hasEaten = true;

						waiterGetFoodLock[myTicket.waiter]->Release();//end passenger/waiter
					}
				}
			}
		}

		if(!myTicket.hasSlept){
			if(myTicket.hasBedding){
				//Sleep
				for(int i = 0; i < 5; i++){
					currentThread->Yield();
				}
				myTicket.hasSlept = true;
			} 
			else {
				//Randomly decide to order bedding
				if(rand()%4 == 0){
					//printf("Passenger %d calls for bedding\n", myID);
					//Do porter stuff
					//Check for any available porters first
					for(int i = 0; i < PORTERS; i++){
						porterLock[i]->Acquire();
						if(porterAvailable[i]){
							porterAvailable[i] = false;
							//passengerGotInLine = false;
							myPorter= i;
							//my porter was waiting for work, so i come up and signal him
							porter[myPorter]->Signal(porterLock[myPorter]);
							break;
						}
						else{
							porterLock[i]->Release();
						}
					}
					//No Porter was available, so I have to get in line
					if(myPorter == -1 && (!isFirstClass)){
						myPorter = 0;
						porterLock[0]->Acquire();
						int shortestLength = porterBeddingLineLength[0];
						for(int i = 1; i < PORTERS; i++){
							porterLock[i]->Acquire();
							if(porterBeddingLineLength[i] < shortestLength){
								shortestLength = porterBeddingLineLength[i];
								porterLock[myPorter]->Release();
								myPorter = i;
							}
							else{
								porterLock[i]->Release();
							}				
						}

						porter[myPorter]->Signal(porterLock[myPorter]); 
					}
					else if(myPorter==-1 && isFirstClass){
						myPorter = 0;
						porterLock[0]->Acquire();
						int shortestLength = porterFirstClassBeddingLineLength[0];
						for(int i = 1; i < PORTERS; i++){
							porterLock[i]->Acquire();
							if(porterFirstClassBeddingLineLength[i] < shortestLength){
								shortestLength = porterFirstClassBeddingLineLength[i];
								porterLock[myPorter]->Release();
								myPorter = i;
							}
							else{
								porterLock[i]->Release();
							}				
						}

						porter[myPorter]->Signal(porterLock[myPorter]); //QUESTIONABLE

					}
					porterBeddingLineLock[myPorter]->Acquire();
					isFirstClass ?
						porterFirstClassBeddingLineLength[myPorter]++:
						porterBeddingLineLength[myPorter]++;
					porterLock[myPorter]->Release();
					
					//printf("passenger%d waiting in porter%d\'s bedding line\n", myID, myPorter);
					printf("Passenger %d calls for bedding\n",myID);
					isFirstClass?
						porterFirstClassBeddingLine[myPorter]->Wait(porterBeddingLineLock[myPorter]):
						porterBeddingLine[myPorter]->Wait(porterBeddingLineLock[myPorter]);
				
					//show the porter my name
					passengerBedding[myPorter]=&myTicket;
					porterLine[myPorter]->Signal( porterBeddingLineLock[myPorter] );
					porterLine[myPorter]->Wait( porterBeddingLineLock[myPorter] );
					//printf("passenger%d got bedding from porter%d\n", myID, myPorter);
					porterBeddingLineLock[myPorter]->Release();
				}
			}	
		}
	}
	//I got on, sat down, ate and slept, so now it's time to see if i should get off
	while(!disembarked){
		atAStopLock->Acquire();
		//if(nowStopped){
			//printf("%d, %d\n",currentStop,myTicket.getOffStop);
			if(TEST_5){
				myTicket.getOffStop=0;
				//passengers always get off at a bad stop
			}
			if(currentStop==myTicket.getOffStop){
				passengerCountLock->Acquire();
				passengersOnTrain--;
				//printf("a passenger got themselves off// got down\n");
				printf("Passenger %d of Train 0 is getting off the Train at stop number %d\n",myID,currentStop);
				passengerCountLock->Release();
				atAStopLock->Release();
				disembarked=true;
			}
			else if(currentStop>myTicket.getOffStop){
				//i am getting off late
				passengerCountLock->Acquire();
				passengersOnTrain--;
				//printf("a passenger got off late... most disappointing\n");
				//I pay a fine for getting of late- DOH
				ticketRevenueLock->Acquire();
				if(TEST_5)
				printf("Passenger %d of Train 0 finds that they waited too long to get off\n and are paying a fine of 2 bucks\n",myID);
				ticketRevenue+=2;
				ticketRevenueLock->Release();
				printf("Passenger %d of Train 0 is getting off the Train at stop number %d\n",myID,currentStop);
				passengerCountLock->Release();
				atAStopLock->Release();
				disembarked=true;
			}
			else{
				atAStopLock->Release();
				//wait for OUR stop- conductor will inform us to wakeup at the next stop
				getOffAtThisStopLock->Acquire();
				getOffAtThisStop->Wait(getOffAtThisStopLock);
				getOffAtThisStopLock->Release();
			}
	}
		//}
		//else{
		//	atAStopLock->Release();
		//	currentThread->Yield();
		//}
	
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
			passengersOnTrain--;
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
	
	//printf("PASSENGER%d finished!\n", myID);
	passengersFinished++;
	if(myTicket.hasEaten)
		passengersEaten++;
	if(myTicket.hasSlept)
		passengersSlept++;
}//end of passenger 

void Chef(int number){
	const unsigned int myID=number;
	//printf("chef%d is alive\n",myID);
	
	while(true){
		chefLock[myID]->Acquire();
		if(chefFirstClassOrdersLeft[myID]!=0){
			//do a first class order
			chefOrdersLineLock[myID]->Acquire();
			chefLock[myID]->Release();
			//yield to simulate cooking their order
			printf("Chef %d of Train 0 is preparing food\n",myID);
			for(int i=0; i<(rand()%13)+12; i++){
				currentThread->Yield();
			}
			chefFirstClassOrdersLeft[myID]--;
			//printf("chef%d cooked the order and now has %d left in his 1st class\n",myID,chefFirstClassOrdersLeft[myID]);
			chefFirstClassOrdersLine[myID]->Signal(chefOrdersLineLock[myID]);
			chefOrdersLineLock[myID]->Release();

			
		}
		else if(chefRegularOrdersLeft[myID]!=0){
			//do a regular order
			chefOrdersLineLock[myID]->Acquire();
			chefLock[myID]->Release();
			printf("Chef %d of Train 0 is preparing food\n",myID);
			for(int i=0; i<(rand()%13)+12; i++){
				currentThread->Yield();
			}
			chefRegularOrdersLeft[myID]--;
			//printf("chef%d cooked the order and now has %d left in his reg class\n",myID,chefFirstClassOrdersLeft[myID]);
			chefRegularOrdersLine[myID]->Signal(chefOrdersLineLock[myID]);
			chefOrdersLineLock[myID]->Release();
		}
		else{	
			//check food stock levels
			globalFoodLock->Acquire();
			for(int i=0;i<4;i++){
				if(foodStock[i]<= MIN_STOCK){
					printf("Chef %d of Train 0 calls for more inventory\n",myID);
					foodStockLow[i]=true;
				}
			}
			globalFoodLock->Release();
			//go on break
			//printf("chef%d has nothing requiring his immediate attention\n", myID);
			chefOnBreak[myID]=true;
			printf("Chef %d of Train 0 is going for a break\n",myID);
			printf("Chef %d of Train 0 returned from break\n",myID);
			chef[myID]->Wait(chefLock[myID]);
		}
	}

}//--end of CHEF

void Porter(int number){
	const unsigned int myID=number;
	//printf("porter %d is alive\n", myID);

	while(true){
		porterLock[myID]->Acquire();
		if(porterBaggageLineLength[myID]!=0){
			porterBaggageLineLock[myID]->Acquire();
			//printf("porter%d has %d people in baggage line and is signalling the first to come up\n", myID, porterBaggageLineLength[myID]);
			//I process a passenger
			porterBaggageLine[myID]->Signal(porterBaggageLineLock[myID]);
			//wait for passenger to tell me their name :D
			porterLine[myID]->Wait(porterBaggageLineLock[myID]);
			printf("Porter %d of Train 0 picks up bags of 1st class Passenger %d\n", myID, passengerBaggage[myID]->holder);
			porterBaggageLineLength[myID]--;
			porterBaggageLineLock[myID]->Release();
		}
		else if(porterFirstClassBeddingLineLength[myID]!=0){
			porterBeddingLineLock[myID]->Acquire();
			//printf("porter%d has %d people in 1st class bedding line and is signalling the first to come up\n", myID, porterBeddingLineLength[myID]);
			//I process a passenger
			porterFirstClassBeddingLine[myID]->Signal(porterBeddingLineLock[myID]);
			//wait for passenger to tell me their name C|:-D
			porterLine[myID]->Wait(porterBeddingLineLock[myID]);
			printf("Porter %d of Train 0 gives bedding to Passenger %d\n",myID, passengerBedding[myID]->holder);
			porterFirstClassBeddingLineLength[myID]--;
			passengerBedding[myID]->hasBedding = true;
			porterLine[myID]->Signal( porterBeddingLineLock[myID] );
			porterBeddingLineLock[myID]->Release();
		}
		else if(porterBeddingLineLength[myID]!=0){
			porterBeddingLineLock[myID]->Acquire();
			//printf("porter%d has %d people in bedding line and is signalling the first to come up\n", myID, porterBeddingLineLength[myID]);
			//I process a passenger
			porterBeddingLine[myID]->Signal(porterBeddingLineLock[myID]);
			//wait for passenger to tell me their name C|:-D
			porterLine[myID]->Wait(porterBeddingLineLock[myID]);
			printf("Porter %d of Train 0 gives bedding to Passenger %d\n",myID, passengerBedding[myID]->holder);
			porterBeddingLineLength[myID]--;
			passengerBedding[myID]->hasBedding = true;
			porterLine[myID]->Signal( porterBeddingLineLock[myID] );
			porterBeddingLineLock[myID]->Release();
		}
		else{
			//printf("porter%d has nothing requiring his immediate attention\n", myID);
			porterAvailable[myID] = true;
			porter[myID]->Wait(porterLock[myID]);
		}
	}
}//end of porter
		
	

void CoachAttendant(int number){
	const unsigned int myID=number;
	int myChef=-1;
	//printf("coach attendant%d is alive\n", myID);

	while(true){
		myChef=-1;
		coachAttendantLock[myID]->Acquire();
		//printf("coachAttendant%d acquired a lock on themselves\n", myID);
		if(coachAttendantFirstLineLength[myID]!=0){
			coachAttendantLineLock[myID]->Acquire();
			//printf("coachAttendant%d has %d people in FC line\n and is signalling the first to come up\n", myID, coachAttendantFirstLineLength[myID]);
			//I process a fc. passenger in line
			coachAttendantFirstLine[myID]->Signal( coachAttendantLineLock[myID]);
			//wait for a passenger to hand over their ticket
			coachAttendantLine[myID]->Wait(coachAttendantLineLock[myID]);
			getSeatLock[myID]->Acquire();
			coachAttendantLineLock[myID]->Release();

			//find a seat for this passenger
			globalSeatLock->Acquire();
			//side effect---- seat is incremented
			passengerTickets[myID]->seatNumber=nextSeat++;
			globalSeatLock->Release();
			//I wake up the passenger and wait for them to have a porter
			getSeat[myID]->Signal(getSeatLock[myID]);
			CAWaitingForPorterLock[myID]->Acquire();
			getSeatLock[myID]->Release();
			CAWaitingForPorter[myID]->Wait(CAWaitingForPorterLock[myID]);
			//passenger has signalled me that they have a porter and I can seat them now!
			printf("Coach Attendant %d of Train 0 gives seat number %d to Passenger %d\n", myID, passengerTickets[myID]->seatNumber, passengerTickets[myID]->holder);
			coachAttendantFirstLineLength[myID]--;
			CAWaitingForPorterLock[myID]->Release();
			boardingPassengersCountLock->Acquire();
			passengersBoardedAtThisStop[currentStop]++;
			boardingPassengersCountLock->Release();
			if(passengersBoardedAtThisStop[currentStop]==passengersWaitingToBoard[currentStop]){
				waitingForBoardingCompletionLock->Acquire();
				//printf("CA%d finds that all passengers have boarded for this stop and is telling conductor\n",myID);
				waitingForBoardingCompletion->Signal(waitingForBoardingCompletionLock);
				waitingForBoardingCompletionLock->Release();
			}
		}
		else if(coachAttendantRegularLineLength[myID]!=0){
			coachAttendantLineLock[myID]->Acquire();
			//printf("coachAttendant%d has %d people in reg line\n and is signalling the first to come up\n", myID, coachAttendantRegularLineLength[myID]);
			//I process a reg. passenger in line
			coachAttendantRegularLine[myID]->Signal( coachAttendantLineLock[myID]);
			//wait for a passenger to hand over their ticket
			coachAttendantLine[myID]->Wait(coachAttendantLineLock[myID]);
			getSeatLock[myID]->Acquire();
			coachAttendantLineLock[myID]->Release();
			//find a seat for this passenger
			globalSeatLock->Acquire();
			passengerTickets[myID]->seatNumber=nextSeat;
			//side effect seat is incremented after print
			printf("Coach Attendant %d of Train 0 gives seat number %d to Passenger %d\n", myID, nextSeat++, passengerTickets[myID]->holder);
			//decrement my line length
			coachAttendantRegularLineLength[myID]--;
			globalSeatLock->Release();
			getSeat[myID]->Signal(getSeatLock[myID]);
			getSeatLock[myID]->Release();
			boardingPassengersCountLock->Acquire();
			passengersBoardedAtThisStop[currentStop]++;
			boardingPassengersCountLock->Release();
			if(passengersBoardedAtThisStop[currentStop]==passengersWaitingToBoard[currentStop]){
				waitingForBoardingCompletionLock->Acquire();
				//printf("CA%d finds that all passengers have boarded for this stop and is telling conductor\n",myID);
				waitingForBoardingCompletion->Signal(waitingForBoardingCompletionLock);
				waitingForBoardingCompletionLock->Release();
			}
				
		}
		else if(coachAttendantFoodLineLength[myID] != 0){
			//do food orders
			coachAttendantLineLock[myID]->Acquire();
			//printf("coachAttendant%d has %d people in food line\n and is signalling the first to come up\n", myID, coachAttendantFoodLineLength[myID]);
			coachAttendantFoodLine[myID]->Signal(coachAttendantLineLock[myID]);

			coachAttendantGetFoodLock[myID]->Acquire();
			coachAttendantLineLock[myID]->Release();

			//validate food orders until what they want is in stock
			do{
				coachAttendantGetFood[myID]->Wait(coachAttendantGetFoodLock[myID]);
				globalFoodLock->Acquire();

				if(foodStock[coachAttendantOrder[myID]->foodOrder] == 0){
					coachAttendantOrder[myID]->validFoodOrder = false;
				}
				else {
					coachAttendantOrder[myID]->validFoodOrder = true;
					foodStock[coachAttendantOrder[myID]->foodOrder]--;
				}
				globalFoodLock->Release();

				coachAttendantGetFood[myID]->Signal(coachAttendantGetFoodLock[myID]);
			} while (!coachAttendantOrder[myID]->validFoodOrder);
			printf("Coach Attendant %d of Train 0 takes food order of 1st class Passenger %d\n",myID,coachAttendantOrder[myID]->holder);
			for(int i = 0; i < CHEFS; i++){
				chefLock[i]->Acquire();
				if(chefOnBreak[i]){
					chefOnBreak[i] = false;
					myChef= i;
					//my chef was waiting for work, so i come up and signal him
					chef[myChef]->Signal(chefLock[myChef]);
					break;
				}
				else{
					chefLock[i]->Release();
				}
			}
			//No chef was available, so I have to get in line
			if(myChef==-1){
				myChef = 0;
				chefLock[0]->Acquire();
				int shortestLength = chefFirstClassOrdersLeft[0];
				for(int i = 1; i < CHEFS; i++){
					chefLock[i]->Acquire();
					if(chefFirstClassOrdersLeft[i] < shortestLength){
						shortestLength = chefFirstClassOrdersLeft[i];
						chefLock[myChef]->Release();
						myChef = i;
					}
					else{
						chefLock[i]->Release();
					}				
				}
				chef[myChef]->Signal(chefLock[myChef]); //QUESTIONABLE
			}
			//now we definitely have a chef that will eventually want to see our order
			//printf("CA%d acquires chef%d's lock\n",myID,myChef);
			chefOrdersLineLock[myChef]->Acquire();
			chefFirstClassOrdersLeft[myChef]++;
			chefLock[myChef]->Release();
			//wait for the chef to cook my food
			//printf("CA%d is waiting on chef%d to cook\n",myID,myChef);
			chefFirstClassOrdersLine[myChef]->Wait(chefOrdersLineLock[myChef]);
			chefOrdersLineLock[myChef]->Release();
			printf("Coach Attendant %d of Train 0 takes food prepared by Chef %d to the 1st class Passenger %d\n",myID,myChef,coachAttendantOrder[myID]->holder);
			//printf("CA%d has delivered the food from the chef, and is waiting for a tip\n",myID);
			//wait for your tip
			coachAttendantGetFood[myID]->Wait(coachAttendantGetFoodLock[myID]);
			//I got my tip and I'm done with this guy
			coachAttendantFoodLineLength[myID]--;
			coachAttendantGetFoodLock[myID]->Release();			
		}
		else{
			//printf("coachAttendant%d has nothing requiring his immediate attention\n", myID);
			coachAttendantAvailable[myID] = true;
			coachAttendant[myID]->Wait(coachAttendantLock[myID]);
		}
		//FOR NOW coachAttendantLock[myID]->Release();
	}
} //end of CoachAttendant

void TicketChecker(int number){
        const unsigned int myID = number;

                while(currentStop<MAX_STOPS){
                        waitingForStopLock->Acquire(); //TC will try to acquire a lock he owns, but that's OK
                        if(conductorWaiting){
                                waitingForStop->Signal(waitingForStopLock);
                       // printf("the ticket checker is asleep, waiting for the next stop\n");
                                waitingForStop->Wait(waitingForStopLock);
                        }
                        else{
                                ticketCheckerWaiting=true;
                                waitingForStop->Wait(waitingForStopLock);
                        }
                        ticketCheckerWaiting=false;
                        //FOR NOW DONT DO THIS waitingForStopLock->Release();


                        //wakes up all passengers waiting at the current stop
                        waitingForTrainLock[currentStop]->Acquire();
                        waitingForTrain[currentStop]->Broadcast(waitingForTrainLock[currentStop]);
                        //printf("the ticket checker has told all the passengers it's time to board\n");
                        //and acquires a lock on himself so that he can set his status and begin helping customers
                        ticketCheckerLock->Acquire();
                        waitingForTrainLock[currentStop]->Release();
                        //by law, no one can be in line yet, so I am available
                        ticketCheckerAvailable=true;

                        for(int i = 0; i < passengersWaitingToBoard[currentStop]; i++){
                                //wait for passengers to come up and give me their ticket
                                //printf("ticket checker is waiting for the next passenger\n");
                                ticketChecker->Wait(ticketCheckerLock);
                                //printf("ticket checker is validating ticket for passenger%d \n", ticketToCheck->holder);
                                //validate the ticket i have just received
                                ticketLock->Acquire();
if(! TEST_1 ) {
                                if(ticketToCheck->getOnStop==currentStop){
                                        ticketToCheck->okToBoard=true;
                                        //increment passenger count?
					printf("Ticket Checker validates the ticket of passenger %d of Train 0 at stop number %d\n", ticketToCheck->holder, currentStop);
                                        passengerCountLock->Acquire();
                                        passengersOnTrain++;
                                        passengerCountLock->Release();
                                }
                                else {
                                        ticketToCheck->okToBoard=false;
					printf("Ticket Checker invalidates the ticket of passenger %d of Train 0 at stop number %d\n", ticketToCheck->holder, currentStop);
                                }
} else{
	ticketToCheck->okToBoard=false;
}                      
                                //let the person know their ticket has been checked and that they
                                //can move on
                                ticketCheck->Signal(ticketLock);
                                ticketLock->Release();                          

                                //move to the next person in line, or wait for someone to come up
                                if(ticketCheckerLineLength>0){

                                        //signal on ticketCheckerLine?
                                        ticketCheckerLine->Signal(ticketCheckerLock);
	
                                }
                                else {
                                        ticketCheckerAvailable=true;
                                }
                                printf("the ticket checker has %d passengers waiting, and has status %d \n", ticketCheckerLineLength, ticketCheckerAvailable);
                        }
                        //necessary?
                        ticketCheckerLock->Release();
                        //all passengers at this stop have been checked- inform conductor and wait for the next stop
                        waitingForGoLock->Acquire();
                        waitingForGo->Signal(waitingForGoLock);
                        waitingForGoLock->Release();
                }
                                       
}//end of ticketChecker

void Conductor(int number){
        const unsigned int myID = number;

        while(currentStop<MAX_STOPS-1){
                waitingForStopLock->Acquire();

                if(ticketCheckerWaiting){
                        //conductor yields 25-50 times before he decides the next stop has arrived      
                        for(int i = 0; i<(randomRange(0,26)+25); i++){
                        	currentThread->Yield();
                        }

                }
                else{
                        conductorWaiting=true;
                        waitingForStop->Wait(waitingForStopLock);
                        //conductor yields 25-50 times before he decides the next stop has arrived      
                        for(int i = 0; i<(randomRange(0,26)+25); i++){
                        	currentThread->Yield();
                        }
                        
                }
		atAStopLock->Acquire();
                currentStop++;
		printf("Conductor announces exit for Passengers at stop number %d\n",currentStop);
		nowStopped=true;
		atAStopLock->Release();
                //printf("WOO WOO, train has just pulled into stop %d\n", currentStop);

		//tell people to check the current stop
                waitingForStop->Broadcast(waitingForStopLock);
                waitingForGoLock->Acquire();
                waitingForStopLock->Release();
             
		conductorWaiting=false;
                waitingForGo->Wait(waitingForGoLock);

                //i have been cleared to go, so i do my bookeeping with a random chance and then move on
                //print the number of passengers in the train
		boardingPassengersCountLock->Acquire();
                waitingForGoLock->Release();
		//check to see if the kitchen requested more stock at this stop
		globalFoodLock->Acquire();
		for(int i=0;i<4;i++){
			if(foodStockLow[i]){
				foodStock[i]+=RESTOCK_AMOUNT;
			}
		}
		globalFoodLock->Release();

                //randomly check ticket value
                //randomly check dining car safe value
		//wait for coach attendant to let me know that everyone has gotten on...

		//tell everyone waiting to get off that they should get off
		getOffAtThisStopLock->Acquire();
		getOffAtThisStop->Broadcast(getOffAtThisStopLock);
		getOffAtThisStopLock->Release();

		if(passengersBoardedAtThisStop[currentStop]!=passengersWaitingToBoard[currentStop] && !TEST_1){
			waitingForBoardingCompletionLock->Acquire();
			boardingPassengersCountLock->Release();
			//printf("conductor has found that not everyone has boarded, so he's going to sleep\n");
			waitingForBoardingCompletion->Wait(waitingForBoardingCompletionLock);
			//printf("Conductor finds that everyone has boarded on this stop, time to loop againXD\n");
			waitingForBoardingCompletionLock->Release();
		}
		else{
			//printf("Conductor finds that everyone has boarded on this stop, time to loop againXD\n");
			boardingPassengersCountLock->Release();
		}
		//randomly do bookeeping with a 1/3 chance 
		if(rand()%3==0){
			diningCarRevenueLock->Acquire();
			ticketRevenueLock->Acquire();
			printf("Train 0 Dining Car revenue = %d\n",diningCarRevenue);
			printf("Train 0 Ticket Revenue = %d\n",ticketRevenue);
			totalRevenue= diningCarRevenue+ticketRevenue;
			printf("Total Train 0 Revenue = %d\n",totalRevenue);
			diningCarRevenueLock->Release();
			ticketRevenueLock->Release();
		}


		//passengerCountLock->Acquire();
		printf("Passengers in the train 0 = %d\n", passengersOnTrain);
		//passengerCountLock->Release();
		//I start the train up again
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
	//we've made it to the last stop
		//but we aren't guaranteed that everyone has gotten off yet 
	//if(passengersOnTrain != PASSENGERS)
		//throwError(true, "Not all passengers on the train");
	while(passengersOnTrain!=0){
		getOffAtThisStopLock->Acquire();
		getOffAtThisStop->Broadcast(getOffAtThisStopLock);
		getOffAtThisStopLock->Release();
		for(int i=0; i<30; i++){
			currentThread->Yield(); //give the lingering passengers a chance to finish eating/sleeping
		}
	}
	//do a final set of prints to summarize
	
	diningCarRevenueLock->Acquire();
	ticketRevenueLock->Acquire();
	printf("Train 0 Dining Car revenue = %d\n",diningCarRevenue);
	printf("Train 0 Ticket Revenue = %d\n",ticketRevenue);
	totalRevenue= diningCarRevenue+ticketRevenue;
	printf("Total Train 0 Revenue = %d\n",totalRevenue);
	diningCarRevenueLock->Release();
	ticketRevenueLock->Release();
	passengerCountLock->Acquire();
	printf("Passengers in the train 0 = %d\n", passengersOnTrain);
	passengerCountLock->Release();

	if(nextSeat != PASSENGERS)
		throwError(true, "Not all passengers were seated");
	/*if(passengersFinished < PASSENGERS){
		for(int i = 0; i < 200; i++){
			currentThread->Yield();	
		}
	}
	if(passengersFinished != PASSENGERS)
		throwError(true, "Not all passengers have finished");
	if(passengersFinished != passengersEaten)
		throwError(true, "Passenger left without eating!");
	if(passengersFinished != passengersSlept)
		throwError(true, "Passengers left without sleeping!");
	*/
}//end of conductor

void Steward(int number){
	const unsigned int myID = number;
	
	while(true){
		//randomly do bookeeping
		if(rand()%5){
			//look at waiter earnings, add them to dining car earnings, clear waiter earnings
			//diningCarRevenueLock->Acquire();
			//waiterRevenueLock->Acquire();
			diningCarRevenue+=waiterRevenue;
			waiterRevenue=0;
			printf("Steward gives the Dining Car revenue %d to the Conductor\n",diningCarRevenue);
			//waiterRevenueLock->Release();
			//diningCarRevenueLock->Release();
		}
		stewardLock->Acquire();
		if(stewardLineLength!=0){
			//serve passenger 

			stewardLine->Signal(stewardLock);
			steward->Wait(stewardLock);
			//passenger has set his ticket value
			stewardGiveWaiterLock->Acquire();
			stewardLock->Release();
			diningCarCapacityLock->Acquire();
			if(diningCarCapacity==0){
				//printf("steward can't fit any more passengers into the dining car\n");
				printf("Steward 0 of Train 0 informs Passenger %d-the dining car is full\n",stewardTicketToCheck->holder); 
				diningCarCapacityLock->Release();
				//give them a null waiter so they can do nothing
				stewardTicketToCheck->waiter=-1;
				stewardLineLength--;
				stewardGiveWaiter->Signal(stewardGiveWaiterLock);
				stewardGiveWaiterLock->Release();
			}//steward is done with this passenger

			else{
				printf("Steward 0 of Train 0 informs Passenger %d-the dining car is not full\n",stewardTicketToCheck->holder); 
				bool foundAWaiter = false;
				//There was space for them, so take away a seat
				diningCarCapacity--;
				diningCarCapacityLock->Release();
				for(int i=0; i<WAITERS;i++){
					//look for waiters on break
					waiterLock[i]->Acquire();
					if(waiterOnBreak[i]==true){
						waiterOnBreak[i]=false;
						stewardTicketToCheck->waiter=i;
						waiter[i]->Signal(waiterLock[i]);
						//printf("steward is giving waiter%d to passenger%d\n",i,stewardTicketToCheck->holder);
						printf("Steward calls Waiters %d from break\n",i);
						foundAWaiter = true;
						break;
					}
				}
				if(!foundAWaiter){
					//no waiters were on break, so I pick the shortest line?
					waiterLock[0]->Acquire();
					int min=waiterLineLength[0];
					stewardTicketToCheck->waiter = 0;
					for(int i=1; i<WAITERS;i++){
						waiterLock[i]->Acquire();
						if(waiterLineLength[i]<min){
							waiterLock[stewardTicketToCheck->waiter]->Release();
							min = waiterLineLength[i];
							stewardTicketToCheck->waiter=i;
						}
						else{
							waiterLock[i]->Release();
						}
					}
				}
				//now we are guaranteed to have a waiter on our ticket
				stewardGiveWaiter->Signal(stewardGiveWaiterLock);
				stewardLock->Acquire();
				stewardGiveWaiterLock->Release();
				steward->Wait(stewardLock);
				printf("Steward didn't wake up a waiter...\n");
				waiterLock[stewardTicketToCheck->waiter]->Release();	
				finalConfirmation->Wait(stewardLock);
				//release steward lock?			
			}	
		}
		else{
		//steward always gives updated revenue when he has nothing to do- this way when
		//the last passenger is done eating, the total revenue of the train should be accurate
			diningCarRevenueLock->Acquire();
			waiterRevenueLock->Acquire();
			diningCarRevenue+=waiterRevenue;
			waiterRevenue=0;
			printf("Steward gives the Dining Car revenue %d to the Conductor\n",diningCarRevenue);
			waiterRevenueLock->Release();
			diningCarRevenueLock->Release();

			//wait for something to do
			stewardAvailable=true;
			steward->Wait(stewardLock);
		}
	}
}//end of Steward

void Waiter(int number){
	const unsigned int myID = number;
	int myChef;
	while(true){
		myChef=-1;
		waiterLock[myID]->Acquire();
		waiterLineLock[myID]->Acquire();
		if(waiterLineLength[myID]!=0){
			waiterGetFoodLock[myID]->Acquire();
			waiterLock[myID]->Release();  //maybe wrong??!
			waiterLine[myID]->Signal(waiterLineLock[myID]);
			waiterLineLock[myID]->Release();
			//process a waiting passenger
			do{
				waiterGetFood[myID]->Wait(waiterGetFoodLock[myID]);
				globalFoodLock->Acquire();

				if(foodStock[passengerFood[myID]->foodOrder] == 0){
					passengerFood[myID]->validFoodOrder = false;
				}
				else {
					passengerFood[myID]->validFoodOrder = true;
					foodStock[passengerFood[myID]->foodOrder]--;
				}
				globalFoodLock->Release();

				waiterGetFood[myID]->Signal(waiterGetFoodLock[myID]);
			} while (!passengerFood[myID]->validFoodOrder);
			printf("Waiter %d is serving Passenger %d of Train 0\n",myID,passengerFood[myID]->holder);

			for(int i = 0; i < CHEFS; i++){
				chefLock[i]->Acquire();
				if(chefOnBreak[i]){
					chefOnBreak[i] = false;
					myChef= i;
					//my chef was waiting for work, so i come up and signal him
					chef[myChef]->Signal(chefLock[myChef]);
					break;
				}
				else{
					chefLock[i]->Release();
				}
			}
			//No Porter was available, so I have to get in line
			if(myChef==-1){
				myChef = 0;
				chefLock[0]->Acquire();
				int shortestLength = chefRegularOrdersLeft[0];
				for(int i = 1; i < CHEFS; i++){
					chefLock[i]->Acquire();
					if(chefRegularOrdersLeft[i] < shortestLength){
						shortestLength = chefRegularOrdersLeft[i];
						chefLock[myChef]->Release();
						myChef = i;
					}
					else{
						chefLock[i]->Release();
					}				
				}
				chef[myChef]->Signal(chefLock[myChef]); //QUESTIONABLE
			}
			//now we definitely have a chef that will eventually want to see our order
			//printf("waiter%d acquires chef%d's lock\n",myID,myChef);
			chefOrdersLineLock[myChef]->Acquire();
			chefRegularOrdersLeft[myChef]++;
			chefLock[myChef]->Release();
			//wait for the chef to cook my food
			//printf("waiter%d is waiting on chef%d to cook\n",myID,myChef);
			chefRegularOrdersLine[myChef]->Wait(chefOrdersLineLock[myChef]);
			chefOrdersLineLock[myChef]->Release();
			printf("waiter%d has the order from the chef\n",myID);
			//now I give the food to the passenger, and wait for him to eat,
			printf("waiter%d is waiting for the passenger to eat\n",myID);
			
			//waiterWaitingForEatingPassenger[myID]->Wait(waiterGetFoodLock[myID]);
			waiterGetFood[myID]->Wait(waiterGetFoodLock[myID]);

			//passenger is done eating and wating for a bill
			printf("waiter%d is giving passenger the bill\n",myID);

			//passengerWaitingForBill[passengerFood[myID]->holder]->Signal(waiterGetFoodLock[myID]);
			//wait for payment
			waiterGetFood[myID]->Signal(waiterGetFoodLock[myID]);

			printf("waiter%d is waiting for passenger to pay\n",myID);

			//waiterWaitingForMoney[myID]->Wait(waiterGetFoodLock[myID]);
			waiterGetFood[myID]->Wait(waiterGetFoodLock[myID]);

			//printf("waiter%d was paid by passenger%d!\n",myID, passengerFood[myID]->holder);
		
			//update steward about the dining car safe
			waiterRevenueLock->Acquire();
			waiterRevenue+=20;
			waiterRevenueLock->Release();

			waiterLineLength[myID]--;
			diningCarCapacityLock->Acquire();
			diningCarCapacity++;  
			diningCarCapacityLock->Release();
			waiterGetFoodLock[myID]->Release();
		}
		else{
			printf("Waiter %d of Train 0 is going for a break\n",myID);
			waiterOnBreak[myID]=true;
			waiterLineLock[myID]->Release();
			waiter[myID]->Wait(waiterLock[myID]);
			printf("Waiter %d of Train 0 returned from break\n",myID);
		}
	}
}//end of Waiter


void simulation(){

//pre-simulation prints
printf("Number of Coach Attendants per Train = %d\n", COACH_ATTENDANTS);
printf("Number of Trains per Train = ??\n");
printf("Number of Chefs per Train = %d\n",CHEFS);
printf("Number of Waiters per Train = %d\n",WAITERS);
printf("Number of Porters per Train = %d\n",PORTERS);
printf("Total number of passengers = %d\n",PASSENGERS);
printf("Number of passengers for Train 0 = %d\n",PASSENGERS);



//setup bullhockey
char * name;
char buffer[40];
for(int i=0; i<MAX_STOPS; i++){
        sprintf(buffer, "%d", i);
        name = new char[40];
        waitingForTrain[i]=new Condition(strcat(strcpy(name, "waitingForTrainCV"),buffer));
	name = new char[40];
        waitingForTrainLock[i]=new Lock(strcat(strcpy(name, "waitingForTrainLock"),buffer));
        passengersWaitingToBoard[i]=0;
	passengersBoardedAtThisStop[i]=0;
}

ticketChecker=new Condition("ticketChecker");
ticketCheckerLine=new Condition("ticketCheckerLine");
ticketCheckerLock=new Lock("ticketCheckerLock");
passengerCountLock=new Lock("passengerCountLock");

waitingForGo=new Condition("waitingForGoCV");
waitingForGoLock=new Lock("waitingForGoLock");
waitingForStop=new Condition("waitingForStopCV");
waitingForStopLock=new Lock("waitingForStopLock");

for(int i=0; i<COACH_ATTENDANTS; i++){
	sprintf(buffer, "%d", i);
	name = new char[40];
	coachAttendant[i]=new Condition(strcat(strcpy(name,"coachAttendantCV"),buffer));
	name = new char[40];
	coachAttendantFirstLine[i]=new Condition(strcat(strcpy(name,"coachAttendantFirstLineCV"),buffer));
	name = new char[40];
	coachAttendantRegularLine[i]=new Condition(strcat(strcpy(name,"coachAttendantRegularLineCV"),buffer));
	name = new char[40];
	coachAttendantLine[i]=new Condition(strcat(strcpy(name,"coachAttendantLineCV"),buffer));
	name = new char[40];
	coachAttendantLock[i]=new Lock(strcat(strcpy(name,"coachAttendantLock"),buffer));
	name = new char[40];
	coachAttendantLineLock[i]=new Lock(strcat(strcpy(name,"coachAttendantLineLock"),buffer));
	name = new char[40];
	getSeatLock[i]=new Lock(strcat(strcpy(name,"getSeatLock"),buffer));
	name = new char[40];
	getSeat[i]=new Condition(strcat(strcpy(name,"getSeat"),buffer));
	name = new char[40];
	CAWaitingForPorter[i]=new Condition(strcat(strcpy(name,"CAWaitingForPorterCV"),buffer));
	name = new char[40];
	CAWaitingForPorterLock[i]=new Lock(strcat(strcpy(name,"CAWaitingForPorterLock"),buffer));
	name = new char[40];
	coachAttendantGetFood[i]=new Condition(strcat(strcpy(name,"coachAttendantGetFoodCV"),buffer));
	name = new char[40];
	coachAttendantGetFoodLock[i]=new Lock(strcat(strcpy(name,"coachAttendantGetFoodLock"),buffer));
	name = new char[40];
	coachAttendantFoodLine[i]=new Condition(strcat(strcpy(name,"coachAttendantFoodLineCV"),buffer));

	coachAttendantFoodLineLength[i]=0;
	coachAttendantFirstLineLength[i]=0;
	coachAttendantRegularLineLength[i]=0;
	coachAttendantAvailable[i]=false;
	foodOrdersWaiting[i]=0;
}

for(int i=0; i<PORTERS;i++){
	porterBaggageLineLength[i]=0;
	porterBeddingLineLength[i]=0;
	porterFirstClassBeddingLineLength[i]=0;

	porterAvailable[i]=0;

	sprintf(buffer, "%d", i);
	name = new char[40];
	porter[i]=new Condition(strcat(strcpy(name, "porterCV"), buffer));
	name = new char[40];
	porterBaggageLine[i]=new Condition(strcat(strcpy(name, "porterBaggageLineCV"), buffer));
	name = new char[40];
	porterLine[i]=new Condition(strcat(strcpy(name, "porterLineCV"), buffer));
	name = new char[40];
	porterLock[i]=new Lock(strcat(strcpy(name, "porterLock"),buffer));
	name = new char[40];
	porterBaggageLineLock[i]=new Lock(strcat(strcpy(name, "porterBaggageLineLock"),buffer));
	name = new char[40];
	porterBeddingLine[i]=new Condition(strcat(strcpy(name, "porterBeddingLineCV"), buffer));
	name = new char[40];
	porterFirstClassBeddingLine[i]=new Condition(strcat(strcpy(name, "porterFirstClassBeddingLineCV"), buffer));
	name = new char[40];
	porterBeddingLineLock[i]=new Lock(strcat(strcpy(name, "porterBeddingLineLock"),buffer));
}


for(int i = 0; i < WAITERS; i++){
	sprintf(buffer, "%d", i);
	name = new char[40];
	waiterLock[i] = new Lock(strcat(strcpy(name,"waiterLock"),buffer));
	name = new char[40];
	waiter[i] = new Condition(strcat(strcpy(name,"waiterCV"),buffer));
	name = new char[40];
	waiterLineLock[i] = new Lock(strcat(strcpy(name,"waiterLineLock"),buffer));
	name = new char[40];
	waiterLine[i] = new Condition(strcat(strcpy(name,"waiterLineCV"),buffer));
	name = new char[40];
	waiterGetFoodLock[i] = new Lock(strcat(strcpy(name,"getFoodLock"),buffer));
	name = new char[40];
	waiterGetFood[i] = new Condition(strcat(strcpy(name,"getFoodCV"),buffer));

	waiterLineLength[i] = 0;
	waiterOnBreak[i] = false;
}

for(int i=0; i<CHEFS;i++){
	chefOnBreak[i]=false;
	chefFirstClassOrdersLeft[i]=0;
	chefRegularOrdersLeft[i]=0;

	sprintf(buffer, "%d", i);
	name = new char[40];
	chefLock[i] = new Lock(strcat(strcpy(name, "chefLock"), buffer));
	name = new char[40];
	chef[i]=new Condition(strcat(strcpy(name, "chefCV"), buffer));
	name = new char[40];
	chefRegularOrdersLine[i]=new Condition(strcat(strcpy(name, "chefOrdersLineCV"), buffer));
	name = new char[40];
	chefFirstClassOrdersLine[i]=new Condition(strcat(strcpy(name, "chefFirstClassOrdersLineCV"), buffer));
	name = new char[40];
	chefOrdersLineLock[i]=new Lock(strcat(strcpy(name, "chefOrdersLineLock"), buffer));

}

Thread* t;
char *pbuff;
for(int i=0; i<PASSENGERS; i++){
        pbuff=new char[20];
        sprintf(pbuff, "Passenger%d", i);
        t=new Thread(pbuff);
        //a more descriptive name is not given because each thread will have a myID var.
        t->Fork((VoidFunctionPtr)Passenger, i);
}

t=new Thread("ticketChecker");
t->Fork((VoidFunctionPtr)TicketChecker, 1);

t=new Thread("steward");
t->Fork((VoidFunctionPtr)Steward, 1);
						if(!TEST_2){
for(int i=0; i<COACH_ATTENDANTS; i++){
        pbuff=new char[20];
	sprintf(pbuff, "CoachAttendant%d", i);
	t=new Thread(pbuff);
	t->Fork((VoidFunctionPtr)CoachAttendant,i);
}
							}

for(int i=0; i<PORTERS; i++){
	pbuff=new char[20];
	sprintf(pbuff,"Porter%d", i);
	t=new Thread(pbuff);
	t->Fork((VoidFunctionPtr)Porter,i);
}

for(int i = 0; i < WAITERS; i++){
        pbuff = new char[20];
	sprintf(pbuff, "Waiter%d", i);
	t = new Thread(pbuff);
	t->Fork((VoidFunctionPtr)Waiter, i);
}

for(int i = 0; i < CHEFS; i++){
        pbuff = new char[20];
	sprintf(pbuff, "Chef%d", i);
	t = new Thread(pbuff);
	t->Fork((VoidFunctionPtr)Chef, i);
}

t=new Thread("Conductor");
t->Fork((VoidFunctionPtr)Conductor, 1);

}//end simulation


///////////////////////////////////////////////////
//                      Test 1                          
///////////////////////////////////////////////////
// This test will cause all passengers to
// be given invalid tickets.  The result of
// this is that they will not be allowed to 
// board the train.
// 
// Fulfills
//	1. Ticket Checker does not allow the 
//		Passengers to be in the Train 
//		who have invalid ticket. 
///////////////////////////////////////////////////

void Test1(){
	TEST_1 = true;
	simulation();
}

///////////////////////////////////////////////////
//                      Test 2                          
///////////////////////////////////////////////////
// This test will have a train with no Coach
// Attendants.  The result of this is that no
// passengers will be seated.
// 
// Fulfills
//	2. Passengers are not allowed to take seat 
//		on their own. They must wait for 
//		the Coach Attendant. No 2 Passengers
//		are given same seat number.
///////////////////////////////////////////////////

void Test2(){
	TEST_2 = true;
	simulation();	
}

///////////////////////////////////////////////////
//                      Test 3                          
///////////////////////////////////////////////////
// This test will change the dining car's max
// capacity to 1.  The result is that passengers
// will have to likely wait for the dining car to 
// be empty before they can eat.
// 
// Fulfills
//	3. Passengers have to go back to their seat 
//		if the Dining Car is full and they 
//		should come back at some random time 
//		until they have been allowed to stay. 
///////////////////////////////////////////////////
void Test3(){
	TEST_3 = true;
	diningCarCapacity = 1;
	simulation();
}

///////////////////////////////////////////////////
//                      Test 4                        
///////////////////////////////////////////////////
// This will set the initial values of the train's
// food stock to be the minimum allowed stock.  The
// result of this is that the chef will order more
// at the next stop.
// 
// Fulfills
//	4. Chefs are to maintain the track of food 
//		inventory. If less than minimal, 
//		food is to be loaded on the next 
//		train stop. 
///////////////////////////////////////////////////
void Test4(){
	foodStock[0] = MIN_STOCK;
	foodStock[1] = MIN_STOCK;
	foodStock[2] = MIN_STOCK;
	foodStock[3] = MIN_STOCK;
	TEST_4 = true;
	simulation();
}

///////////////////////////////////////////////////
//                      Test 5                        
///////////////////////////////////////////////////
// This test makes all passengers wait until after 
// their stop to get off- the passengers also pay
// a small fine for this.
// 
// Fulfills
//	5. If passengers get off at their stop
//		they print a message and pay
//		a fine.
///////////////////////////////////////////////////
void Test5(){
	TEST_5 = true;
	simulation();
}

///////////////////////////////////////////////////
//                     Full Simulation                         
///////////////////////////////////////////////////
// This is a fully functional train simulation
//
// Fulfills
//	2. No 2 Passengers are given same seat number.
//	5. Steward wakes up Waiters when there are 
//		Passengers waiting in Dining Car. 
//	6. 1st class Passengers order food from their room. 
//	7. All Passengers must have bedding after 
//		having food, before getting off the 
//		Train at their stop. 
//	8. Coach Attendants and Conductor should keep 
//		track of number of Passengers at every 
//		stop of train. 
///////////////////////////////////////////////////

void TrainTests(){
	int choice;
        std::cout << "\n\n\nMain Menu\n";
        std::cout << "1. Test 1\n";
        std::cout << "2. Test 2\n";
        std::cout << "3. Test 3\n";
        std::cout << "4. Test 4\n";
	std::cout << "5. Test 5\n";
        std::cout << "6. Full Simulation\n";
        std::cout << "0. Exit\n";
        std::cout << "Choice: ";
        std::cin >> choice;

        switch(choice){
                case 1:
			Test1();
			break;
                case 2:
			Test2();
			break;
                case 3:
			Test3();
			break;
                case 4:
			Test4();
			break;
		case 5:
			Test5();
			break;
		case 6:
			simulation();
			break;
                case 0:
			break;
                default:
			break;
	}
	return;
}



