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

void Conductor(){
        const unsigned int myID = 0;
        int i = 0;
        
        int one[1];
	int two[2];
	int three[3];
	int four[4];

        while(GetMV(currentStop,-1) < MAX_STOPS-1){
                ServerAcquire(waitingForStopLock,-1);

                if(GetMV(ticketCheckerWaiting,-1)){
                        /*conductor yields 25-50 times before he decides the next stop has arrived      */
                        for(i = 0; i<(randomRange(0,26)+25); i += 1){
                        	Yield();
                        }

                }
                else{
                        SetMV(conductorWaiting, true,-1);
                        ServerWait(waitingForStop, waitingForStopLock,-1);
                        /*conductor yields 25-50 times before he decides the next stop has arrived      */
                        for(i = 0; i<(randomRange(0,26)+25); i += 1){
                        	Yield();
                        }
                        
                }
		ServerAcquire(atAStopLock,-1);
                SetMV(currentStop, GetMV(currentStop,-1) + 1,-1);
                one[0] = GetMV(currentStop,-1);
		Print("Conductor announces exit for Passengers at stop number %d\n",sizeof("Conductor announces exit for Passengers at stop number %d\n"),one,1);
		SetMV(nowStopped, true,-1);
		ServerRelease(atAStopLock,-1);
                /*printf("WOO WOO, train has just pulled into stop %d\n", currentStop);*/

		/*tell people to check the current stop*/
                ServerBroadcast(waitingForStop, waitingForStopLock,-1);
                ServerAcquire(waitingForGoLock,-1);
                ServerRelease(waitingForStopLock,-1);
             
		SetMV(conductorWaiting, false,-1);
                ServerWait(waitingForGo, waitingForGoLock,-1);

                /*i have been cleared to go, so i do my bookeeping with a random chance and then move on*/
                /*print the number of passengers in the train*/
		ServerAcquire(boardingPassengersCountLock,-1);
                ServerRelease(waitingForGoLock,-1);
		/*check to see if the kitchen requested more stock at this stop*/
		ServerAcquire(globalFoodLock,-1);
		for(i=0;i<4;i += 1){
			if(GetMV(foodStockLow[i],-1)){
				SetMV(foodStock[i], GetMV(foodStock[i],-1) + RESTOCK_AMOUNT,-1);
			}
		}
		ServerRelease(globalFoodLock,-1);

                /*randomly check ticket value*/
                /*randomly check dining car safe value*/
		/*wait for coach attendant to let me know that everyone has gotten on...*/

		/*tell everyone waiting to get off that they should get off*/
		ServerAcquire(getOffAtThisStopLock,-1);
		ServerBroadcast(getOffAtThisStop, getOffAtThisStopLock,-1);
		ServerRelease(getOffAtThisStopLock,-1);

		if(GetMV(passengersBoardedAtThisStop[GetMV(currentStop,-1)],-1) != GetMV(passengersWaitingToBoard[GetMV(currentStop,-1)],-1)){
			ServerAcquire(waitingForBoardingCompletionLock,-1);
			ServerRelease(boardingPassengersCountLock,-1);
			/*printf("conductor has found that not everyone has boarded, so he's going to sleep\n");*/
			ServerWait(waitingForBoardingCompletion, waitingForBoardingCompletionLock,-1);
			/*printf("Conductor finds that everyone has boarded on this stop, time to loop againXD\n");*/
			ServerRelease(waitingForBoardingCompletionLock,-1);
		}
		else{
			/*printf("Conductor finds that everyone has boarded on this stop, time to loop againXD\n");*/
			ServerRelease(boardingPassengersCountLock,-1);
		}
		/*randomly do bookeeping with a 1/3 chance */
		if(Rand()%3==0){
			ServerAcquire(diningCarRevenueLock,-1);
			ServerAcquire(ticketRevenueLock,-1);
			one[0] = GetMV(diningCarRevenue,-1);
			Print("Train 0 Dining Car revenue = %d\n",sizeof("Train 0 Dining Car revenue = %d\n"),one,1);
			one[0] = GetMV(ticketRevenue,-1);
			Print("Train 0 Ticket Revenue = %d\n",sizeof("Train 0 Ticket Revenue = %d\n"),one,1);
			SetMV(totalRevenue, GetMV(diningCarRevenue,-1) + GetMV(ticketRevenue,-1),-1);
			one[0] = GetMV(totalRevenue,-1);
			Print("Total Train 0 Revenue = %d\n",sizeof("Total Train 0 Revenue = %d\n"),one,1);
			ServerRelease(diningCarRevenueLock,-1);
			ServerRelease(ticketRevenueLock,-1);
		}


		/*passengerCountLock->Acquire();*/
		one[0] = GetMV(passengersOnTrain,-1);
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
	while(GetMV(passengersOnTrain,-1) != 0){
		ServerAcquire(getOffAtThisStopLock,-1);
		ServerBroadcast(getOffAtThisStop, getOffAtThisStopLock,-1);
		ServerRelease(getOffAtThisStopLock,-1);
		for(i=0; i<30; i += 1){
			Yield(); /*give the lingering passengers a chance to finish eating/sleeping*/
		}
	}
	/*do a final set of prints to summarize*/
	
	ServerAcquire(diningCarRevenueLock,-1);
	ServerAcquire(ticketRevenueLock,-1);
	one[0] = GetMV(diningCarRevenue,-1);
	Print("Train 0 Dining Car revenue = %d\n",sizeof("Train 0 Dining Car revenue = %d\n"),one,1);
	one[0] = GetMV(ticketRevenue,-1);
	Print("Train 0 Ticket Revenue = %d\n",sizeof("Train 0 Ticket Revenue = %d\n"),one,1);
	SetMV(totalRevenue, GetMV(diningCarRevenue,-1) + GetMV(ticketRevenue,-1),-1);
	one[0] = GetMV(totalRevenue,-1);
	Print("Total Train 0 Revenue = %d\n",sizeof("Total Train 0 Revenue = %d\n"),one,1);
	ServerRelease(diningCarRevenueLock,-1);
	ServerRelease(ticketRevenueLock,-1);
	ServerAcquire(passengerCountLock,-1);
	one[0] = GetMV(passengersOnTrain,-1);
	Print("Passengers in the train 0 = %d\n",sizeof("Passengers in the train 0 = %d\n"),one,1);
	ServerRelease(passengerCountLock,-1);

}/*end of conductor*/

void main(){

	createVars();
		
	Conductor();
}/*end simulation*/


