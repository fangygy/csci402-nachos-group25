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
int ticketToCheckHolder, ticketToCheckGetOnStop, ticketToCheckOkToBoard;
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
int passengerTicketsHolder[COACH_ATTENDANTS];
int passengerTicketsSeatNumber[COACH_ATTENDANTS];
int foodOrdersWaiting[COACH_ATTENDANTS];
int CAWaitingForPorter[COACH_ATTENDANTS];
int CAWaitingForPorterLock[COACH_ATTENDANTS];
int coachAttendantFoodLineLength[COACH_ATTENDANTS];
int coachAttendantFoodLine[COACH_ATTENDANTS];
int coachAttendantGetFood[COACH_ATTENDANTS];
int coachAttendantGetFoodLock[COACH_ATTENDANTS];
int coachAttendantOrderHolder[COACH_ATTENDANTS];
int coachAttendantOrderFoodOrder[COACH_ATTENDANTS];
int coachAttendantOrderValidFoodOrder[COACH_ATTENDANTS];
/*--end CoachAttendant/Passenger interactions*/

/*data for Porter/Passenger interactions*/
int porterBaggageLineLength[PORTERS];
int porterAvailable[PORTERS];
int porter[PORTERS];
int porterBaggageLine[PORTERS];
int porterLine[PORTERS];
int porterLock[PORTERS];
int porterBaggageLineLock[PORTERS];
int passengerBaggage[PORTERS];
int porterBeddingLineLength[PORTERS];
int porterFirstClassBeddingLineLength[PORTERS];
int porterFirstClassBeddingLine[PORTERS];
int porterBeddingLine[PORTERS];
int porterBeddingLineLock[PORTERS];
int passengerBeddingHolder[PORTERS];
int passengerBeddingHasBedding[PORTERS];
/*--end Porter/Passenger data*/

/*data for Waiter/Passenger interactions*/
int waiterLineLength[WAITERS];
int passengerFoodHolder[WAITERS];
int passengerFoodFoodOrder[WAITERS];
int passengerFoodValidFoodOrder[WAITERS];
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
int stewardTicketToCheckHolder;
int stewardTicketToCheckWaiter;
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

Ticket tickets[PASSENGERS];

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

void createVars(){
	int i;
	char * name;
	
	passengersFinished = CreateMV("passengersFinished",-1);
	passengersEaten = CreateMV("passengersEaten",-1);
	passengersSlept = CreateMV("passengersSlept",-1);

	passengerID = CreateMV("passengerID",-1);
	coachAttendantID = CreateMV("coachAttendantID",-1);
	porterID = CreateMV("porterID",-1);
	waiterID = CreateMV("waiterID",-1);
	chefID = CreateMV("chefID",-1);

	totalRevenue=CreateMV("totalRevenue",-1);
	ticketRevenue=CreateMV("ticketRevenue",-1);
	diningCarRevenue=CreateMV("diningCarRevenue",-1);
	waiterRevenue=CreateMV("waiterRevenue",-1);

	currentStop = CreateMV("currentStop",-1);
	passengersOnTrain = CreateMV("passengersOnTrain",-1);

	for(i = 0; i < 4; i++){
		name = "foodStock  ";
        	name[9] = (char)(i/10+48);
        	name[10] = (char)(i%10+48);
		foodStock[i] = CreateMV(name,-1);
		
		name = "foodStockLow  ";
        	name[12] = (char)(i/10+48);
        	name[13] = (char)(i%10+48);
		foodStockLow[i] = CreateMV(name,-1);
	}

	nowStopped=CreateMV("nowStopped",-1);
	

	for(i = 0; i < PASSENGERS; i++){
		name = "getOnStop  ";
		name[9] = (char)(i/10+48);
		name[10] = (char)(i%10+48);
		tickets[i].getOnStop = CreateMV(name,-1);
		name = "getOffStop  ";
		name[10] = (char)(i/10+48);
		name[11] = (char)(i%10+48);
		tickets[i].getOffStop = CreateMV(name,-1);
		name = "holder  ";
		name[6] = (char)(i/10+48);
		name[7] = (char)(i%10+48);
		tickets[i].holder = CreateMV(name,-1);
		name = "okToBoard  ";
		name[9] = (char)(i/10+48);
		name[10] = (char)(i%10+48);
		tickets[i].okToBoard = CreateMV(name,-1);
		name = "seatNumber  ";
		name[10] = (char)(i/10+48);
		name[11] = (char)(i%10+48);
		tickets[i].seatNumber = CreateMV(name,-1);
		name = "hasBedding  ";
		name[10] = (char)(i/10+48);
		name[11] = (char)(i%10+48);
		tickets[i].hasBedding = CreateMV(name,-1);
		name = "hasEaten  ";
		name[8] = (char)(i/10+48);
		name[9] = (char)(i%10+48);
		tickets[i].hasEaten = CreateMV(name,-1);
		name = "hasSlept  ";
		name[8] = (char)(i/10+48);
		name[9] = (char)(i%10+48);
		tickets[i].hasSlept = CreateMV(name,-1);
		name = "foodOrder  ";
		name[9] = (char)(i/10+48);
		name[10] = (char)(i%10+48);
		tickets[i].foodOrder = CreateMV(name,-1);
		name = "validFoodOrder  ";
		name[14] = (char)(i/10+48);
		name[15] = (char)(i%10+48);
		tickets[i].validFoodOrder = CreateMV(name,-1);
		name = "waiter  ";
		name[6] = (char)(i/10+48);
		name[7] = (char)(i%10+48);
		tickets[i].waiter = CreateMV(name,-1);
	}

	/* data for passenger/ticketChecker interactions*/
	for(i = 0; i < MAX_STOPS; i++){
		name = "passengersWaitingToBoard  ";
        	name[24] = (char)(i/10+48);
        	name[25] = (char)(i%10+48);
		passengersWaitingToBoard[i] = CreateMV(name,-1);
	}
	ticketCheckerLineLength = CreateMV("ticketCheckerLineLength",-1);
	ticketCheckerAvailable = CreateMV("ticketCheckerAvailable",-1);
	ticketToCheckHolder = CreateMV("ticketToCheckHolder",-1);
	ticketToCheckGetOnStop = CreateMV("ticketToCheckGetOnStop",-1);
	ticketToCheckOkToBoard = CreateMV("ticketToCheckOkToBoard",-1);
	/*--end passenger/ticketChecker data*/

	/*data for CoachAttendant/Passenger interactions*/
	for(i = 0; i < COACH_ATTENDANTS; i++){
		name = "coachAttendantRegularLineLength  ";
        	name[31] = (char)(i/10+48);
        	name[32] = (char)(i%10+48);
		coachAttendantRegularLineLength[i] = CreateMV(name,-1);
		name = "coachAttendantFirstLineLength  ";
        	name[29] = (char)(i/10+48);
        	name[30] = (char)(i%10+48);
		coachAttendantFirstLineLength[i] = CreateMV(name,-1);
		name = "coachAttendantAvailable  ";
        	name[23] = (char)(i/10+48);
        	name[24] = (char)(i%10+48);
		coachAttendantAvailable[i] = CreateMV(name,-1);
		
		name = "passengerTicketsHolder  ";
        	name[22] = (char)(i/10+48);
        	name[23] = (char)(i%10+48);
		passengerTicketsHolder[i] = CreateMV(name,-1);
		name = "passengerTicketsSeatNumber  ";
        	name[26] = (char)(i/10+48);
        	name[27] = (char)(i%10+48);
		passengerTicketsSeatNumber[i] = CreateMV(name,-1);
		
		name = "foodOrdersWaiting  ";
        	name[17] = (char)(i/10+48);
        	name[18] = (char)(i%10+48);
		foodOrdersWaiting[i] = CreateMV(name,-1);
		name = "coachAttendantFoodLineLength  ";
        	name[28] = (char)(i/10+48);
        	name[29] = (char)(i%10+48);
		coachAttendantFoodLineLength[i] = CreateMV(name,-1);
		
		name = "coachAttendantOrderHolder  ";
        	name[25] = (char)(i/10+48);
        	name[26] = (char)(i%10+48);
        	coachAttendantOrderHolder[i] = CreateMV(name,-1);
		name = "coachAttendantOrderFoodOrder  ";
        	name[28] = (char)(i/10+48);
        	name[29] = (char)(i%10+48);
        	coachAttendantOrderFoodOrder[i] = CreateMV(name,-1);
        name = "coachAttendantOrderValidFoodOrder  ";
        	name[33] = (char)(i/10+48);
        	name[34] = (char)(i%10+48);
        	coachAttendantOrderValidFoodOrder[i] = CreateMV(name,-1);
	}
	nextSeat = CreateMV("nextSeat",-1);
	/*--end CoachAttendant/Passenger interactions*/

	/*data for Porter/Passenger interactions*/
	for(i = 0; i < PORTERS; i++){
		name = "porterBaggageLineLength  ";
        	name[23] = (char)(i/10+48);
        	name[24] = (char)(i%10+48);
		porterBaggageLineLength[i] = CreateMV(name,-1);
		name = "porterAvailable  ";
        	name[15] = (char)(i/10+48);
        	name[16] = (char)(i%10+48);
		porterAvailable[i] = CreateMV(name,-1);
		
		name = "passengerBaggage  ";
        	name[16] = (char)(i/10+48);
        	name[17] = (char)(i%10+48);
		passengerBaggage[i] = CreateMV(name,-1);
		
		name = "porterBeddingLineLength  ";
        	name[23] = (char)(i/10+48);
        	name[24] = (char)(i%10+48);
		porterBeddingLineLength[i] = CreateMV(name,-1);
		name = "porterFirstClassBeddingLineLength  ";
        	name[33] = (char)(i/10+48);
        	name[34] = (char)(i%10+48);
		porterFirstClassBeddingLineLength[i] = CreateMV(name,-1);

		name = "passengerBeddingHolder  ";
        	name[22] = (char)(i/10+48);
        	name[23] = (char)(i%10+48);
		passengerBeddingHolder[i] = CreateMV(name,-1);
		name = "passengerBeddingHasBedding  ";
        	name[26] = (char)(i/10+48);
        	name[27] = (char)(i%10+48);
		passengerBeddingHasBedding[i] = CreateMV(name,-1);
	}
	/*--end Porter/Passenger data*/

	/*data for Waiter/Passenger interactions*/
	for(i = 0; i < WAITERS; i++){
		name = "waiterLineLength  ";
        	name[16] = (char)(i/10+48);
        	name[17] = (char)(i%10+48);
		waiterLineLength[i] = CreateMV(name,-1);
		
		name = "passengerFoodHolder  ";
        	name[19] = (char)(i/10+48);
        	name[20] = (char)(i%10+48);
		passengerFoodHolder[i] = CreateMV(name,-1);
		name = "passengerFoodFoodOrder  ";
        	name[22] = (char)(i/10+48);
        	name[23] = (char)(i%10+48);
		passengerFoodFoodOrder[i] = CreateMV(name,-1);
		name = "passengerFoodValidFoodOrder  ";
        	name[27] = (char)(i/10+48);
        	name[28] = (char)(i%10+48);
		passengerFoodValidFoodOrder[i] = CreateMV(name,-1);
	}
	/*--end Waiter/Passenger data*/

	/*data for ticketChecker/Conductor interactions*/
	ticketCheckerWaiting = CreateMV("ticketCheckerWaiting",-1);
	conductorWaiting = CreateMV("conductorWaiting",-1);
	/*--end ticketChecker/Conductor data*/

	/*data for Conductor/CoachAttendant interactions*/
	for(i = 0; i < MAX_STOPS; i++){
		name = "passengersBoardedAtThisStop  ";
        	name[27] = (char)(i/10+48);
        	name[28] = (char)(i%10+48);
		passengersBoardedAtThisStop[i] = CreateMV(name,-1);
	}
	/*--end conductor/CA interactions*/

	/*data for steward/passenger interations*/
	stewardLineLength = CreateMV("stewardLineLength",-1);
	stewardAvailable = CreateMV("stewardAvailable",-1);
	
	stewardTicketToCheckHolder = CreateMV("stewardTicketToCheckHolder",-1);
	stewardTicketToCheckWaiter = CreateMV("stewardTicketToCheckWaiter",-1);
	/*--end steward/passenger data*/

	/*data for steward/waiter interactions*/
	for(i = 0; i < WAITERS; i++){
		name = "waiterOnBreak  ";
        	name[13] = (char)(i/10+48);
        	name[14] = (char)(i%10+48);
		waiterOnBreak[i] = CreateMV(name,-1);
	}
	/*--end steward/waiter data*/

	/*data for chef/ANYONE interactions*/
	for(i = 0; i < CHEFS; i++){
		name = "chefOnBreak  ";
        	name[11] = (char)(i/10+48);
        	name[12] = (char)(i%10+48);
		chefOnBreak[i] = CreateMV(name,-1);
		name = "chefFirstClassOrdersLeft  ";
        	name[24] = (char)(i/10+48);
        	name[25] = (char)(i%10+48);
		chefFirstClassOrdersLeft[i] = CreateMV(name,-1);
		name = "chefRegularOrdersLeft  ";
        	name[21] = (char)(i/10+48);
        	name[22] = (char)(i%10+48);
		chefRegularOrdersLeft[i] = CreateMV(name,-1);
	}
	/*--end chef/ANYONE shared data*/


	/**************************************/
	/* Locks, etc			      */
	/**************************************/
	
	IDLock = CreateServerLock("IDLock",-1);
	atAStopLock = CreateServerLock("atAStopLock",-1);
	getOffAtThisStopLock = CreateServerLock("getOffAtThisStopLock",-1);
	getOffAtThisStop = CreateServerCondition("getOffAtThisStopCV",-1);
	ticketRevenueLock = CreateServerLock("ticketRevenueLock",-1);
	diningCarRevenueLock = CreateServerLock("diningCarRevenueLock",-1);
	waiterRevenueLock = CreateServerLock("waiterRevenueLock",-1);
	ticketLock = CreateServerLock("ticketLock",-1);
	ticketCheck = CreateServerCondition("ticketCheck",-1);
	globalSeatLock = CreateServerLock("globalSeatLock",-1);
	globalFoodLock = CreateServerLock("globalFoodLock",-1);
	waitingForBoardingCompletionLock = CreateServerLock("waitingForBoardingCompletionLock",-1);
	waitingForBoardingCompletion = CreateServerCondition("WaitingForBoardingCompletionCV",-1);
	boardingPassengersCountLock = CreateServerLock("BoardingPassengersCountLock",-1);
	stewardLock = CreateServerLock("stewardLock",-1);
	steward = CreateServerCondition("stewardCV",-1);
	diningCarCapacityLock = CreateServerLock("diningCarCapacityLock",-1);
	stewardGiveWaiterLock = CreateServerLock("stewardGiveWaiterLock",-1);
	stewardGiveWaiter = CreateServerCondition("stewardGiveWaiterCV",-1);
	stewardLine = CreateServerCondition("stewardLineCV",-1);
	finalConfirmation = CreateServerCondition("finalConfirmationCV",-1);

	/*setup bullhockey*/

	for(i=0; i<MAX_STOPS; i += 1){
	        name = "waitingForTrainCV  ";
        	name[17] = (char)(i/10+48);
        	name[18] = (char)(i%10+48);
	        waitingForTrain[i] = CreateServerCondition(name,-1);
		name = "waitingForTrainLock  ";
        	name[19] = (char)(i/10+48);
        	name[20] = (char)(i%10+48);
	        waitingForTrainLock[i] = CreateServerLock(name,-1);
	}

	ticketChecker = CreateServerCondition("ticketChecker",-1);
	ticketCheckerLine = CreateServerCondition("ticketCheckerLine",-1);
	ticketCheckerLock = CreateServerLock("ticketCheckerLock",-1);
	passengerCountLock = CreateServerLock("passengerCountLock",-1);

	waitingForGo = CreateServerCondition("waitingForGoCV",-1);
	waitingForGoLock = CreateServerLock("waitingForGoLock",-1);
	waitingForStop = CreateServerCondition("waitingForStopCV",-1);
	waitingForStopLock = CreateServerLock("waitingForStopLock",-1);

	for(i=0; i<COACH_ATTENDANTS; i += 1){
		name = "coachAttendantCV  ";
        	name[16] = (char)(i/10+48);
        	name[17] = (char)(i%10+48);
		coachAttendant[i] = CreateServerCondition(name,-1);
		name = "coachAttendantFirstLineCV  ";
        	name[25] = (char)(i/10+48);
        	name[26] = (char)(i%10+48);
		coachAttendantFirstLine[i] = CreateServerCondition(name,-1);
		name = "coachAttendantRegularLineCV  ";
        	name[27] = (char)(i/10+48);
        	name[28] = (char)(i%10+48);
		coachAttendantRegularLine[i] = CreateServerCondition(name,-1);
		name = "coachAttendantLineCV  ";
        	name[20] = (char)(i/10+48);
        	name[21] = (char)(i%10+48);
		coachAttendantLine[i] = CreateServerCondition(name,-1);
		name = "coachAttendantLock  ";
        	name[18] = (char)(i/10+48);
        	name[19] = (char)(i%10+48);
		coachAttendantLock[i] = CreateServerLock(name,-1);
		name = "coachAttendantLineLock  ";
        	name[22] = (char)(i/10+48);
        	name[23] = (char)(i%10+48);
		coachAttendantLineLock[i] = CreateServerLock(name,-1);
		name = "getSeatLock  ";
        	name[11] = (char)(i/10+48);
        	name[12] = (char)(i%10+48);
		getSeatLock[i] = CreateServerLock(name,-1);
		name = "getSeat  ";
        	name[7] = (char)(i/10+48);
        	name[8] = (char)(i%10+48);
		getSeat[i] = CreateServerCondition(name,-1);
		name = "CAWaitingForPorterCV  ";
        	name[20] = (char)(i/10+48);
        	name[21] = (char)(i%10+48);
		CAWaitingForPorter[i] = CreateServerCondition(name,-1);
		name = "CAWaitingForPorterLock  ";
        	name[22] = (char)(i/10+48);
        	name[23] = (char)(i%10+48);
		CAWaitingForPorterLock[i] = CreateServerLock(name,-1);
		name = "coachAttendantGetFoodCV  ";
        	name[23] = (char)(i/10+48);
        	name[24] = (char)(i%10+48);
		coachAttendantGetFood[i] = CreateServerCondition(name,-1);
		name = "coachAttendantGetFoodLock  ";
        	name[25] = (char)(i/10+48);
        	name[26] = (char)(i%10+48);
		coachAttendantGetFoodLock[i] = CreateServerLock(name,-1);
		name = "coachAttendantFoodLineCV  ";
        	name[24] = (char)(i/10+48);
        	name[25] = (char)(i%10+48);
		coachAttendantFoodLine[i] = CreateServerCondition(name,-1);
	}

	for(i=0; i<PORTERS;i += 1){
		name = "porterCV  ";
        	name[8] = (char)(i/10+48);
        	name[9] = (char)(i%10+48);
		porter[i] = CreateServerCondition(name,-1);
		name = "porterBaggageLineCV  ";
        	name[19] = (char)(i/10+48);
        	name[20] = (char)(i%10+48);
		porterBaggageLine[i] = CreateServerCondition(name,-1);
		name = "porterLineCV  ";
        	name[12] = (char)(i/10+48);
        	name[13] = (char)(i%10+48);
		porterLine[i] = CreateServerCondition(name,-1);
		name = "porterLock  ";
        	name[10] = (char)(i/10+48);
        	name[11] = (char)(i%10+48);
		porterLock[i] = CreateServerLock(name,-1);
		name = "porterBaggageLineLock  ";
        	name[21] = (char)(i/10+48);
        	name[22] = (char)(i%10+48);
		porterBaggageLineLock[i] = CreateServerLock(name,-1);
		name = "porterBeddingLineCV  ";
        	name[19] = (char)(i/10+48);
        	name[20] = (char)(i%10+48);
		porterBeddingLine[i] = CreateServerCondition(name,-1);
		name = "porterFirstClassBeddingLineCV  ";
        	name[29] = (char)(i/10+48);
        	name[30] = (char)(i%10+48);
		porterFirstClassBeddingLine[i] = CreateServerCondition(name,-1);
		name = "porterBeddingLineLock  ";
        	name[21] = (char)(i/10+48);
        	name[22] = (char)(i%10+48);
		porterBeddingLineLock[i] = CreateServerLock(name,-1);
	}


	for(i = 0; i < WAITERS; i += 1){
		name = "waiterLock  ";
        	name[10] = (char)(i/10+48);
        	name[11] = (char)(i%10+48);
		waiterLock[i] = CreateServerLock(name,-1);
		name = "waiterCV  ";
        	name[8] = (char)(i/10+48);
        	name[9] = (char)(i%10+48);
		waiter[i] = CreateServerCondition(name,-1);
		name = "waiterLineLock  ";
        	name[14] = (char)(i/10+48);
        	name[15] = (char)(i%10+48);
		waiterLineLock[i] = CreateServerLock(name,-1);
		name = "waiterLineCV  ";
        	name[12] = (char)(i/10+48);
        	name[13] = (char)(i%10+48);
		waiterLine[i] = CreateServerCondition(name,-1);
		name = "getFoodLock  ";
        	name[11] = (char)(i/10+48);
        	name[12] = (char)(i%10+48);
		waiterGetFoodLock[i] = CreateServerLock(name,-1);
		name = "getFoodCV  ";
        	name[9] = (char)(i/10+48);
        	name[10] = (char)(i%10+48);
		waiterGetFood[i] = CreateServerCondition(name,-1);
	}

	for(i=0; i<CHEFS;i += 1){
		name = "chefLock  ";
        	name[8] = (char)(i/10+48);
        	name[9] = (char)(i%10+48);
		chefLock[i] = CreateServerLock(name,-1);
		name = "chefCV  ";
        	name[6] = (char)(i/10+48);
        	name[7] = (char)(i%10+48);
		chef[i] = CreateServerCondition(name,-1);
		name = "chefOrdersLineCV  ";
        	name[16] = (char)(i/10+48);
        	name[17] = (char)(i%10+48);
		chefRegularOrdersLine[i] = CreateServerCondition(name,-1);
		name = "chefFirstClassOrdersLineCV  ";
        	name[26] = (char)(i/10+48);
        	name[27] = (char)(i%10+48);
		chefFirstClassOrdersLine[i] = CreateServerCondition(name,-1);
		name = "chefOrdersLineLock  ";
        	name[18] = (char)(i/10+48);
        	name[19] = (char)(i%10+48);
 		chefOrdersLineLock[i] = CreateServerLock(name,-1);
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
	SetMV(ticketRevenue, GetMV(ticketRevenue,-1) + 2 + (1*(int)isFirstClass),-1);
	
	ServerAcquire(IDLock,-1);
	myID = GetMV(passengerID,-1);
	SetMV(passengerID, GetMV(passengerID,-1) + 1,-1);
	ServerRelease(IDLock,-1);
        
        
       /*		int getOnStop, getOffStop;
			int holder;
			int okToBoard;
			int seatNumber;
			bool hasBedding;
			bool hasEaten;
			bool hasSlept;
			int foodOrder;
			bool validFoodOrder;
			int waiter;
	*/
        myTicket.getOnStop = tickets[myID].getOnStop;
        myTicket.getOffStop = tickets[myID].getOffStop;
        myTicket.holder = tickets[myID].holder;
        myTicket.okToBoard = tickets[myID].okToBoard;
        myTicket.seatNumber = tickets[myID].seatNumber;
        myTicket.hasBedding = tickets[myID].hasBedding;
        myTicket.hasEaten = tickets[myID].hasEaten;
        myTicket.hasSlept = tickets[myID].hasSlept;
        myTicket.foodOrder = tickets[myID].foodOrder;
        myTicket.validFoodOrder = tickets[myID].validFoodOrder;
        myTicket.waiter = tickets[myID].waiter;
        
        
        /*assumes passengers only get off at the last stop(for now)*/
        SetMV(myTicket.getOnStop, (Rand()%(MAX_STOPS-1)),-1);
        SetMV(myTicket.getOffStop, randomRange(GetMV(myTicket.getOnStop,-1),MAX_STOPS),-1);
        SetMV(myTicket.holder, myID,-1);/*FOR DEBUGGING*/
       	SetMV(myTicket.hasEaten, false,-1);
	SetMV(myTicket.hasSlept, false,-1);
	SetMV(myTicket.hasBedding, false,-1);
	SetMV(myTicket.foodOrder, -1,-1);
	SetMV(myTicket.validFoodOrder, false,-1);
	SetMV(myTicket.waiter, -1,-1);
	one[0] = myID;
	Print("Passenger %d belongs to Train 0\n",sizeof("Passenger %d belongs to Train 0\n"),one,1);
	two[0] = myID;
	two[1] = GetMV(myTicket.getOnStop,-1);
	Print("Passenger %d is going to get on the Train at stop number %d\n",sizeof("Passenger %d is going to get on the Train at stop number %d\n"),two,2);
	two[1] = GetMV(myTicket.getOffStop,-1);
	Print("Passenger %d is going to get off the Train at stop number %d\n",sizeof("Passenger %d is going to get off the Train at stop number %d\n"),two,2);

        /*passenger is now ready to go to a stop and wait to board the train*/
        ServerAcquire(waitingForTrainLock[GetMV(myTicket.getOnStop,-1)],-1);
        /*so he increments the number of people at the stop and waits*/
        SetMV(passengersWaitingToBoard[GetMV(myTicket.getOnStop,-1)], GetMV(passengersWaitingToBoard[GetMV(myTicket.getOnStop,-1)],-1) + 1,-1);
        /*printf("passenger%d is in line at stop %d\n",myID, myTicket.getOnStop);*/
        ServerWait(waitingForTrain[GetMV(myTicket.getOnStop,-1)], waitingForTrainLock[GetMV(myTicket.getOnStop,-1)],-1);
        ServerRelease(waitingForTrainLock[GetMV(myTicket.getOnStop,-1)],-1);

        /*and then checks on the ticketChecker*/
        ServerAcquire(ticketCheckerLock,-1);
        if(!GetMV(ticketCheckerAvailable,-1)){
                /*passenger gets in line*/
                passengerGotInLine=true;
                SetMV(ticketCheckerLineLength, GetMV(ticketCheckerLineLength,-1) + 1,-1);
                ServerWait(ticketCheckerLine, ticketCheckerLock,-1);
        }
        else {
                /*ticket checker is free so passenger goes up to him*/
                SetMV(ticketCheckerAvailable, false,-1);
                passengerGotInLine=false;
        }
        /*hand TC our ticket and wait to see what he says*/
        ServerAcquire(ticketLock,-1);
        SetMV(ticketToCheckHolder, myTicket.holder,-1);
        SetMV(ticketToCheckGetOnStop, myTicket.getOnStop,-1);
        SetMV(ticketToCheckOkToBoard, myTicket.okToBoard,-1);
        ServerSignal(ticketChecker, ticketCheckerLock,-1);
        ServerRelease(ticketCheckerLock,-1);
        /*printf("Passenger%d has signaled and is now waiting on ticketChecker\n", myID);*/
        ServerWait(ticketCheck, ticketLock,-1);
        /*let the ticketChecker go on to do other things, and see if we should get on the train*/
        if(passengerGotInLine)
                SetMV(ticketCheckerLineLength, GetMV(ticketCheckerLineLength,-1) - 1,-1);
        if(!GetMV(myTicket.okToBoard,-1)){
                /*i had a bad ticket*/
                /*print msg and return*/
                /*printf("passenger%d had a bad ticket and is leaving\n", myID);*/
		Print("Passenger %d of Train 0 has an invalid ticket\n",sizeof("Passenger %d of Train 0 has an invalid ticket\n"),one,1);
		ServerRelease(ticketLock,-1);
                return;
        }
		Print("Passenger %d of Train 0 has a valid ticket\n",sizeof("Passenger %d of Train 0 has a valid ticket\n"),one,1);
        /*I can get on the train and wait for my next thing*/
        /*printf("passenger%d got on the train\n", myID);*/
        /*ticketCheckerLock->ServerRelease();*/
        ServerRelease(ticketLock,-1);

	/*Passenger is now on the train and checking on the CoachAttendants*/
	passengerGotInLine = NULL;  /*Reusing this since it's not needed anymore for P/TC interaction*/
	
	/*Check for any available CA's first*/
	for(i = 0; i < COACH_ATTENDANTS; i += 1){
		ServerAcquire(coachAttendantLock[i],-1);
		if(GetMV(coachAttendantAvailable[i],-1)){		
			SetMV(coachAttendantAvailable[i], false,-1);
			/*passengerGotInLine = false;*/
			myCoachAttendant = i;
			/*my coach attendant was waiting for work, so i come up and signal him*/
			ServerSignal(coachAttendant[myCoachAttendant], coachAttendantLock[myCoachAttendant],-1);
			break;
		}
		else{
			ServerRelease(coachAttendantLock[i],-1);
		}
	}
	/*If no available CA could be found, then get in the shortest line*/
	if(myCoachAttendant == -1){
		myCoachAttendant = 0;
		ServerAcquire(coachAttendantLock[0],-1);
		shortestLength = isFirstClass ? GetMV(coachAttendantFirstLineLength[0],-1) : GetMV(coachAttendantRegularLineLength[0],-1);
		
		for(i = 1; i < COACH_ATTENDANTS; i += 1){
			ServerAcquire(coachAttendantLock[i],-1);
			if(isFirstClass){
				if(GetMV(coachAttendantFirstLineLength[i],-1) < shortestLength){
					shortestLength = GetMV(coachAttendantFirstLineLength[i],-1);
					ServerRelease(coachAttendantLock[myCoachAttendant],-1);
					myCoachAttendant = i;
					
				}
				else{
					ServerRelease(coachAttendantLock[i],-1);
				}
			}
			else{
				if(GetMV(coachAttendantRegularLineLength[i],-1) < shortestLength){
					shortestLength = GetMV(coachAttendantRegularLineLength[i],-1);
					ServerRelease(coachAttendantLock[myCoachAttendant],-1);
					myCoachAttendant = i;
				}
				else{
					ServerRelease(coachAttendantLock[i],-1);
				}
			}
		}
		/*signal our coach attendant*/
		/*printf("passenger%d getting in coachAttendant%d\'s line\n", myID, myCoachAttendant);*/
		ServerSignal(coachAttendant[myCoachAttendant], coachAttendantLock[myCoachAttendant],-1); /*QUESTIONABLE*/
	}
	
	ServerAcquire(coachAttendantLineLock[myCoachAttendant],-1);
	/*tell CA what kind of passenger you are*/
	if (isFirstClass){ 
		SetMV(coachAttendantFirstLineLength[myCoachAttendant],  GetMV(coachAttendantFirstLineLength[myCoachAttendant],-1) + 1,-1);
	}
	else{
		SetMV(coachAttendantRegularLineLength[myCoachAttendant], GetMV(coachAttendantRegularLineLength[myCoachAttendant],-1) + 1,-1);
	}
	
	ServerRelease(coachAttendantLock[myCoachAttendant],-1);
	
	/*printf("passenger%d waiting in coachAttendant%d\'s line\n", myID, myCoachAttendant);*/
	isFirstClass ? 
		ServerWait(coachAttendantFirstLine[myCoachAttendant], coachAttendantLineLock[myCoachAttendant],-1 ) : 
		ServerWait(coachAttendantRegularLine[myCoachAttendant], coachAttendantLineLock[myCoachAttendant],-1 );
	/*printf("passenger%d is moving up to coachAttendant%d\n", myID, myCoachAttendant);*/
	
	
	/*Passenger is ready to get a seat now*/
	ServerAcquire(getSeatLock[myCoachAttendant],-1);

	/*now that coach attendant is ready for me i hand him my ticket so he knows my name*/
	SetMV(passengerTicketsHolder[myCoachAttendant], myTicket.holder,-1);
	SetMV(passengerTicketsSeatNumber[myCoachAttendant], myTicket.seatNumber,-1);
	ServerSignal(coachAttendantLine[myCoachAttendant], coachAttendantLineLock[myCoachAttendant],-1 );
	ServerRelease(coachAttendantLineLock[myCoachAttendant],-1);
	/*printf("Passenger%d has signaled and is now waiting on coachAttendant%d\n", myID, myCoachAttendant);*/
	
	ServerWait(getSeat[myCoachAttendant], getSeatLock[myCoachAttendant],-1 );

	if(!isFirstClass){
		three[0] = myID;
		three[1] = GetMV(myTicket.seatNumber,-1);
		three[2] = myCoachAttendant;
		Print("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n", sizeof("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n"), three, 3);
	}

	if(isFirstClass)
		ServerAcquire(CAWaitingForPorterLock[myCoachAttendant],-1);

	ServerRelease(getSeatLock[myCoachAttendant],-1);/*end of regular class passenger/CA*/

	if(isFirstClass){
	/*Do porter stuff*/
		/*since it's a first class passenger, get a porter*/
		/*Check for any available porters first*/
		for(i = 0; i < PORTERS; i += 1){
			ServerAcquire(porterLock[i],-1);
			if(GetMV(porterAvailable[i],-1)){			
				SetMV(porterAvailable[i], false,-1);
				/*passengerGotInLine = false;*/
				myPorter = i;
				/*my porter was waiting for work, so i come up and signal him*/
				ServerSignal(porter[myPorter], porterLock[myPorter],-1);
				break;
			}
			else{
				ServerRelease(porterLock[i],-1);
			}
		}
		/*No Porter was available, so I have to get in line*/
		if(myPorter == -1){
			myPorter = 0;
			ServerAcquire(porterLock[0],-1);
			shortestLength = GetMV(porterBaggageLineLength[0],-1);
		
			for(i = 1; i < PORTERS; i += 1){
				ServerAcquire(porterLock[i],-1);
				if(GetMV(porterBaggageLineLength[i],-1) < shortestLength){
					shortestLength = GetMV(porterBaggageLineLength[i],-1);
					ServerRelease(porterLock[myPorter],-1);
					myPorter = i;
					
				}
				else{
					ServerRelease(porterLock[i],-1);
				}				
			}

			/*printf("passenger%d getting in porter%d\'s line\n", myID, myPorter);*/
			ServerSignal(porter[myPorter], porterLock[myPorter],-1); /*QUESTIONABLE*/
		}
		ServerAcquire(porterBaggageLineLock[myPorter],-1);
		SetMV(porterBaggageLineLength[myPorter], GetMV(porterBaggageLineLength[myPorter],-1) + 1,-1);
		ServerRelease(porterLock[myPorter],-1);
		
		/*printf("passenger%d waiting in porter%d\'s line\n", myID, myPorter);*/
		ServerWait(porterBaggageLine[myPorter], porterBaggageLineLock[myPorter],-1);
		/*printf("passenger%d is giving bags up to porter%d\n", myID, myPorter);*/
	
		/*show the porter my name*/
		SetMV(passengerBaggage[myPorter], myTicket.holder,-1);
		ServerSignal(porterLine[myPorter], porterBaggageLineLock[myPorter],-1 );
		ServerRelease(porterBaggageLineLock[myPorter],-1);

		ServerSignal(CAWaitingForPorter[myCoachAttendant], CAWaitingForPorterLock[myCoachAttendant],-1);
		ServerRelease(CAWaitingForPorterLock[myCoachAttendant],-1);
		three[0] = myID;
		three[1] = GetMV(myTicket.seatNumber,-1);
		three[2] = myCoachAttendant;
		Print("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n",sizeof("Passenger %d of Train 0 is given seat number %d by the Coach Attendant %d\n"), three, 3);
		two[0] = myID;
		two[1] = myPorter;
		Print("1st class Passenger %d of Train 0 is served by Porter %d\n",sizeof("1st class Passenger %d of Train 0 is served by Porter %d\n"),two,2); 
				
	}
	/*HAVE TO SET THESE BACK, due to bullhockey- they might be used later for ordering food or bedding*/
	myPorter=-1;
        myCoachAttendant=-1;

	while(!GetMV(myTicket.hasEaten,-1) || !GetMV(myTicket.hasSlept,-1)){
		if(!GetMV(myTicket.hasEaten,-1)){
			/*Randomly decide to get hungry*/
			if(Rand()%4 == 0){
				/*printf("passenger%d is now hungry\n",myID);*/
				/*ServerAcquire food somehow (depends on passenger class)*/
				if(isFirstClass){
					/*Do CA/Passenger food interaction*/
					for(i = 0; i < COACH_ATTENDANTS; i += 1){
						ServerAcquire(coachAttendantLock[i],-1);
						if(GetMV(coachAttendantAvailable[i],-1)){
							SetMV(coachAttendantAvailable[i], false,-1);

							myCoachAttendant = i;
							/*my CA was waiting for work, so i come up and signal him*/
							ServerSignal(coachAttendant[myCoachAttendant], coachAttendantLock[myCoachAttendant],-1);
							break;
						}
						else{
							ServerRelease(coachAttendantLock[i],-1);
						}
					}
					/*No CA was available, so I have to get in line*/
					if(myCoachAttendant == -1){
						myCoachAttendant = 0;
						ServerAcquire(coachAttendantLock[0],-1);
						shortestLength = GetMV(coachAttendantFoodLineLength[0],-1);
					
						for(i = 1; i < COACH_ATTENDANTS; i += 1){
							ServerAcquire(coachAttendantLock[i],-1);
							if(GetMV(coachAttendantFoodLineLength[i],-1) < shortestLength){
								shortestLength = GetMV(coachAttendantFoodLineLength[i],-1);
								ServerRelease(coachAttendantLock[myCoachAttendant],-1);
								myCoachAttendant = i;
								
							}
							else{
								ServerRelease(coachAttendantLock[i],-1);
							}				
						}

						/*printf("passenger%d getting in porter%d\'s line\n", myID, myPorter);*/
						ServerSignal(coachAttendant[myCoachAttendant], coachAttendantLock[myCoachAttendant],-1); /*QUESTIONABLE*/
					}
					ServerAcquire(coachAttendantLineLock[myCoachAttendant],-1);
					SetMV(coachAttendantFoodLineLength[myCoachAttendant], GetMV(coachAttendantFoodLineLength[myCoachAttendant],-1) + 1,-1);
					ServerRelease(coachAttendantLock[myCoachAttendant],-1);
		
					/*printf("passenger%d waiting in coachAttendant%d\'s food line\n", myID, myCoachAttendant);*/
					ServerWait(coachAttendantFoodLine[myCoachAttendant], coachAttendantLineLock[myCoachAttendant],-1);
					/*now I have my CA's attention, so I try to order*/
					ServerAcquire(coachAttendantGetFoodLock[myCoachAttendant],-1);
					ServerRelease(coachAttendantLineLock[myCoachAttendant],-1);

					do{
						SetMV(myTicket.foodOrder, Rand()%4,-1);
						SetMV(coachAttendantOrderHolder[myCoachAttendant], myTicket.holder,-1);
						SetMV(coachAttendantOrderFoodOrder[myCoachAttendant], myTicket.foodOrder,-1);
						SetMV(coachAttendantOrderValidFoodOrder[myCoachAttendant], myTicket.validFoodOrder,-1);
						ServerSignal(coachAttendantGetFood[myCoachAttendant], coachAttendantGetFoodLock[myCoachAttendant],-1 );
						ServerWait(coachAttendantGetFood[myCoachAttendant], coachAttendantGetFoodLock[myCoachAttendant],-1 );
					} while (!GetMV(myTicket.validFoodOrder,-1));
					/*i've been given the food, so I tip the CA*/
					ServerSignal(coachAttendantGetFood[myCoachAttendant], coachAttendantGetFoodLock[myCoachAttendant],-1);
					SetMV(myTicket.hasEaten, true,-1);
					ServerRelease(coachAttendantGetFoodLock[myCoachAttendant],-1);/*end 1stclass/CA	*/
				}
				else {
					/*Do Waiter/Passenger food interaction*/
					ServerAcquire(stewardLock,-1);
					if(GetMV(stewardAvailable,-1)){
						SetMV(stewardAvailable, false,-1);
						ServerSignal(steward, stewardLock,-1);
					}
					SetMV(stewardLineLength, GetMV(stewardLineLength,-1) + 1,-1);
					ServerWait(stewardLine, stewardLock,-1); /* now no matter what I have the steward's attention*/
					ServerAcquire(stewardGiveWaiterLock,-1); /* i'm not sure we need this lock*/
					SetMV(stewardTicketToCheckHolder, myTicket.holder,-1);
					SetMV(stewardTicketToCheckWaiter, myTicket.waiter,-1);
					ServerSignal(steward, stewardLock,-1);
					ServerRelease(stewardLock,-1);
					ServerWait(stewardGiveWaiter, stewardGiveWaiterLock,-1);/*definitely need this cv*/
					/*I either have a waiter or not*/
					if(GetMV(myTicket.waiter,-1) == -1){
						ServerRelease(stewardGiveWaiterLock,-1);
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
						ServerAcquire(waiterLineLock[GetMV(myTicket.waiter,-1)],-1);
						ServerRelease(stewardGiveWaiterLock,-1);
						SetMV(waiterLineLength[GetMV(myTicket.waiter,-1)], GetMV(waiterLineLength[GetMV(myTicket.waiter,-1)],-1) + 1,-1);
						ServerAcquire(stewardLock,-1);
						ServerSignal(steward, stewardLock,-1);
						/*stewardLineLength -= 1;*/
						ServerRelease(stewardLock,-1);/*if i get context switched here.. i lose*/
						/*two possibilities - one, i get to wait first, and things are fine...*/
						/*two, waiter goes first, sees someone was in line, signals, but i'm not waiting*/
							/*THIS IS NOW FIXED- steward waits for you to signal him that you have your waiter*/
						/*printf("passenger%d dies here\n",myID);*/
						ServerWait(waiterLine[GetMV(myTicket.waiter,-1)], waiterLineLock[GetMV(myTicket.waiter,-1)],-1);
						/*printf("passenger%d didn't die there\n", myID);*/
						ServerAcquire(waiterGetFoodLock[GetMV(myTicket.waiter,-1)],-1);
						ServerAcquire(stewardLock,-1);
						SetMV(stewardLineLength, GetMV(stewardLineLength,-1) - 1,-1);
						ServerSignal(finalConfirmation, stewardLock,-1);
						ServerRelease(stewardLock,-1);/*end passenger/steward*/
						/*printf("passenger%d got the food lock\n",myID);*/
						ServerRelease(waiterLineLock[GetMV(myTicket.waiter,-1)],-1);
						/*i've been woken up so i give my order*/
						
						do{
							SetMV(myTicket.foodOrder, Rand()%4,-1);
							SetMV(passengerFoodHolder[GetMV(myTicket.waiter,-1)], myTicket.holder,-1);
							SetMV(passengerFoodFoodOrder[GetMV(myTicket.waiter,-1)], myTicket.foodOrder,-1);
							SetMV(passengerFoodValidFoodOrder[GetMV(myTicket.waiter,-1)], myTicket.validFoodOrder,-1);
							ServerSignal(waiterGetFood[GetMV(myTicket.waiter,-1)], waiterGetFoodLock[GetMV(myTicket.waiter,-1)] ,-1);
							ServerWait(waiterGetFood[GetMV(myTicket.waiter,-1)], waiterGetFoodLock[GetMV(myTicket.waiter,-1)] ,-1);
						} while (!GetMV(myTicket.validFoodOrder,-1));
						two[0] = myID;
						two[1] = GetMV(myTicket.waiter,-1);
						Print("Passenger %d of Train 0 is served by Waiter %d\n",sizeof("Passenger %d of Train 0 is served by Waiter %d\n"),two,2);
						/*simulate a random amount of time to actually eat the food*/
						for(i=0; i<(Rand()%13)+12;i += 1){
							Yield();
						}

						/*let the waiter know i'm done eating*/
						ServerSignal(waiterGetFood[GetMV(myTicket.waiter,-1)], waiterGetFoodLock[GetMV(myTicket.waiter,-1)],-1);						
						/*wait for the waiter to give me the bill*/
						ServerWait(waiterGetFood[GetMV(myTicket.waiter,-1)], waiterGetFoodLock[GetMV(myTicket.waiter,-1)],-1);
						/*let the waiter see i'm done paying the bill*/
						ServerSignal(waiterGetFood[GetMV(myTicket.waiter,-1)], waiterGetFoodLock[GetMV(myTicket.waiter,-1)],-1);
						SetMV(myTicket.hasEaten, true,-1);

						ServerRelease(waiterGetFoodLock[GetMV(myTicket.waiter,-1)],-1);/*end passenger/waiter*/
					}
				}
			}
		}

		if(!GetMV(myTicket.hasSlept,-1)){
			if(GetMV(myTicket.hasBedding,-1)){
				/*Sleep*/
				for( i = 0; i < 5; i += 1){
					Yield();
				}
				SetMV(myTicket.hasSlept, true,-1);
			} 
			else {
				/*Randomly decide to order bedding*/
				if(Rand()%4 == 0){
					/*printf("Passenger %d calls for bedding\n", myID);*/
					/*Do porter stuff*/
					/*Check for any available porters first*/
					for(i = 0; i < PORTERS; i += 1){
						ServerAcquire(porterLock[i],-1);
						if(GetMV(porterAvailable[i],-1)){
							SetMV(porterAvailable[i], false,-1);
							/*passengerGotInLine = false;*/
							myPorter = i;
							/*my porter was waiting for work, so i come up and signal him*/
							ServerSignal(porter[myPorter], porterLock[myPorter],-1);
							break;
						}
						else{
							ServerRelease(porterLock[i],-1);
						}
					}
					/*No Porter was available, so I have to get in line*/
					if(myPorter == -1 && (!isFirstClass)){
						myPorter = 0;
						ServerAcquire(porterLock[0],-1);
						shortestLength = GetMV(porterBeddingLineLength[0],-1);
						for(i = 1; i < PORTERS; i += 1){
							ServerAcquire(porterLock[i],-1);
							if(GetMV(porterBeddingLineLength[i],-1) < shortestLength){
								shortestLength = GetMV(porterBeddingLineLength[i],-1);
								ServerRelease(porterLock[myPorter],-1);
								myPorter = i;
							}
							else{
								ServerRelease(porterLock[i],-1);
							}				
						}

						ServerSignal(porter[myPorter], porterLock[myPorter],-1); 
					}
					else if(myPorter==-1 && isFirstClass){
						myPorter = 0;
						ServerAcquire(porterLock[0],-1);
						shortestLength = GetMV(porterFirstClassBeddingLineLength[0],-1);
						for(i = 1; i < PORTERS; i += 1){
							ServerAcquire(porterLock[i],-1);
							if(GetMV(porterFirstClassBeddingLineLength[i],-1) < shortestLength){
								shortestLength = GetMV(porterFirstClassBeddingLineLength[i],-1);
								ServerRelease(porterLock[myPorter],-1);
								myPorter = i;
							}
							else{
								ServerRelease(porterLock[i],-1);
							}				
						}

						ServerSignal(porter[myPorter], porterLock[myPorter],-1); /*QUESTIONABLE*/

					}
					ServerAcquire(porterBeddingLineLock[myPorter],-1);
					if(isFirstClass){
						SetMV(porterFirstClassBeddingLineLength[myPorter], GetMV(porterFirstClassBeddingLineLength[myPorter],-1) + 1,-1);
					}
					else{
						SetMV(porterBeddingLineLength[myPorter], GetMV(porterBeddingLineLength[myPorter],-1) + 1,-1);
					}
					ServerRelease(porterLock[myPorter],-1);
					
					/*printf("passenger%d waiting in porter%d\'s bedding line\n", myID, myPorter);*/
					one[0] = myID;
					Print("Passenger %d calls for bedding\n",sizeof("Passenger %d calls for bedding\n"),one,1);
					isFirstClass?
						ServerWait(porterFirstClassBeddingLine[myPorter], porterBeddingLineLock[myPorter],-1):
						ServerWait(porterBeddingLine[myPorter], porterBeddingLineLock[myPorter],-1);
				
					/*show the porter my name*/
					SetMV(passengerBeddingHolder[myPorter], myTicket.holder,-1);
					SetMV(passengerBeddingHasBedding[myPorter], myTicket.hasBedding,-1);
					ServerSignal(porterLine[myPorter], porterBeddingLineLock[myPorter],-1 );
					ServerWait(porterLine[myPorter], porterBeddingLineLock[myPorter],-1 );
					/*printf("passenger%d got bedding from porter%d\n", myID, myPorter);*/
					ServerRelease(porterBeddingLineLock[myPorter],-1);
				}
			}	
		}
	}
	/*I got on, sat down, ate and slept, so now it's time to see if i should get off*/
	while(!disembarked){
		ServerAcquire(atAStopLock,-1);
		/*if(nowStopped){*/
			/*printf("%d, %d\n",currentStop,myTicket.getOffStop);*/
			if(GetMV(currentStop,-1) == GetMV(myTicket.getOffStop,-1)){
				ServerAcquire(passengerCountLock,-1);
				SetMV(passengersOnTrain, GetMV(passengersOnTrain,-1) - 1,-1);
				/*printf("a passenger got themselves off got down\n");*/
				two[0] = myID;
				two[1] = GetMV(currentStop,-1);
				Print("Passenger %d of Train 0 is getting off the Train at stop number %d\n",sizeof("Passenger %d of Train 0 is getting off the Train at stop number %d\n"),two,2);
				ServerRelease(passengerCountLock,-1);
				ServerRelease(atAStopLock,-1);
				disembarked=true;
			}
			else if(GetMV(currentStop,-1) > GetMV(myTicket.getOffStop,-1)){
				/*i am getting off late*/
				ServerAcquire(passengerCountLock,-1);
				SetMV(passengersOnTrain, GetMV(passengersOnTrain,-1) - 1,-1);
				/*printf("a passenger got off late... most disappointing\n");*/
				/*I pay a fine for getting of late- DOH*/
				ServerAcquire(ticketRevenueLock,-1);
				SetMV(ticketRevenue, GetMV(ticketRevenue,-1) + 2,-1);
				ServerRelease(ticketRevenueLock,-1);
				two[0] = myID;
				two[1] = GetMV(currentStop,-1);
				Print("Passenger %d of Train 0 is getting off the Train at stop number %d\n",sizeof("Passenger %d of Train 0 is getting off the Train at stop number %d\n"),two,2);
				ServerRelease(passengerCountLock,-1);
				ServerRelease(atAStopLock,-1);
				disembarked=true;
			}
			else{
				ServerRelease(atAStopLock,-1);
				/*wait for OUR stop- conductor will inform us to wakeup at the next stop*/
				ServerAcquire(getOffAtThisStopLock,-1);
				ServerWait(getOffAtThisStop, getOffAtThisStopLock,-1);
				ServerRelease(getOffAtThisStopLock,-1);
			}
	}
		/*}
		else{
			atAStopLock->ServerRelease();
			currentThread->Yield();
		}*/
	
/*
		else {			
			//I'm either not at a stop, or the stop is less than the one I want
			getOffAtThisStopLock->ServerAcquire();
			do{
				atAStopLock->ServerRelease();
				getOffAtThisStop->ServerWait(getOffAtThisStopLock);
				atAStopLock->ServerAcquire();
			}while(currentStop<myTicket.getOffStop);
			getOffAtThisStopLock->ServerRelease();
			passengerCountLock->ServerAcquire();
			passengersOnTrain -= 1;
			if(currentStop==myTicket.getOffStop){
				printf("a passenger got themselves off// got down\n");
				passengerCountLock->ServerRelease();
				atAStopLock->ServerRelease();
			}
			else{
				printf("a passenger got off late... most disappointing\n");
				passengerCountLock->ServerRelease();
				atAStopLock->ServerRelease();
			}
		}
*/	
	
	/*printf("PASSENGER%d finished!\n", myID);*/
	SetMV(passengersFinished, GetMV(passengersFinished,-1) + 1,-1);
	if(GetMV(myTicket.hasEaten,-1))
		SetMV(passengersEaten, GetMV(passengersEaten,-1) + 1,-1);
	if(GetMV(myTicket.hasSlept,-1))
		SetMV(passengersSlept, GetMV(passengersSlept,-1) + 1,-1);
}/*end of passenger */


void main(){

	createVars();
		
	Passenger();

}/*end simulation*/



