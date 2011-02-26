// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
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
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
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
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
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
// 	Setup and run a train simulation 
//		-josh
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
	int diningCarSeatNumber;
	bool firstClass;
	bool receivedFood;
	bool receivedBedding;
	bool seated;
	int price;
} Ticket;

//---------------------
// GLOBAL DATA
//---------------------
#define MAX_STOPS 10

// entities per train
#define CONDUCTORS 1
#define STEWARDS 1
#define TICKET_CHECKERS 1
#define COACH_ATTENDANTS 3
#define WAITERS 3         
#define CHEFS 2
#define PORTERS 3
#define PASSENGERS 20
#define TRAINS 1

#define DINING_CAR_SEATS 20
#define CHEF_INVENTORY_CAPACITY 3
#define CHEF_INVENTORY_MINIMUM 1
#define FIRST_CLASS_TICKET_PRICE 10
#define ECONOMY_CLASS_TICKET_PRICE 5
#define FOOD_PRICE 5

// test data
bool TEST_1 = false;
bool TEST_2 = false;
bool TEST_3 = false;
bool TEST_4 = false;
bool TEST_5 = false;
//--end test data

// setup data
Condition* setup = new Condition("setup");
Lock* setupLock = new Lock("setupLock");
int passengersInitialized = 0;
bool passengerGotOff[PASSENGERS];
//--end setup data

//data for Conductor/other attendants interactions
bool simulationRunning = true;
bool conductorWaiting = false;

int currentStop = 0;
int passengersOnTrain = 0;
int passengersSeated = 0;
int passengersWaitingToGetOff = 0;

Condition* conductor = new Condition("conductorCV");
Condition* waitingToLeaveStation = new Condition("waitingToLeaveStationCV");
Condition* waitingToArriveAtStation = new Condition("waitingToArriveAtStationCV");
Condition* signallingDeparture = new Condition("signallingCoachAttendanCV");
Condition* waitingForTrainStop[MAX_STOPS];
Condition* waitingToGetOff = new Condition("waitingToGetOffCV");

Lock* signallingDepartureLock = new Lock("signallingDepartureLock");
Lock* conductorLock = new Lock("conductorLock");
Lock* waitingToLeaveStationLock = new Lock("waitingToLeaveStationLock");
Lock* waitingToArriveAtStationLock = new Lock("waitingToArriveAtStationLock");
Lock* waitingForTrainStopLock[MAX_STOPS];
Lock* passengersBoardedLock = new Lock("passengersBoardedLock");
Lock* passengersWaitingToGetOffLock = new Lock("passengersWaitingToGetOffLock");
Lock* passengersSeatedLock = new Lock("passengersSeatedLock");
//--end Conductor data

// data for Passenger/Ticket Checker interactions
Ticket* ticketToCheck;

int ticketCheckerLineLength = 0;
bool ticketCheckerAvailable = false; 
int passengersWaitingToBoard[MAX_STOPS];
int ticketRevenue = 0;
int passengersChecked = 0; // for testing purpose to keep track
								// of valid and invalid tickets

Condition* ticketChecker = new Condition("ticketChecker"); //TC waits on this for a passenger 
Condition* ticketCheckerLine = new Condition("ticketCheckerLine"); //passengers wait on this when TC is unavailable

Lock* ticketCheckerLock = new Lock("ticketCheckerLock");
Lock* ticketRevenueLock = new Lock("ticketRevenueLock");
//--end Passenger/Ticket Checker data

// data for Passenger/Coach Attendant interactions
Ticket* coachAttendantSeatTicket[COACH_ATTENDANTS];
Ticket* coachAttendantFoodTicket[COACH_ATTENDANTS];

int nextSeat = 0;
int coachAttendantFirstClassLineLength[COACH_ATTENDANTS];
int coachAttendantEconomyClassLineLength[COACH_ATTENDANTS];
int coachAttendantFoodLineLength[COACH_ATTENDANTS];
bool coachAttendantAvailable[COACH_ATTENDANTS];

Condition* coachAttendant[COACH_ATTENDANTS]; // CA waits on this for a passenger 
Condition* coachAttendantFirstClassLine[COACH_ATTENDANTS]; // passengers wait on this when CA is unavailable
Condition* coachAttendantEconomyClassLine[COACH_ATTENDANTS]; 
Condition* coachAttendantFoodLine[COACH_ATTENDANTS];

Lock* seatLock = new Lock("seatNumberLock");
Lock* coachAttendantLock[COACH_ATTENDANTS];
Lock* coachAttendantFirstClassLineLengthLock[COACH_ATTENDANTS];
Lock* coachAttendantEconomyClassLineLengthLock[COACH_ATTENDANTS];
Lock* coachAttendantFoodLineLengthLock[COACH_ATTENDANTS];
Lock* choosingCoachAttendantLock = new Lock("choosingCoachAttendantLock");
//--end Passenger/Coach Attendant

// data for Passenger/Porter interactions
Ticket* porterBeddingTicket[PORTERS];
Ticket* porterLuggageTicket[PORTERS];

int porterFirstClassLuggageLineLength[PORTERS];
int porterBeddingLineLength[PORTERS];
bool porterAvailable[PORTERS];

Condition* porter[PORTERS]; // CA waits on this for a passenger 
Condition* porterFirstClassLuggageLine[PORTERS]; // passengers wait on this when CA is unavailable
Condition* porterBeddingLine[PORTERS]; 

Lock* porterLock[PORTERS];
Lock* porterFirstClassLuggageLineLengthLock[PORTERS];
Lock* porterBeddingLineLengthLock[PORTERS];
Lock* choosingPorterLock = new Lock("choosingPorterLock");
//--end Passenger/Porter

// data for Steward interactions
Ticket* stewardTicket;

int stewardLineLength = 0;
bool stewardAvailable = false; 
int diningCarRevenue = 0;
bool diningCarFull = false;
int diningSeatsAvailable = 0;
int waiterRevenue[WAITERS];

Condition* steward = new Condition("steward"); //Steward waits on this for a passenger 
Condition* stewardLine = new Condition("stewardLine"); // attendants wait on this when steward is busy

Lock* diningSeatLock = new Lock("diningSeatLock");
Lock* stewardLock = new Lock("stewardLock");
Lock* diningCarRevenueLock = new Lock("diningCarRevenueLock");
Lock* waiterRevenueLock[WAITERS];
//--end Steward data

// data for Chef interactions
Ticket* chefFirstClassTicket[CHEFS];
Ticket* chefEconomyClassTicket[CHEFS];

int chefFirstClassLineLength[CHEFS];
int chefEconomyClassLineLength[CHEFS];
int chefInventory[CHEFS];
bool chefAvailable[CHEFS];
bool chefKitchenClean[CHEFS];
bool chefReloadingInventoryAtNextStop[CHEFS];

Condition* chef[CHEFS]; // Chef waits on this for an order 
Condition* chefFirstClassLine[CHEFS]; // Chefs wait on this when chef is unavailable
Condition* chefEconomyClassLine[CHEFS]; 
Condition* chefWaitingToReloadInventory[CHEFS];

Lock* chefLock[CHEFS];
Lock* chefFirstClassLineLengthLock[CHEFS];
Lock* chefEconomyClassLineLengthLock[CHEFS];
Lock* chefInventoryLock[CHEFS];
Lock* choosingChefLock = new Lock("choosingChefLock");
//--end Chef

// data for Waiter interactions
Ticket* waiterFoodTicket[WAITERS];

int waiterLineLength[WAITERS];
bool waiterAvailable[WAITERS];

Condition* waiter[WAITERS]; // Waiter waits on this for a passenger 
Condition* waiterLine[WAITERS]; // passengers wait on this when CA is unavailable

Lock* waiterLock[WAITERS];
Lock* waiterLineLengthLock[WAITERS];
Lock* choosingWaiterLock = new Lock("choosingWaiterLock");
//--end Waiter

//-------------------
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

//----------------
// THREAD TYPES
//----------------

void Passenger(int number){
 
	// PASSENGER INITIALIZATION
	
	const unsigned int myID = number; // unique passender ID

	int myCoachAttendant = -1;
	int myPorter = -1;
	int myWaiter = -1;
	
	//initialization of ticket structure 
	Ticket myTicket;
	myTicket.getOnStop = (rand()%(MAX_STOPS-1));	// assumes passengers only get off at the last stop(for now)
	myTicket.getOffStop = randomRange(myTicket.getOnStop,MAX_STOPS);
	myTicket.holder=myID; // associated ticket with a certain passenger ID 
	myTicket.firstClass = (rand()%2);
	myTicket.receivedFood = false;
	myTicket.receivedBedding = false;
	myTicket.seated = false;
	if(myTicket.firstClass)
		myTicket.price = FIRST_CLASS_TICKET_PRICE;
	else
		myTicket.price = ECONOMY_CLASS_TICKET_PRICE;
	// pre-simulation print statements
	printf("\nPassenger %d belongs to Train 0\n", myID);
 	printf("Passenger %d is going to get on the Train at stop number %d\n", myID, myTicket.getOnStop);
   	printf("Passenger %d is going to get off the Train at stop number %d\n", myID, myTicket.getOffStop);

	passengersWaitingToBoard[myTicket.getOnStop]++; // increment number of passengers waiting at the stop
	waitingForTrainStopLock[myTicket.getOnStop]->Acquire();	
	
	passengersInitialized++;
	if(passengersInitialized == PASSENGERS)
	{
		setupLock->Acquire();
		setup->Signal(setupLock);
		setupLock->Release();
	}
	waitingForTrainStop[myTicket.getOnStop]->Wait(waitingForTrainStopLock[myTicket.getOnStop]);	// passenger waiting for CV to board
																									// train at their specific stop 
	waitingForTrainStopLock[myTicket.getOnStop]->Release();
	//--END PASSENGER INITIALIZATION
	
	// BEGINNING OF PASSENGER/TICKET CHECKER INTERACTION
	ticketCheckerLock->Acquire();
	
	// if the ticket checker is busy,
	// passenger gets in line
	if(!ticketCheckerAvailable){
		ticketCheckerLineLength++; 
		ticketCheckerLine->Wait(ticketCheckerLock);	// passenger waiting for ticket
														// checker to signal them
		ticketCheckerLineLength--;
	}
	// ticket checker is free so passenger goes up
	else {
		ticketCheckerAvailable = false; // ticket checker is about to be busy with me
	}
	// make my ticket data available to the 
	// Ticket Checker
	ticketToCheck = &myTicket;
	
	// signal and wait for ticket checker to validate my ticket
	ticketChecker->Signal(ticketCheckerLock); 
	ticketChecker->Wait(ticketCheckerLock);
		
	// check to see if i can get in the train	
	if(!myTicket.okToBoard){
		// i had a bad ticket so
		// print msg and return
		printf("Passenger %d of Train %d has an invalid ticket\n", myID,0);
		ticketCheckerLock->Release();
		return;
	}
	//I can get on the train 
	printf("Passenger %d of Train %d has a valid ticket\n", myID,0);

	//increment passenger count
	passengersBoardedLock->Acquire();
	passengersOnTrain++;
	//printf("PON = %d\n",passengersOnTrain);
	passengersBoardedLock->Release();
	//printf("PASSENGER%d ACQUIRING CHOOSE CA\n",myID);
	choosingCoachAttendantLock->Acquire();
	//printf("PASSENGER%d CHOOSING CA\n",myID);
	ticketCheckerLock->Release();	
	//--END PASSENGER/TICKET CHECKER INTERACTION
	
	// BEGINNING OF PASSENGER/COACH ATTENDANT INTERACTION
	
	if(myTicket.firstClass){
		//printf("PASSENGER%d STARTS 1st CA\n",myID);
		// if first class passenger find the coach attendant
		// with the shortest first class line
		int min = 0;
		for(int i = 0; i<COACH_ATTENDANTS; i++)
		{
			if(coachAttendantFirstClassLineLength[i]<coachAttendantFirstClassLineLength[min])
				min = i;	
		}
		myCoachAttendant = min;
		//printf("1st CLASS PASSENGER%d CHOSE CA%d\n",myID,myCoachAttendant);
		coachAttendantLock[myCoachAttendant]->Acquire();
		// increment line count
		coachAttendantFirstClassLineLengthLock[myCoachAttendant]->Acquire();
		coachAttendantFirstClassLineLength[myCoachAttendant]++;
		coachAttendantFirstClassLineLengthLock[myCoachAttendant]->Release(); 
		//printf("P%d RELEASED CHOOSE CA LOCK\n",myID);
		choosingCoachAttendantLock->Release();
		
		// if the coach attendant is available
		// signal to be escorted to your seat
		if(coachAttendantAvailable[myCoachAttendant]){
			//printf("1st CLASS P%d SIGNALS FREE CA%d FOR SEAT\n",myID,myCoachAttendant);
			coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]);
		}
		//printf("1st CLASS P%d WAITING FOR CA%d FOR SEAT\n",myID,myCoachAttendant);
		coachAttendantFirstClassLine[myCoachAttendant]->Wait(coachAttendantLock[myCoachAttendant]);
	}
	else{
		//printf("PASSENGER%d STARTS CA\n",myID);
		// if economy class passenger find the coach attendant
		// with the shortest economy class line
		int min = 0;
		for(int i = 0; i<COACH_ATTENDANTS; i++)
		{
			if(coachAttendantEconomyClassLineLength[i]<coachAttendantEconomyClassLineLength[min])
				min = i;
		}
		myCoachAttendant = min;
		//printf("PASSENGER%d CHOSE CA%d\n",myID,myCoachAttendant);
		coachAttendantLock[myCoachAttendant]->Acquire();
		//increment line count
		coachAttendantEconomyClassLineLengthLock[myCoachAttendant]->Acquire();
		coachAttendantEconomyClassLineLength[myCoachAttendant]++;
		coachAttendantEconomyClassLineLengthLock[myCoachAttendant]->Release();
		//printf("P%d RELEASED CHOOSE CA LOCK\n",myID);
		choosingCoachAttendantLock->Release();
		
		// if the coach attendant is available
		// signal to be escorted to your seat
		if(coachAttendantAvailable[myCoachAttendant]){
			//printf("P%d SIGNALS FREE CA%d FOR SEAT\n",myID,myCoachAttendant);
			coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]);
		}
		//printf("P%d WAITING FOR CA%d FOR SEAT\n",myID,myCoachAttendant);
		coachAttendantEconomyClassLine[myCoachAttendant]->Wait(coachAttendantLock[myCoachAttendant]);
	}
	
	// make my ticket data available 
	// to the coach attendant
	coachAttendantSeatTicket[myCoachAttendant] = &myTicket;
	
	// signal and wait for coach attendant 
	// assign me a seat number 
	//printf("P%d SIGNALS AND WAITS CA%d\n",myID,myCoachAttendant);
	coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]); 
	coachAttendant[myCoachAttendant]->Wait(coachAttendantLock[myCoachAttendant]); 
	coachAttendantLock[myCoachAttendant]->Release();
	
	// if first class, get a 
	// porter before sitting
	if(myTicket.firstClass)
		choosingPorterLock->Acquire(); 
	else{
		//printf("P%d IS ECON CLASS\n",myID);
		printf("Passenger %d of Train %d is given seat number %d by the Coach Attendant %d\n",myID,0,myTicket.seatNumber,myCoachAttendant);
		myTicket.seated = true;
	}
		
	//--END PASSENGER/COACH ATTENDANT INTERACTION
	
	// BEGINNING OF PASSENGER/PORTER INTERACTION FOR FIRST CLASS LUGGAGE
	if(myTicket.firstClass){	
			
		// if first class passenger find the porter
		// with the shortest first class line
		int min = 0;
		for(int i = 0; i<PORTERS; i++)
		{
			if(porterFirstClassLuggageLineLength[i]<porterFirstClassLuggageLineLength[min])
				min = i;	
		}
		myPorter = min;
		//printf("PASSENGER%d CHOSE PORTER %d FOR BEDDING \n",myID,myPorter);
		porterLock[myPorter]->Acquire();
		porterFirstClassLuggageLineLengthLock[myPorter]->Acquire();
		porterFirstClassLuggageLineLength[myPorter]++;
		porterFirstClassLuggageLineLengthLock[myPorter]->Release();
		choosingPorterLock->Release();
		
		// if the porter is availabe signal 
		// and wait to receive bedding 
		if(porterAvailable[myPorter])
			porter[myPorter]->Signal(porterLock[myPorter]);
		// make my ticket data available
		porterLuggageTicket[myPorter] = &myTicket;
		//printf("\nPASSENGER %d REQUESTS LUGGAGE SERVICE\n",myID);
		porterFirstClassLuggageLine[myPorter]->Wait(porterLock[myPorter]);
			
		// signal and wait for porter
		// to serve me
		//printf("P%d SIGNALS AND WAITS FOR PORTER\n",myID);
		porter[myPorter]->Signal(porterLock[myPorter]);
		porter[myPorter]->Wait(porterLock[myPorter]);
		printf("Passenger %d of Train %d is given seat number %d by the Coach Attendant %d\n",myID,0,myTicket.seatNumber,myCoachAttendant);
		printf("1st class Passenger %d of Train %d is served by Porter %d\n",myID,0,myPorter);
		porterLock[myPorter]->Release();
	}
	//--END PASSENGER/PORTER INTERACTION FOR FIRST CLASS LUGGAGE
			
	// BEGINNING OF RANDOM HUNGER FOR ECONOMY CLASS 
	while(!myTicket.receivedFood && !myTicket.firstClass){
		//printf("P%d BEGINS RANDOM HUNGER\n",myID);
		if(rand()%4==0){
			//printf("PASSENGER%d ACQUIRING STEWARD LOCK\n",myID);
			stewardLock->Acquire();
			
			// if the steward is busy,
			// passenger gets in line
			if(!stewardAvailable){
				stewardLineLength++; 
				//printf("PASSENGER%d WAITING IN STEWARD LINE\n",myID);
				stewardLine->Wait(stewardLock);	// passenger waiting for 
													// steward to signal them
				stewardLineLength--;
			}
			// steward is waiting for passengers
			// so wake him up to check the line
			else {
				//printf("PASSENGER%d SIGNALS AND WAITS ON FREE STEWARD\n",myID);
				stewardLineLength++;
				steward->Signal(stewardLock);
				stewardLine->Wait(stewardLock);
				stewardLineLength--;
			}
			// make my ticket data available to the 
			// Steward
			stewardTicket = &myTicket;
			//printf("PASSENGER%d SIGNALS AND WAITS ON STEWARD\n",myID);
			// signal and wait for steward to let me 
			// know  if dining car is full or not
			steward->Signal(stewardLock); 
			steward->Wait(stewardLock);
		
			// if dining car is full return to my
			// seat and wait to get hungry again	
			if(diningCarFull){
				printf("Passenger %d of Train %d is informed by Stewart-the Dining Car is full\n",myID,0);
				stewardLock->Release();	
			}
			else{
				printf("Passenger %d of Train %d is informed by Stewart-the Dining Car is not full\n",myID,0);
				stewardLock->Release();
				diningSeatLock->Acquire();
				diningSeatsAvailable--;
				diningSeatLock->Release();
				
				// BEGINNNING OF PASSENGER/WAITER INTERACTION
				// find waiter with shortest line
				choosingWaiterLock->Acquire();
				int min = 0;
				for(int i = 0; i<WAITERS; i++)
				{
					if(waiterLineLength[i]<waiterLineLength[min])
						min = i;	
				}
				myWaiter = min;
				// increment line count
				waiterLock[myWaiter]->Acquire();
				waiterLineLengthLock[myWaiter]->Acquire();
				waiterLineLength[myWaiter]++;
				waiterLineLengthLock[myWaiter]->Release(); 
				//printf("PASSENGER%d SIGNALS STEWARD\n",myID);
				//printf("PASSENGER%d CHOSE WAITER%d\n",myID,myWaiter);
				//printf("P%d RELEASED CHOOSE WAITER LOCK\n",myID);
				choosingWaiterLock->Release();
				
				//printf("PASSENGER%d WAITS ON WAITER%d\n",myID,myWaiter);
				// Let steward know you have 
				// chosen a waiter so that they
				// can be alerted off break 
				stewardLock->Acquire();
				steward->Signal(stewardLock);
				stewardLock->Release();
				waiterLine[myWaiter]->Wait(waiterLock[myWaiter]);
	
				// make my ticket data available 
				// to the waiter
				waiterFoodTicket[myWaiter] = &myTicket;
	
				// signal and wait for coach attendant 
				// assign me a seat number 
				//printf("P%d SIGNALS AND WAITS WAITER%d\n",myID,myWaiter);
				waiter[myWaiter]->Signal(waiterLock[myWaiter]); 
				// wait to receive food
				waiter[myWaiter]->Wait(waiterLock[myWaiter]);
				printf("Passenger %d of Train %d is served by Waiter %d\n",myID,0,myWaiter);
				myTicket.receivedFood = true;
				// pay the waiter 
				waiter[myWaiter]->Signal(waiterLock[myWaiter]); 
				waiterLock[myWaiter]->Release();

				//--END OF Passenger/WAITER INTERACTION
			}

			stewardLock->Release();
		}
		else
			currentThread->Yield();
	}	
	//--END RANDOM HUNGER FOR ECONOMY CLASS 
	
	// BEGINNING OF RANDOM HUNGER FOR FIRST CLASS 
	while(!myTicket.receivedFood && myTicket.firstClass){
		if(rand()%4==0){
			//printf("P%d ACQURING CHOOSE CA LOCK\n",myID);
			choosingCoachAttendantLock->Acquire();
			//printf("P%d ACQURED CHOOSE CA LOCK\n",myID);
			// if first class passenger find the coach attendant
			// with the shortest first class line
			int min = 0;
			for(int i = 0; i<COACH_ATTENDANTS; i++)
			{
				if(coachAttendantFoodLineLength[i]<coachAttendantFoodLineLength[min])
				min = i;	
			}
			myCoachAttendant = min;
			//printf("P%d CHOSE CA%d\n",myID,myCoachAttendant);
			coachAttendantLock[myCoachAttendant]->Acquire();
			coachAttendantFoodLineLengthLock[myCoachAttendant]->Acquire();
			coachAttendantFoodLineLength[myCoachAttendant]++;
			coachAttendantFoodLineLengthLock[myCoachAttendant]->Release();
			//printf("P%d RELEASED CHOOSE CA LOCK\n",myID);
			choosingCoachAttendantLock->Release();
	
			// if the coach attendant is available
			// signal to give your order to coach attendant 
			if(coachAttendantAvailable[myCoachAttendant]){
				coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]);
				//printf("PASSENGER%d WAKES UP CA%d\n",myID,myCoachAttendant);
			}
			// signal and wait for coach attendant 
			// to take my food order 
			//printf("PASSENGER%d WAITS IN CA%d FOOD LINE\n",myID,myCoachAttendant); 
			printf("Passenger %d of Train %d gives food order to  Coach Attendant %d\n",myID,0,myCoachAttendant);
			// signal and wait until i receive my food order
			coachAttendantFoodLine[myCoachAttendant]->Wait(coachAttendantLock[myCoachAttendant]); 
			// make my ticket data available
			coachAttendantFoodTicket[myCoachAttendant] = &myTicket;
			//printf("PASSENGER%d SIGNALS AND WAITS CA%d\n",myID,myCoachAttendant);
			coachAttendant[myCoachAttendant]->Signal(coachAttendantLock[myCoachAttendant]);
			coachAttendant[myCoachAttendant]->Wait(coachAttendantLock[myCoachAttendant]);
			myTicket.receivedFood = true;
			coachAttendantLock[myCoachAttendant]->Release();	
			//printf("P%d RECEIVED FOOD FROM CA%d\n",myID,myCoachAttendant);	
		}
		else
			currentThread->Yield();
	}
	//--END RANDOM HUNGER FOR FIRST CLASS 


	//printf("P%d BEFOR BEDDING WHILE RECEIVED BEDDING =%d\n",myID,myTicket.receivedBedding);	
	// BEGINNING OF RANDOM BEDDING 
	while(!myTicket.receivedBedding){
	//printf("P%d BEGINS RANDOM BEDDING\n",myID);
		if(rand()%4==0){
			choosingPorterLock->Acquire();
			
			// if first class passenger find the porter
			// with the shortest first class line
			int min = 0;
			for(int i = 0; i<PORTERS; i++)
			{
				if(porterBeddingLineLength[i]<porterBeddingLineLength[min])
					min = i;	
			}
			myPorter = min;
			//printf("PASSENGER%d CHOSE PORTER %d FOR BEDDING \n",myID,myPorter);
			porterLock[myPorter]->Acquire();
			porterBeddingLineLengthLock[myPorter]->Acquire();
			porterBeddingLineLength[myPorter]++;
			porterBeddingLineLengthLock[myPorter]->Release();
			choosingPorterLock->Release();
		
			// if the porter is availabe signal 
			// and wait to receive bedding 
			if(porterAvailable[myPorter])
				porter[myPorter]->Signal(porterLock[myPorter]);
			// make my ticket data available
			porterBeddingTicket[myPorter] = &myTicket;
			printf("Passenger %d calls for bedding\n",myID);
			porterBeddingLine[myPorter]->Wait(porterLock[myPorter]);
				
			// signal and wait for porter
			// to serve me
			//printf("P%d SIGNALS AND WAITS FOR PORTER\n",myID);
			porter[myPorter]->Signal(porterLock[myPorter]);
			porter[myPorter]->Wait(porterLock[myPorter]);
			if(myTicket.firstClass)
				printf("1st class Passenger %d of Train %d is served by Porter %d\n",myID,0,myPorter);
			else
				printf("Economy class Passenger %d of Train %d is served by Porter %d\n",myID,0,myPorter);
			myTicket.receivedBedding = true;
			porterLock[myPorter]->Release();
		}
		else
			currentThread->Yield();
	}
	
	//--END RANDOM BEDDING 
	
	// BEGINNING OF PASSENGER GETTING OFF
	if(currentStop<myTicket.getOffStop){
		// if running test 5, intentionally make all
		// passengers get off at least one stop late
		if(TEST_5&&myTicket.getOffStop!=MAX_STOPS-1){
			waitingForTrainStopLock[myTicket.getOffStop+1]->Acquire();
			waitingForTrainStop[myTicket.getOffStop+1]->Wait(waitingForTrainStopLock[myTicket.getOffStop+1]);
			waitingForTrainStopLock[myTicket.getOffStop+1]->Release();
		}
		else{
			waitingForTrainStopLock[myTicket.getOffStop]->Acquire();
			waitingForTrainStop[myTicket.getOffStop]->Wait(waitingForTrainStopLock[myTicket.getOffStop]);
			waitingForTrainStopLock[myTicket.getOffStop]->Release();	
		}
	}
	
	// increment passengers waiting to get off
	conductorLock->Acquire();
	passengersWaitingToGetOffLock->Acquire();
	passengersWaitingToGetOff++;
	//printf("P-ON=%d P-WAITING=%d\n",passengersOnTrain,passengersWaitingToGetOff);
	passengersWaitingToGetOffLock->Release();
	
	// wait for conductor to let you off
	//printf("PASSENGER%d ACQUIRING CONDUCTOR LOCK\n",myID);
	passengerGotOff[myID]=true;
	//printf("PASSENGER%d WAITING FOR CONDUCTOR\n",myID);
	waitingToGetOff->Wait(conductorLock);
	conductorLock->Release();
	
	// calculate penalty fee if you 
	// didn't get off at your stop
	if(currentStop!=myTicket.getOffStop){
		ticketRevenueLock->Acquire();
		ticketRevenue+=1;
		ticketRevenueLock->Release();
	}
		
	//decrement passenger counts
	passengersWaitingToGetOffLock->Acquire();
	passengersWaitingToGetOff--;
	passengersWaitingToGetOffLock->Release();
	passengersBoardedLock->Acquire();
	passengersOnTrain--;
	//printf("P-ON=%d P-WAITING=%d\n",passengersOnTrain,passengersWaitingToGetOff);
	passengersBoardedLock->Release();
	
	//printf("%d OFF\n",myID);
	if(currentStop!=MAX_STOPS)
		printf("Passenger %d of Train %d is getting off the Train at stop number %d\n",myID,0,currentStop);
	else
		printf("Passenger %d of Train %d is getting off the Train at stop number %d\n",myID,0,currentStop-1);
	// passenger lets conductor
	// know that he/she got off
	conductorLock->Acquire();
	waitingToGetOff->Signal(conductorLock);
	conductorLock->Release();
	 
	//--END PASSENGER GETTING OFF
	
}//end of passenger
		 
void TicketChecker(int number){
	const unsigned int myID = number;

	while(simulationRunning){
		// wait to arrive at a station
		ticketCheckerLock->Acquire();
		//printf("ticket checker waits for next station arrival\n");
		waitingToArriveAtStation->Wait(ticketCheckerLock);
		//printf("ticket checker signals conductor\n");
		waitingToArriveAtStation->Signal(ticketCheckerLock);

		// start of Ticket Checker/Passenger interaction -
		// wakes up all passengers waiting at the current stop
		//printf("ticket checker acquiring train stop lock\n");
		waitingForTrainStopLock[currentStop]->Acquire();
		//printf("ticket checker broadcasts station arrival\n");
		waitingForTrainStop[currentStop]->Broadcast(waitingForTrainStopLock[currentStop]);
		waitingForTrainStopLock[currentStop]->Release();		 
		ticketCheckerAvailable=true; // no passengers in line yet so 
											// ticket checker is free
		// reset passengers checked
		passengersChecked=0;
		
		// check all passengers tickets at current stop
		for(int i=0; i< passengersWaitingToBoard[currentStop]; i++){
			//wait for passengers to come up and give me their ticket
			ticketChecker->Wait(ticketCheckerLock);
			// validate the ticket i have just received

			// ticket was valid
			if(ticketToCheck->getOnStop==currentStop&&!TEST_1){
				ticketToCheck->okToBoard=true;
				printf("Ticket Checker validates the ticket of passenger %d of Train %d at stop number %d\n", ticketToCheck->holder,0,currentStop);
			}
			// ticket was invalid
			else {
				ticketToCheck->okToBoard=false;
				printf("Ticket Checker invalidates the ticket of passenger %d of Train %d at stop number %d\n", ticketToCheck->holder,0,currentStop);
			}
			// increment passengers checked
			passengersChecked++;
			// calculate ticket revenue
			ticketRevenueLock->Acquire();
			ticketRevenue+=ticketToCheck->price;
			ticketRevenueLock->Release();
			// let the person know their ticket has been checked
			ticketChecker->Signal(ticketCheckerLock);
			// if anyone is in line signal them, 
			// otherwise ticket checker is free
			if(ticketCheckerLineLength>0)
				ticketCheckerLine->Signal(ticketCheckerLock);	
			else
				ticketCheckerAvailable=true;
		}
		ticketCheckerLock->Release();
	}				
}//end of ticketChecker

void CoachAttendant(int number){
	const unsigned int myID = number;
	int myChef = -1; // use for orders
	int myStop = 0; // the stop i am working
				     // to make sure up to date
				     // with current stop
	while(simulationRunning){
		
		while(passengersSeated<passengersWaitingToBoard[myStop]&&!(TEST_2||TEST_1)){
			
			coachAttendantLock[myID]->Acquire();
			//printf("CA%d FIRST LINE=%d ECON LINE= %d\n",myID,coachAttendantFirstClassLineLength[myID],coachAttendantEconomyClassLineLength[myID]);
			// handle first class passengers first
			if(coachAttendantFirstClassLineLength[myID]>0){
				//printf("CA%d ASSIGNING A 1st CLASS SEAT\n",myID);
				// if there are first class passengers 
				// in the line signal and wait
				//printf("CA%d SIGNALS AND WAITS 1st CLASS \n",myID);
				coachAttendantFirstClassLine[myID]->Signal(coachAttendantLock[myID]);
				coachAttendant[myID]->Wait(coachAttendantLock[myID]);
				// assign the passenger a seat number
				seatLock->Acquire();
				coachAttendantSeatTicket[myID]->seatNumber = nextSeat;
				nextSeat++;
				seatLock->Release();
				printf("Coach Attendant %d of Train %d gives seat number %d to Passenger %d\n",myID, 0, coachAttendantSeatTicket[myID]->seatNumber,coachAttendantSeatTicket[myID]->holder);
				// decrement line count
				coachAttendantFirstClassLineLengthLock[myID]->Acquire();
				coachAttendantFirstClassLineLength[myID]--;
				coachAttendantFirstClassLineLengthLock[myID]->Release();
				// escort the passenger to their seat
				//printf("CA%d SIGNALS AND RELEASES 1st CLASS \n",myID);
				coachAttendant[myID]->Signal(coachAttendantLock[myID]);
				// increment passengers seated
				passengersSeatedLock->Acquire();
				passengersSeated++;
				passengersSeatedLock->Release();
				coachAttendantLock[myID]->Release();
			}
			else if(coachAttendantEconomyClassLineLength[myID]>0){
				//printf("CA%d ASSIGNING A SEAT\n",myID);
				// if there are economy class passengers 
				// in the line signal and wait
				//printf("CA%d SIGNALS AND WAITS\n",myID);
				coachAttendantEconomyClassLine[myID]->Signal(coachAttendantLock[myID]);
				coachAttendant[myID]->Wait(coachAttendantLock[myID]);
				// assign the passenger a seat number
				seatLock->Acquire();
				coachAttendantSeatTicket[myID]->seatNumber = nextSeat;
				nextSeat++;
				seatLock->Release();
				printf("Coach Attendant %d of Train %d gives seat number %d to Passenger %d\n",myID, 0, coachAttendantSeatTicket[myID]->seatNumber,coachAttendantSeatTicket[myID]->holder);
				// decrement line count
				coachAttendantEconomyClassLineLengthLock[myID]->Acquire();
				coachAttendantEconomyClassLineLength[myID]--;
				coachAttendantEconomyClassLineLengthLock[myID]->Release();
				// escort the passenger to their seat
				//printf("CA%d SIGNALS AND RELEASES\n",myID);
				coachAttendant[myID]->Signal(coachAttendantLock[myID]);
				// increment passengers seated
				passengersSeatedLock->Acquire();
				passengersSeated++;
				passengersSeatedLock->Release();
				coachAttendantLock[myID]->Release();
			}
			else {
				if(myStop!=currentStop||currentStop==MAX_STOPS){
					coachAttendantLock[myID]->Release();
					break;
				}	
				// if the lines are empty, set
				// availability to true and wait
				//printf("CA%d LINES EMPTY \n",myID);
				coachAttendantAvailable[myID]=true;
				coachAttendant[myID]->Wait(coachAttendantLock[myID]);
				//printf("CA%d LINES AWAKE\n",myID);
				coachAttendantAvailable[myID]=false;
				coachAttendantLock[myID]->Release();
			}	
		}
		
		// handle first class food orders	
		//printf("CA%d FOOD LINE=%d\n",myID,coachAttendantFoodLineLength[myID]);
		while(coachAttendantFoodLineLength[myID]>0){
			
			coachAttendantLock[myID]->Acquire();
			// find the chef with the
			// shortest first class line
			//printf("CA%d ACQUIRING CHOOSE CHEF LOCK\n",myID);
			choosingChefLock->Acquire();
			//printf("CA%d ACQUIRES CHOOSE CHEF LOCK\n",myID);
			int min = 0;
			for(int i = 0; i<CHEFS; i++)
			{
			if(chefFirstClassLineLength[i]<chefFirstClassLineLength[min])
				min = i;	
			}
			myChef = min;
			// if the chef i chose is waiting to reload
			// at the next stop, break and do other things
			//printf("CA%d ACQUIRING CHEF LOCK\n",myID);
			chefLock[myChef]->Acquire();
			//printf("CA%d ACQUIRES CHEF LOCK\n",myID);
			if(chefReloadingInventoryAtNextStop[myChef]){
				choosingChefLock->Release();
				//printf("CA%d RELEASES CHOOSE CHEF LOCK BECAUSE CHEF RELOADING\n",myID);
				chefLock[myChef]->Release();
				coachAttendantLock[myID]->Release();
				break;
			}
			//printf("CA%d CHOSE CHEF%d\n",myID,myChef);
			// increment line count
			chefFirstClassLineLengthLock[myChef]->Acquire();
			chefFirstClassLineLength[myChef]++;
			chefFirstClassLineLengthLock[myChef]->Release();
			//printf("CA%d RELEASES CHOOSE CHEF LOCK\n",myID);
			choosingChefLock->Release();
			
			// if there are passengers in line 
			// for food in the line signal and wait
			coachAttendantFoodLine[myID]->Signal(coachAttendantLock[myID]);
			//printf("CA%d SIGNALS FOOD LINE AND WAITS\n",myID);
			coachAttendant[myID]->Wait(coachAttendantLock[myID]);
			// take passenger's food order
			printf("Coach Attendant %d of Train %d takes food order of 1st class Passenger %d\n",myID,0,coachAttendantFoodTicket[myID]->holder);
					
			// BEGINNING COACH ATTENDANT/CHEF INTERACTION
	
			// if the chef is available signal
			// to have hime receive your order
			if(chefAvailable[myChef]){
				//printf("CA%d SIGNALS FREE CHEF%d\n",myID,myChef);
				chef[myChef]->Signal(chefLock[myChef]);
			}
			//printf("CA%d WAITING FOR COOK%d TO RECEIVE ORDER\n",myID,myChef);
			chefFirstClassLine[myChef]->Wait(chefLock[myChef]);
	
			// signal and wait for coach attendant 
			// assign me a seat number 
			//printf("CA%d SIGNALS AND WAITS FOR COOK%d\n",myID,myChef);
			chef[myChef]->Signal(chefLock[myChef]); 
			chef[myChef]->Wait(chefLock[myChef]); 
			chefLock[myChef]->Release();	
			//printf("CA%d RELEASES CHEF LOCK\n",myID);	
			//--END COACH ATTENDANT/CHEF INTERACTION		
					
			printf("Coach Attendant %d of Train %d takes food prepared by Chef %d to the 1st class Passenger %d\n",myID,0,myChef,coachAttendantFoodTicket[myID]->holder);
			// decrement line count
			coachAttendantFoodLineLengthLock[myID]->Acquire();
			coachAttendantFoodLineLength[myID]--;
			coachAttendantFoodLineLengthLock[myID]->Release();
			// give passenger their food
			coachAttendant[myID]->Signal(coachAttendantLock[myID]);
			coachAttendantLock[myID]->Release();
			//printf("CA%d SIGNALS AND RELEASES P%d\n",myID,coachAttendantFoodTicket[myID]->holder);	
		}
	
		// signal for departure to next stop		
		//printf("CA%d WAITING FOR DEPARTURE LOCK\n",myID);
		signallingDepartureLock->Acquire();
		 //printf("CA%d myStop=%d currentStop =%d\n",myID,myStop,currentStop);
		 //printf("CA%d firstLine=%d econLine =%d\n",myID,coachAttendantFirstClassLineLength[myID],coachAttendantEconomyClassLineLength[myID]);
		 //printf("CONDUCTORWAITING=%d\n",conductorWaiting);
		 //printf("PASSENGERS WAITING TO BOARD = %d\n",passengersWaitingToBoard[currentStop]);
		if(myStop!=currentStop){
			myStop = currentStop;
			signallingDepartureLock->Release();
			continue;
		}
		else if(conductorWaiting&&(passengersSeated==passengersWaitingToBoard[currentStop]||(passengersWaitingToBoard[currentStop]==passengersChecked&&(TEST_1||TEST_2)))){
			conductorLock->Acquire();
			//printf("CA%d SIGNALS CONDUCTOR TO LEAVE\n",myID);
			waitingToLeaveStation->Signal(conductorLock);
			conductorLock->Release();
			//printf("CA%d WAITS FOR NEXT STATION\n",myID);
			signallingDeparture->Wait(signallingDepartureLock);
			signallingDepartureLock->Release();
			//printf("CA%d WOKEN UP BY CONDUCTOR\n",myID);
		}
		else{
			//printf("CA%d RELEASING DEPARTURE LOCK\n",myID);
			signallingDepartureLock->Release();
			currentThread->Yield();
		}
	}	
}//end of CoachAttendant

void Porter(int number){
	const unsigned int myID = number;
	while(simulationRunning){
		porterLock[myID]->Acquire();
		// handle first class passengers first
		if(porterFirstClassLuggageLineLength[myID]>0){
			// if there are first class passengers 
			// in the line signal and wait
			//printf("porter signalling and waiting on first line\n");
			porterFirstClassLuggageLine[myID]->Signal(porterLock[myID]);
			porter[myID]->Wait(porterLock[myID]);
			printf("Porter %d of Train %d picks up bags of 1st class Passenger %d\n",myID, 0, porterLuggageTicket[myID]->holder);	
			// decrement line count
			porterFirstClassLuggageLineLengthLock[myID]->Acquire();
			porterFirstClassLuggageLineLength[myID]--;
			porterFirstClassLuggageLineLengthLock[myID]->Release();
			// serve the passenger
			//printf("porter signals first line\n");
			porter[myID]->Signal(porterLock[myID]);
			porterLock[myID]->Release();
		}
		else if(porterBeddingLineLength[myID]>0){
			// if there are economy class passengers 
			// in the line signal and wait
			//printf("porter signalling and waiting on econ line\n");
			porterBeddingLine[myID]->Signal(porterLock[myID]);
			porter[myID]->Wait(porterLock[myID]);
			printf("Porter %d of Train %d gives bedding to Passenger %d\n",myID, 0, porterBeddingTicket[myID]->holder);
			// decrement line count
			porterBeddingLineLengthLock[myID]->Acquire();
			porterBeddingLineLength[myID]--;
			porterBeddingLineLengthLock[myID]->Release();
			// serve the passenger
			//printf("porter signals econ line\n");
			porter[myID]->Signal(porterLock[myID]);
			porterLock[myID]->Release();		
		}
		else{
			// if all lines are empty, set
			// availability to true and wait
			porterAvailable[myID]=true;
			porter[myID]->Wait(porterLock[myID]);
			porterAvailable[myID]=false;
			porterLock[myID]->Release();
		}
	}	
	
}//end of Porter

void Chef(int number){

	const unsigned int myID = number;
	
	while(simulationRunning){
		chefLock[myID]->Acquire();
		// if kitchen is dirty clean it
		if(!chefKitchenClean[myID]){
			//printf("CHEF%d CLEANS KITCHEN\n",myID);
			chefKitchenClean[myID]=true;
			// check inventory, if at a
			// minimal level reload at next stop
			if(chefInventory[myID]<=CHEF_INVENTORY_MINIMUM){
				chefReloadingInventoryAtNextStop[myID] = true;
				printf("Chef %d of Train %d calls for more inventory\n",myID,0);
			}
		}
		if(chefInventory[myID]==0){
			// if inventory is empty,
			// wait to reload inventory at 
			// next stop before proceeding
			//printf("CHEF%d WAITS TO RELOAD INVENTORY AT NEXT STOP\n",myID);
			chefReloadingInventoryAtNextStop[myID] = true;
			chefWaitingToReloadInventory[myID]->Wait(chefLock[myID]);
			//printf("CHEF%d RELOADs INVENTORY\n",myID);
		}
		// handle first class passengers first
		if(chefFirstClassLineLength[myID]>0){
			// if there are first class passengers 
			// in the line signal and wait
			//printf("CHEF%d FIRST LINE SIGNALS AND WAITS\n",myID);
			chefFirstClassLine[myID]->Signal(chefLock[myID]);
			chef[myID]->Wait(chefLock[myID]);
			printf("Chef %d of Train %d is preparing food\n",myID, 0);
			// decrement inventory
			chefInventoryLock[myID]->Acquire();
			chefInventory[myID]--;
			//printf("CHEF%d INVENTORY=%d\n",myID,chefInventory[myID]);
			chefInventoryLock[myID]->Release();	
			// escort the passenger to their seat
			//printf("CHEF%d SIGNALS AND RELEASES\n",myID);
			chef[myID]->Signal(chefLock[myID]);
			// decrement line count
			chefFirstClassLineLengthLock[myID]->Acquire();
			chefFirstClassLineLength[myID]--;
			chefFirstClassLineLengthLock[myID]->Release();
			chefLock[myID]->Release();
		}
		else if(chefEconomyClassLineLength[myID]>0){
			// if there are economy class passengers 
			// in the line signal and wait
			//printf("CHEF%d ECON LINE SIGNALS AND WAITS\n",myID);
			chefEconomyClassLine[myID]->Signal(chefLock[myID]);
			chef[myID]->Wait(chefLock[myID]);
			printf("Chef %d of Train %d is preparing food\n",myID, 0);
			// decrement inventory
			chefInventoryLock[myID]->Acquire();
			chefInventory[myID]--;
			chefInventoryLock[myID]->Release();	
			// escort the passenger to their seat
			//printf("CHEF%d SIGNALS AND RELEASES\n",myID);
			chef[myID]->Signal(chefLock[myID]);
			// decrement line count
			chefEconomyClassLineLengthLock[myID]->Acquire();
			chefEconomyClassLineLength[myID]--;
			chefEconomyClassLineLengthLock[myID]->Release();
			chefLock[myID]->Release();
		}
		else{
			printf("Chef %d of Train %d is going for a break\n",myID,0);
			// if all lines are empty, set
			// availability to true and wait
			chefAvailable[myID]=true;
			chef[myID]->Wait(chefLock[myID]);
			printf("Chef %d of Train %d returned from break\n",myID,0);
			chefAvailable[myID]=false;
			chefLock[myID]->Release();
		}
	}	
	
}//end of Chef

void Waiter(int number){
	const unsigned int myID = number;
	int myChef = -1;
	
	while(simulationRunning){
		waiterLock[myID]->Acquire();	
		//printf("WAITER%d LINE=%d\n",myID,waiterLineLength[myID]);
		if(waiterLineLength[myID]>0){
			// find the chef with the
			// shortest economy class line
			//printf("WAITER%d ACQUIRING CHOOSE CHEF LOCK\n",myID);
			choosingChefLock->Acquire();
			//printf("WAITER%d ACQUIRES CHOOSE CHEF LOCK\n",myID);
			int min = 0;
			for(int i = 0; i<CHEFS; i++)
			{
				if(chefEconomyClassLineLength[i]<chefEconomyClassLineLength[min]||(chefReloadingInventoryAtNextStop[min]&&!chefReloadingInventoryAtNextStop[i]))
					min = i;	
			}
			myChef = min;
			
			// if the chef i chose is waiting to reload
			// at the next stop, break and do other things
			//printf("WAITER%d ACQUIRING CHEF LOCK\n",myID);
			chefLock[myChef]->Acquire();
			//printf("WAITER%d ACQUIRES CHEF LOCK\n",myID);
			if(chefReloadingInventoryAtNextStop[myChef]){
				// wake up free coach attendants to signal 
				// departure in case only one coach attendant
				// is active and deadlocks with a waiter in
				// the situation where all cooks are waiting
				// to restock at the next stop
				for(int i=0;i<COACH_ATTENDANTS;i++){
				if(coachAttendantAvailable[i])
					coachAttendant[i]->Signal(coachAttendantLock[i]);
				}
			}
			//printf("WAITER%d CHOSE CHEF%d\n",myID,myChef);
			// increment line count
			chefEconomyClassLineLengthLock[myChef]->Acquire();
			chefEconomyClassLineLength[myChef]++;
			chefEconomyClassLineLengthLock[myChef]->Release();
			choosingChefLock->Release();
			//printf("WAITER%d RELEASES CHOOSE CHEF LOCK\n",myID);
			
			// if there are passengers in line 
			// for food in the line signal and wait
			//printf("SIGNAL WAITER LINE AND WAIT%d\n",myID);
			waiterLine[myID]->Signal(waiterLock[myID]);
			//printf("WAITER%d WAIT\n",myID);
			waiter[myID]->Wait(waiterLock[myID]);
					
			// BEGINNING WAITER/CHEF INTERACTION
	
			// if the chef is available signal
			// to have hime receive your order
			if(chefAvailable[myChef]){
				//printf("WAITER%d SIGNALS FREE CHEF%d\n",myID,myChef);
				chef[myChef]->Signal(chefLock[myChef]);
			}
			//printf("WAITER%d WAITING FOR COOK%d TO RECEIVE ORDER\n",myID,myChef);
			chefEconomyClassLine[myChef]->Wait(chefLock[myChef]);
			// signal and wait for chef 
			// to cook passenger's food
			//printf("WAITER%d SIGNALS AND WAITS FOR COOK%d\n",myID,myChef);
			chef[myChef]->Signal(chefLock[myChef]); 
			chef[myChef]->Wait(chefLock[myChef]); 
			chefLock[myChef]->Release();	
			//printf("WAITER%d RELEASES CHEF LOCK\n",myID);	
			//--END WAITER/CHEF INTERACTION		
			printf("Waiter %d  is serving Passenger %d of Train %d\n",myID,waiterFoodTicket[myID]->holder,0);
			// decrement line count
			waiterLineLengthLock[myID]->Acquire();
			waiterLineLength[myID]--;
			waiterLineLengthLock[myID]->Release();
			// give passenger their food
			waiter[myID]->Signal(waiterLock[myID]);
			waiter[myID]->Wait(waiterLock[myID]);
			// collect passenger's bill
			waiterRevenueLock[myID]->Acquire();
			waiterRevenue[myID]+= FOOD_PRICE;
			waiterRevenueLock[myID]->Release();
			waiterLock[myID]->Release();
			//printf("WAITER%d SIGNALS AND RELEASES P%d\n",myID,waiterFoodTicket[myID]->holder);	
		}
		else{
		    printf("Waiter %d of Train %d is going for a break\n",myID,0);
			// if all lines are empty,set
			// availability to true and wait
			waiterAvailable[myID]=true;
			waiter[myID]->Wait(waiterLock[myID]);
			printf("Waiter %d of Train %d returned from break\n",myID,0);
			waiterAvailable[myID]=false;
			waiterLock[myID]->Release();
		}
	}	
	//printf("WAITER%d IS A BITCH\n",myID);
}//end of Waiter

void Steward(int number){
	const unsigned int myID = number;

	while(simulationRunning){
		//printf("STEWARD ACQUIRES STEWARD LOCK\n");				
		stewardLock->Acquire();
		if(rand()%4==0){
			for(int i=0;i<WAITERS;i++){
				diningCarRevenueLock->Acquire();
				waiterRevenueLock[i]->Acquire();
				diningCarRevenue+= waiterRevenue[i];
				waiterRevenue[i] = 0;
				waiterRevenueLock[i]->Release();
				diningCarRevenueLock->Release();
			}
			
		}
		if(stewardLineLength>0){
			// if there are passengers waiting
			// in the line signal and wait
			
			//printf("STEWARD SIGNALS AND WAITS ON LINE\n");
			stewardLine->Signal(stewardLock);
			steward->Wait(stewardLock);
			
			// see if dining car is full
			if(diningSeatsAvailable==DINING_CAR_SEATS){
				diningCarFull = true;
			}
			else{
				diningSeatLock->Acquire();
				diningSeatsAvailable++;
				diningSeatLock->Release();
			}
			// inform the passenger about whether
			// or not dining car is full
			//printf("STEWARD SIGNALS PASSENGER ABOUT DINING CAR\n");
			steward->Signal(stewardLock);
			if(diningCarFull){
				printf("Steward %d of Train %d informs Passenger %d-the Train is full\n",myID,0,stewardTicket->holder);
				//printf("STEWARD RELEASES STEWARD LOCK\n");	
				stewardLock->Release();
			}
			else{
				printf("Steward %d of Train %d informs Passenger %d-the Train is not full\n",myID,0,stewardTicket->holder);
				//printf("STEWARD WAITING\n");
				steward->Wait(stewardLock);
				//printf("STEWARD AWAKE\n");
				// steward call all waiters 
				// of break to server passengers
				for(int i=0;i<WAITERS;i++){
					if(waiterAvailable[i])
						waiter[i]->Signal(waiterLock[i]);
				}
				printf("Steward calls back all Waiters from break\n");
				stewardLock->Release();
				//printf("STEWARD RELEASES STEWARD LOCK\n");
			}
		}
		else{
			// if the line is empty wait
			stewardAvailable=true;
			//printf("STEWARD LINES ARE EMPTY\n");
			steward->Wait(stewardLock);
			stewardAvailable=false;
			stewardLock->Release();
			//printf("STEWARD RELEASES STEWARD LOCK\n");
		}		
	
	}				
}//end of steward

void Conductor(int number){
	const unsigned int myID = number;
	while(currentStop < MAX_STOPS){
		// conductor yields 25-50 times before 
		// he decides the next stop has arrived	
		for(int i = 0; i<(randomRange(0,26)+25); i++){
			if(rand()%20==0){
				printf("Train %d Dining Car revenue = %d\n",0,diningCarRevenue);
				printf("Train %d Ticket Revenue = %d\n",0,ticketRevenue); 
				printf("Total Train %d Revenue = %d\n\n",0,ticketRevenue+diningCarRevenue);	
			}
			currentThread->Yield();
		}
		//printf("CONDUCTOR ACQUIRING TICKET CHECKER\n");
		ticketCheckerLock->Acquire();
		//printf("CONDUCTOR SIGNALS AND WAITS ONTICKET CHECKER\n");
		// signal and wait on ticket checker to
		// get up for station arrival
		waitingToArriveAtStation->Signal(ticketCheckerLock);
		waitingToArriveAtStation->Wait(ticketCheckerLock);
		
		// reset number of passengers seated
		passengersSeatedLock->Acquire();
		passengersSeated=0;
		passengersSeatedLock->Release();
			
		// Announce arrival
		printf("\nConductor announces exit for Passengers at stop number %d\n", currentStop);
		
		printf("Train %d Dining Car revenue = %d\n",0,diningCarRevenue);
		printf("Train %d Ticket Revenue = %d\n",0,ticketRevenue); 
		printf("Total Train %d Revenue = %d\n",0,ticketRevenue+diningCarRevenue);
		passengersBoardedLock->Acquire();
		printf("Total Passengers in the Train %d = %d\n\n",0,passengersOnTrain);
		passengersBoardedLock->Release();

		//printf("%d PASSENGERS TO BOARD\n",passengersWaitingToBoard[currentStop]);		
		ticketCheckerLock->Release();
	
		// let chefs reload inventory
		// if they are waiting to
		for(int i=0; i<CHEFS;i++){
			if(chefReloadingInventoryAtNextStop[i]){
				chefLock[i]->Acquire();
				chefWaitingToReloadInventory[i]->Signal(chefLock[i]);
				chefInventory[i] = CHEF_INVENTORY_CAPACITY;
				chefReloadingInventoryAtNextStop[i]=false;
				chefLock[i]->Release();
			}		
		}
			
		// wake up the coach attendant
		// who signalled to leave at the
		// last stop
		if(currentStop>0){
			//printf("conductor acquiring departure lock\n");
			signallingDepartureLock->Acquire();
			//printf("signalling signalling coach attendant\n");		
			signallingDeparture->Broadcast(signallingDepartureLock);
			signallingDepartureLock->Release();
		}
	
		// Let passengers who are trying to get 
		// off before leaving exit the train
		if(currentStop!=MAX_STOPS-1){
			//printf("conductor acquiring get of lock\n");
			while(passengersWaitingToGetOff>0){
				//printf("in conductor while\n");
				//printf("acquiring conductor lock\n");
				conductorLock->Acquire();
				//printf("conductor signals and waits someone to get of\n");
				waitingToGetOff->Signal(conductorLock);
				waitingToGetOff->Wait(conductorLock);
				//printf("conductor informed that passenger off\n");
				conductorLock->Release();
			}
		}
					
		// Wait to leave station
		
		conductorLock->Acquire();
		//printf("CONDUCTOR WAITING TO LEAVE\n");
		conductorWaiting = true;
		waitingToLeaveStation->Wait(conductorLock);
		// increment current stop 
		currentStop++; 
		conductorWaiting = false;
		conductorLock->Release();
		//printf("CONDUCTOR LEAVING\n");
			
		// if we are at the last stop wait
		// for all passengers to get off
		if(currentStop==MAX_STOPS){
			//for(int i=0;i<PASSENGERS;i++){
				//if(!passengerGotOff[i])
				//	printf("P%d REMAINS\n",i);
			//}
			// wake up the coach attendant
			// who signalled to leave at the
			// last stop
			if(currentStop>0){
				//printf("conductor acquiring departure lock\n");
				signallingDepartureLock->Acquire();
				//printf("signalling signalling coach attendant\n");		
				signallingDeparture->Broadcast(signallingDepartureLock);
				signallingDepartureLock->Release();
			}
			
			// while there are passengers still
			// on board, wait for them to get off 
			while(passengersOnTrain>0){	
				// let chefs reload inventory
				// if they are waiting to
				for(int i=0; i<CHEFS;i++){
					if(chefReloadingInventoryAtNextStop[i]){
						chefLock[i]->Acquire();
						chefWaitingToReloadInventory[i]->Signal(chefLock[i]);
						chefInventory[i] = CHEF_INVENTORY_CAPACITY;
						chefReloadingInventoryAtNextStop[i]=false;
						chefLock[i]->Release();
					}		
				}
				// if running test 2, no coach
				// attendants or seated passengers
				// so just break;
				if(TEST_2)
					break;	
				// wake up free coach attendants to 
				// keep checking for passengers to help
				for(int i=0;i<COACH_ATTENDANTS;i++){
				if(coachAttendantAvailable[i])
					coachAttendant[i]->Signal(coachAttendantLock[i]);
				}
				// if all remaining passengers are waiting
				// to get off, broadcast to them to get off	
				//printf("P-ON=%d P-WAITING=%d\n",passengersOnTrain,passengersWaitingToGetOff);	
				if(passengersWaitingToGetOff==passengersOnTrain){
					conductorLock->Acquire();
					waitingToGetOff->Broadcast(conductorLock);
					waitingToGetOff->Wait(conductorLock);
					conductorLock->Release();
					currentThread->Yield();
				}
				else
					currentThread->Yield();
			}
		}
			
		// cleared to move to next stop
		// print number of passengers onboard
		printf("\nTrain %d Dining Car revenue = %d\n",0,diningCarRevenue);
		printf("Train %d Ticket Revenue = %d\n",0,ticketRevenue); 
		printf("Total Train %d Revenue = %d\n",0,ticketRevenue+diningCarRevenue);
		passengersBoardedLock->Acquire();
		printf("Total Passengers in the Train %d = %d\n\n",0,passengersOnTrain);
		passengersBoardedLock->Release();
			
		// if we are at the last stop
		// break
		if(currentStop==MAX_STOPS)
			break;
	}
	simulationRunning = false;
	//printf("\nSIMULATION END\n");
}//end of conductor


void simulation(){
	// Print statements needed before the start of simulation
	printf("\nNumber of Coach Attendants per Train = %d\n", COACH_ATTENDANTS);
	printf("Number of Trains per Train = %d\n",1);
	printf("Number of Trains = %d\n", TRAINS);
	printf("Number of Chefs per Train = %d\n", CHEFS);
	printf("Number of Waiters per Train = %d\n", WAITERS);
	printf("Number of Porters per Train = %d\n", PORTERS);
	printf("Total number of passengers = %d\n\n", PASSENGERS);
	printf("Number of passengers for Train %d = %d\n", 0, PASSENGERS);
	
	// lock for atomic setup steps
	setupLock->Acquire();
	
	//setup bullhockey
	char *name;
	char buffer[40];
	for(int i=0; i<MAX_STOPS; i++){
		sprintf(buffer, "%d", i);
		name = new char[40];
		waitingForTrainStop[i]=new Condition(strcat(strcpy(name,"waitingForTrainStopCV"),buffer));
		name = new char[40];
		waitingForTrainStopLock[i]=new Lock(strcat(strcpy(name,"waitingForTrainStopLock"),buffer));
	}
	
	// initialize coach attendant variables
	for(int i=0; i<COACH_ATTENDANTS; i++){
		sprintf(buffer, "%d", i);	
		coachAttendantFirstClassLineLength[i] = 0;
		coachAttendantEconomyClassLineLength[i] = 0;
		coachAttendantAvailable[i] = false;
		coachAttendantFoodLineLength[i] = 0;
		name = new char[50];
		coachAttendant[i] = new Condition(strcat(strcpy(name,"coachAttendantCV"),buffer));
		name = new char[50];
		coachAttendantFirstClassLine[i] = new Condition(strcat(strcpy(name,"coachAttendantFirstClassLineCV"),buffer));
		name = new char[50];
		coachAttendantEconomyClassLine[i] = new Condition(strcat(strcpy(name,"coachAttendantEconomyClassLineCV"),buffer));
		name = new char[50];
		coachAttendantFoodLine[i] = new Condition(strcat(strcpy(name,"coachAttendantFoodLineCV"),buffer));
		name = new char[50];
		coachAttendantLock[i] = new Lock(strcat(strcpy(name,"coachAttendantLock"),buffer));
		name = new char[50];
		coachAttendantFirstClassLineLengthLock[i] = new Lock(strcat(strcpy(name,"coachAttendantFirstClassLineLengthLock"),buffer));
		name = new char[50];
		coachAttendantEconomyClassLineLengthLock[i] = new Lock(strcat(strcpy(name,"coachAttendantEconomyClassLineLengthLock"),buffer));
		name = new char[50];
		coachAttendantFoodLineLengthLock[i] = new Lock(strcat(strcpy(name,"coachAttendantFoodLineLengthLock"),buffer)); 
	}

	// initialize porter variables
	for(int i=0; i<PORTERS; i++){
		sprintf(buffer, "%d", i);	
		porterFirstClassLuggageLineLength[i] = 0;
		porterBeddingLineLength[i] = 0;
		porterAvailable[i] = false;
		name = new char[50];
		porter[i] = new Condition(strcat(strcpy(name,"porterCV"),buffer));
		name = new char[50];
		porterFirstClassLuggageLine[i] = new Condition(strcat(strcpy(name,"porterFirstClassLuggageLineCV"),buffer));
		name = new char[50];
		porterBeddingLine[i] = new Condition(strcat(strcpy(name,"porterBeddingLineCV"),buffer));
		name = new char[50];
		porterLock[i] = new Lock(strcat(strcpy(name,"porterLock"),buffer));
		name = new char[50];
		porterFirstClassLuggageLineLengthLock[i] = new Lock(strcat(strcpy(name,"porterFirstClassLuggageLineLengthLock"),buffer));
		name = new char[50];
		porterBeddingLineLengthLock[i] = new Lock(strcat(strcpy(name,"porterBeddingLineLengthLock"),buffer));
	}

	// initialize chef variables
	for(int i=0; i<CHEFS; i++){
		sprintf(buffer, "%d", i);	
		chefFirstClassLineLength[i] = 0;
		chefEconomyClassLineLength[i] = 0;
		chefInventory[i] = CHEF_INVENTORY_CAPACITY;
		chefAvailable[i] = false;
		chefKitchenClean[i] = false;
		chefReloadingInventoryAtNextStop[i] = false;
		name = new char[50];
		chef[i] = new Condition(strcat(strcpy(name,"chefCV"),buffer));
		name = new char[50];
		chefFirstClassLine[i] = new Condition(strcat(strcpy(name,"chefFirstClassLineCV"),buffer));
		name = new char[50];
		chefEconomyClassLine[i] = new Condition(strcat(strcpy(name,"chefEconomyClassLineCV"),buffer));
		name = new char[50];
		chefWaitingToReloadInventory[i] = new Condition(strcat(strcpy(name,"chefWaitingToReloadInventoryCV"),buffer));
		name = new char[50];
		chefLock[i] = new Lock(strcat(strcpy(name,"chefLock"),buffer));
		name = new char[50];
		chefFirstClassLineLengthLock[i] = new Lock(strcat(strcpy(name,"chefFirstClassLineLengthLock"),buffer));
		name = new char[50];
		chefEconomyClassLineLengthLock[i] = new Lock(strcat(strcpy(name,"chefEconomyClassLineLengthLock"),buffer));
		name = new char[50];
		chefInventoryLock[i] = new Lock(strcat(strcpy(name,"chefInventoryLock"),buffer));
	}

	// initialize waiter variables
	for(int i=0; i<WAITERS; i++){
		sprintf(buffer, "%d", i);	
		waiterLineLength[i] = 0;
		waiterAvailable[i] = false;
		waiterRevenue[i] = 0;
		name = new char[50];
		waiter[i] = new Condition(strcat(strcpy(name,"waiterCV"),buffer));
		name = new char[50];
		waiterLine[i] = new Condition(strcat(strcpy(name,"waiterLineCV"),buffer));
		name = new char[50];
		waiterLock[i] = new Lock(strcat(strcpy(name,"waiterLock"),buffer));
		name = new char[50];
		waiterLineLengthLock[i] = new Lock(strcat(strcpy(name,"waiterLineLengthLock"),buffer));
		name = new char[50];
		waiterRevenueLock[i] = new Lock(strcat(strcpy(name,"waiterRevenueLock"),buffer));
	}

	Thread* t;
	char *pbuff;
	// create passenger threads and fork them
	for(int i=0; i<PASSENGERS; i++){
		passengerGotOff[i] = false;
		pbuff=new char[20];
		sprintf(pbuff, "Passenger%d", i);
		t=new Thread(pbuff);
		t->Fork((VoidFunctionPtr)Passenger, i);
		
	}
	
	// create coach attendants threads and fork them
	for(int i=0; i<COACH_ATTENDANTS; i++){
		pbuff=new char[20];
		sprintf(pbuff, "CoachAttendant%d", i);
		t=new Thread(pbuff);
		t->Fork((VoidFunctionPtr)CoachAttendant, i);
	}	
	
	// create porter threads and fork them
	for(int i=0; i<PORTERS; i++){
		pbuff=new char[20];
		sprintf(pbuff, "Porter%d", i);
		t=new Thread(pbuff);
		t->Fork((VoidFunctionPtr)Porter, i);
	}	
	
	// create chef threads and fork them
	for(int i=0; i<CHEFS; i++){
		pbuff=new char[20];
		sprintf(pbuff, "Chef%d", i);
		t=new Thread(pbuff);
		t->Fork((VoidFunctionPtr)Chef, i);
	}	

	// create waiter threads and fork them
	for(int i=0; i<WAITERS; i++){
		pbuff=new char[20];
		sprintf(pbuff, "Waiter%d", i);
		t=new Thread(pbuff);
		t->Fork((VoidFunctionPtr)Waiter, i);
	}	
	
	setup->Wait(setupLock);
	setupLock->Release();
	//--end of setup
	
	// simulation start
	t=new Thread("steward");
	t->Fork((VoidFunctionPtr)Steward, 1);
	
	t=new Thread("Conductor");
	t->Fork((VoidFunctionPtr)Conductor, 1);
	
	t=new Thread("ticketChecker");
	t->Fork((VoidFunctionPtr)TicketChecker, 1);

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
//      1. Ticket Checker does not allow the 
//              Passengers to be in the Train 
//              who have invalid ticket. 
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
//      2. Passengers are not allowed to take seat 
//              on their own. They must wait for 
//              the Coach Attendant. No 2 Passengers
//              are given same seat number.
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
//      3. Passengers have to go back to their seat 
//              if the Dining Car is full and they 
//              should come back at some random time 
//              until they have been allowed to stay. 
///////////////////////////////////////////////////
void Test3(){
		#undef DINING_CAR_SEATS
        #define DINING_CAR_SEATS 1
        TEST_3 = true;
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
//      4. Chefs are to maintain the track of food 
//              inventory. If less than minimal, 
//              food is to be loaded on the next 
//              train stop. 
///////////////////////////////////////////////////
void Test4(){
        for(int i=0;i<CHEFS;i++){
        	chefInventory[i] = CHEF_INVENTORY_MINIMUM;
        }
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
//      5. If passengers get off at their stop
//              they print a message and pay
//              a fine.
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
//      2. No 2 Passengers are given same seat number.
//      5. Steward wakes up Waiters when there are 
//              Passengers waiting in Dining Car. 
//      6. 1st class Passengers order food from their room. 
//      7. All Passengers must have bedding after 
//              having food, before getting off the 
//              Train at their stop. 
//      8. Coach Attendants and Conductor should keep 
//              track of number of Passengers at every 
//              stop of train. 
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

	
	



	

	
	
	























