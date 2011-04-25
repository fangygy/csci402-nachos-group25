/* AppClerk.c
 */

#include "syscall.h"
#define NUM_CLERKS 5
#define NUM_CUSTOMERS 30
#define NUM_SENATORS 5
#define NV 0x9999

/*
*	Yinlerthai Chan
*	Application Clerk function thread
*	Receives an "application" from the Customer in the form of an SSN
*	Passes the SSN data to the Passport Clerk
*	Files the Customer's application, then dismisses the Customer
*	
*/
enum BOOLEAN{
	false,
	true
};

int traceLock;

void ClerkTrace(char* clerkType, int myIndex, char* custType, int myCust, char* msg) {
	/* Example */
	/* ClerkTrace("App", myIndex, "Sen", myClerk, "Here is mah message to a Senator.\n") */
	/* ClerkTrace("App", myIndex, 0x00, 0, "Here is mah message to no one in particular.\n") */
	Trace(clerkType, myIndex);
	if (custType != 0x00) {
		Trace(" -> ", NV);
		Trace(custType, myCust);
	}
	Trace(": ", NV);
	Trace(msg, NV);
}

int main() {
	/* ------------------Local data -------------------------*/
	int myIndex;

	int initIndex = CreateMV("ApIn", sizeof("ApIn"), 1, 0x9999);
	int initIndexLock = ServerCreateLock("ApInLk", sizeof("ApInLk"), 1);

	int mySSN;
	int i;
	int cType = 0;

	enum BOOLEAN loop = true;
	int shutdown = CreateMV("shut", sizeof("shut"), 1, 0x9999);
	/* -----------------Shared Data------------------------*/

	/* Number of customers/senators in office and thier locks */
	int officeSenator = CreateMV("OfSe", sizeof("OfSe"), 1, 0x9999);
	int officeCustomer = CreateMV("OfCu", sizeof("OfCu"), 1, 0x9999);	
	int senatorLock = ServerCreateLock("SeLk", sizeof("SeLk"), 1);
	int customerLock = ServerCreateLock("CuLk", sizeof("CuLk"), 1);

	/*Locks, conditions, and data used for waitinng */
	int clerkWaitLock = ServerCreateLock("ClWaLk", sizeof("ClWaLk"), 1);
	int clerkWaitCV = ServerCreateCV("ClWaCV", sizeof("ClWaCV"), 1);
	int numAppWait = CreateMV("NuApWa", sizeof("NuApWa"), 1, 0x9999);

	/* Reg and priv line lengths, line locks, and reg and priv line conditions. */
	int regACLineLength = CreateMV("RgApLn", sizeof("RgApLn"), 1, 0x9999);
	int privACLineLength = CreateMV("PrApLn", sizeof("PrApLn"), 1, 0x9999);
	int acpcLineLock = ServerCreateLock("ACPCLk", sizeof("ACPCLk"), 1);
	int regACLineCV = ServerCreateCV("RgApCV", sizeof("RgApCV"), 1);
	int privACLineCV = ServerCreateCV("PrApCV", sizeof("PrApCV"), 1);


	/* Individual clerk locks, conditions, data, data booleans, and states */
	int appLock = ServerCreateLock("ApLk", sizeof("ApLk"), NUM_CLERKS);
	int appCV = ServerCreateCV("ApCV", sizeof("ApCV"), NUM_CLERKS);

	int appState = CreateMV("ApSt", sizeof("ApSt"), NUM_CLERKS, 0x9999);
	int appData = CreateMV("ApDa", sizeof("ApDa"), NUM_CLERKS, 0x9999);

	/* Money data and locks for each clerk type */
	int appMoney = CreateMV("ApMn", sizeof("ApMn"), 1, 0x9999);
	int appMoneyLock = ServerCreateLock("ApMnLk", sizeof("ApMnLk"), 1);

	/* Individual customer's file state, type, and its lock */
	int fileLock = ServerCreateLock("FiLk", sizeof("FiLk"), NUM_CUSTOMERS + NUM_SENATORS);
	int fileState = CreateMV("FiSt", sizeof("FiSt"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	int fileType = CreateMV("FiTp", sizeof("FiTp"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);

	traceLock = ServerCreateLock("trace", sizeof("trace"), 1);
	
	/* initializes myIndex */
	ServerAcquire(initIndexLock, 0);
	myIndex = GetMV(initIndex, 0);
	SetMV(initIndex, 0, GetMV(initIndex, 0) + 1);
	ServerRelease(initIndexLock, 0);
	
	SetMV(appState, myIndex, 1);
	Write("AppClerk starting up\n", sizeof("AppClerk starting up\n"), ConsoleOutput);

	Write("AppClerk about to loop forever\n", sizeof("AppClerk about to loop forever\n"), ConsoleOutput);

	/* --------------------BEGIN APPCLERK STUFF----------------*/
	while(loop == true){

		Write("AppClerk starting loop\n", sizeof("AppClerk starting loop\n"), ConsoleOutput);

		if (GetMV(shutdown, 0) == 1) {
			ServerAcquire(traceLock, 0);
			ClerkTrace("App", myIndex, 0x00, 0, "Shutting down.\n"); 
			ServerRelease(traceLock, 0);
			Exit(0);
		}
		ServerAcquire(senatorLock, 0);
		ServerAcquire(customerLock, 0);
		if(GetMV(officeSenator, 0) > 0 && GetMV(officeCustomer, 0) > 0){
			ServerRelease(senatorLock, 0);
			ServerRelease(customerLock, 0);

			ServerAcquire(clerkWaitLock, 0);
			ServerWait(clerkWaitCV, 0, clerkWaitLock, 0);
			ServerRelease(clerkWaitLock, 0);
		}
		else{
			ServerRelease(senatorLock, 0);
			ServerRelease(customerLock, 0);
		}

		ServerAcquire(acpcLineLock, 0);
		
		/* Check for the privileged customer line first
		*  If there are privileged customers, do AppClerk tasks, then received $500
		*/
		if(GetMV(privACLineLength, 0) > 0){
			Write("AppClerk checking priv line\n", sizeof("AppClerk checking priv line\n"), ConsoleOutput);
			/*Trace("Checking privLine\n", 0x9999);*/
			SetMV(privACLineLength, 0, GetMV(privACLineLength, 0)-1);

			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0)+1);	/* shows customer that one clerk is waiting */
			ServerRelease(senatorLock, 0);

			ServerAcquire(appLock, myIndex);
			SetMV(appState, myIndex, 0);	/* 0 = AVAILABLE */
			ServerSignal(privACLineCV, 0, acpcLineLock, 0); /* Signals the next customer in priv line */
			ServerRelease(acpcLineLock, 0);
			/*Trace("Waiting privLine\n", 0x9999);*/
			ServerWait(appCV, myIndex, appLock, myIndex); /* Waits for the next customer */

			mySSN = GetMV(appData, myIndex);
			ServerAcquire(fileLock, mySSN);
			Write("AppClerk gotten SSN from priv customer\n", sizeof("AppClerk gotten SSN from priv customer\n"), ConsoleOutput);

			/* check the customer type */
			if(GetMV(fileType, mySSN) == 0){ /* 0 = CUSTOMER */
				cType = 0;
			}
			else{
				cType = 1;	/* 1 = SENATOR */
			}

			if(GetMV(fileState, mySSN) == 0){ /* 0 = NONE */
				SetMV(fileState, mySSN, 2); /* 2 = APPDONE */
				ServerRelease(fileLock, mySSN);
			}
			else if(GetMV(fileState, mySSN) == 1){ /* 1 = PICDONE */
				SetMV(fileState, mySSN, 3); /* 3 = APPPICDONE */
				ServerRelease(fileLock, mySSN);
			}
			else{
				ServerAcquire(traceLock, 0);
				ClerkTrace("App", myIndex, "Cust", mySSN,
					"ERROR:Customer doesn't have a picture application or no application. What are you doing here?\n");
				ServerRelease(traceLock, 0);
				ServerRelease(fileLock, mySSN);
			}

			for (i = 0; i < 20; i++){
				Yield();
			}

			ServerAcquire(traceLock, 0);
			if(cType == 0){
				ClerkTrace("App", myIndex, "Cust", mySSN, "Customer's app has been filed.\n");
			}
			else{
				ClerkTrace("App", myIndex, "Sen", mySSN, "Senator's app has been filed.\n");
			}
			ServerRelease(traceLock, 0);

			ServerAcquire(appMoneyLock, 0);
			SetMV(appMoney, 0, GetMV(appMoney, 0) + 500);

			ServerAcquire(traceLock, 0);
			if(cType == 0){
				ClerkTrace("App", myIndex, "Cust", mySSN, "Accepts $500 from Customer.\n");
			}
			else{
				ClerkTrace("App", myIndex, "Sen", mySSN, "Accepts $500 from Senator.\n");
			}
			ServerRelease(traceLock, 0);

			ServerRelease(appMoneyLock, 0);
			ServerSignal(appCV, myIndex, appLock, myIndex); /* signal customer awake */
			ServerRelease(appLock, myIndex);	/* Release clerk lock */
			Write("AppClerk done with priv customer\n", sizeof("AppClerk done with priv customer\n"), ConsoleOutput);

		}	
		/* Check for regular customer line next.
		* If there are regular customers, do AppClerk tasks 
		*/
		else if(GetMV(regACLineLength,0) > 0){
			Write("AppClerk checking reg line\n", sizeof("AppClerk checking reg line\n"), ConsoleOutput);
			/*Trace("Checking regLine\n", 0x9999);*/
			SetMV(regACLineLength, 0, GetMV(regACLineLength, 0)-1);

			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0)+1);	/* shows customer that one clerk is waiting */
			ServerRelease(senatorLock, 0);

			ServerAcquire(appLock, myIndex);
			SetMV(appState, myIndex, 0); /* 0 = AVAILABLE */
			ServerSignal(regACLineCV, 0, acpcLineLock, 0); /* Signals the next customer in priv line */
			ServerRelease(acpcLineLock, 0);
			/*Trace("Waiting regLine\n", 0x9999);*/
			ServerWait(appCV, myIndex, appLock, myIndex); /* Waits for the next customer */

			mySSN = GetMV(appData, myIndex);
			ServerAcquire(fileLock, mySSN);
			Write("AppClerk gotten SSN from reg customer\n", sizeof("AppClerk gotten SSN from reg customer\n"), ConsoleOutput);

			/* check the customer type */
			if(GetMV(fileType, mySSN) == 0){
				cType = 0;
			}
			else{
				cType = 1;
			}

			if(GetMV(fileState, mySSN) == 0){ /* 0 = NONE */
				SetMV(fileState, mySSN, 2); /* 2 = APPDONE */
				ServerRelease(fileLock, mySSN);
			}
			else if(GetMV(fileState, mySSN) == 1){ /* 1 = PICDONE */
				SetMV(fileState, mySSN, 3); /* 3 = APPPICDONE */
				ServerRelease(fileLock, mySSN);
			}
			else{
				ServerAcquire(traceLock, 0);
				ClerkTrace("App", myIndex, "Cust", mySSN,
					"ERROR:Customer doesn't have a picture application or no application. What are you doing here?\n");
				ServerRelease(traceLock, 0);
				ServerRelease(fileLock, mySSN);
			}

			for (i = 0; i < 20; i++){
				Yield();
			}

			ServerAcquire(traceLock, 0);
			if(cType == 0){
				ClerkTrace("App", myIndex, "Cust", mySSN, "Informs Customer their app has been filed.\n");
			}
			else{
				ClerkTrace("App", myIndex, "Sen", mySSN, "Informs Senator their app has been filed.\n");
			}
			ServerRelease(traceLock, 0);

			ServerSignal(appCV, myIndex, appLock, myIndex); /* signal customer awake */
			ServerRelease(appLock, myIndex);	/* Release clerk lock */
			Write("AppClerk done with reg customer\n", sizeof("AppClerk done with reg customer\n"), ConsoleOutput);
		}
		/* If there are neither privileged or regular customers, go on break */
		else{
			Write("AppClerk going on break\n", sizeof("AppClerk going on break\n"), ConsoleOutput);
			/*Trace("Going on break\n", 0x9999);*/
			ServerRelease(acpcLineLock, 0);
			ServerAcquire(appLock, myIndex);
			SetMV(appState, myIndex, 2); /* 2 = BREAK */
			
			ServerAcquire(traceLock, 0);
			ClerkTrace("App", myIndex, 0x00, 0, "Going on break.\n");
			ServerRelease(traceLock, 0);

			ServerWait(appCV, myIndex, appLock, myIndex);
			
			ServerAcquire(traceLock, 0);
			ClerkTrace("App", myIndex, 0x00, 0, "Returning from break.\n");
			ServerRelease(traceLock, 0);
			
			ServerRelease(appLock, myIndex);
		}
	}
}