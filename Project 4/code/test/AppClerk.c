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

int main() {
	/* ------------------Local data -------------------------*/
	int myIndex;

	int initIndex = CreateMV("AppIndex", sizeof("AppIndex"), 1, 0x9999);
	int initIndexLock = ServerCreateLock("AppIndexLock", sizeof("AppIndexLock"), 1);

	int mySSN;
	int i;
	int cType = 0;

	enum BOOLEAN loop = true;
	int shutdown = CreateMV("shutdown", sizeof("shutdown"), 1, 0x9999);
	/* -----------------Shared Data------------------------*/

	/* Number of customers/senators in office and thier locks */
	int officeSenator = CreateMV("officeSenator", sizeof("officeSenator"), 1, 0x9999);
	int officeCustomer = CreateMV("officeCustomer", sizeof("officeCustomer"), 1, 0x9999);	
	int senatorLock = ServerCreateLock("senatorLock", sizeof("senatorLock"), 1);
	int customerLock = ServerCreateLock("customerLock", sizeof("customerLock"), 1);

	/*Locks, conditions, and data used for waitinng */
	int clerkWaitLock = ServerCreateLock("clerkWaitLock", sizeof("clerkWaitLock"), 1);
	int clerkWaitCV = ServerCreateCV("clerkWaitCV", sizeof("clerkWaitCV"), 1);
	int numAppWait = CreateMV("numAppWait", sizeof("numAppWait"), 1, 0x9999);

	/* Reg and priv line lengths, line locks, and reg and priv line conditions. */
	int regACLineLength = CreateMV("regACLineLength", sizeof("regACLineLength"), 1, 0x9999);
	int privACLineLength = CreateMV("privACLineLength", sizeof("privACLineLength"), 1, 0x9999);
	int acpcLineLock = ServerCreateLock("acpcLineLock", sizeof("acpcLineLock"), 1);
	int regACLineCV = ServerCreateCV("regACLineCV", sizeof("regACLineCV"), 1);
	int privACLineCV = ServerCreateCV("privACLineCV", sizeof("privACLineCV"), 1);


	/* Individual clerk locks, conditions, data, data booleans, and states */
	int appLock = ServerCreateLock("appLock", sizeof("appLock"), NUM_CLERKS);
	int appCV = ServerCreateCV("appCV", sizeof("appCV"), NUM_CLERKS);

	int appState = CreateMV("appState", sizeof("appState"), NUM_CLERKS, 0x9999);
	int appData = CreateMV("appData", sizeof("appData"), NUM_CLERKS, 0x9999);

	/* Money data and locks for each clerk type */
	int appMoney = CreateMV("appMoney", sizeof("appMoney"), 1, 0x9999);
	int appMoneyLock = ServerCreateLock("appMoneyLock", sizeof("appMoneyLock"), 1);

	/* Individual customer's file state, type, and its lock */
	int fileLock = ServerCreateLock("fileLock", sizeof("fileLock"), NUM_CUSTOMERS + NUM_SENATORS);
	int fileState = CreateMV("fileState", sizeof("fileState"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	int fileType = CreateMV("fileType", sizeof("fileType"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);

	/* initializes myIndex */
	ServerAcquire(initIndexLock, 0);
	myIndex = GetMV(initIndex, 0);
	SetMV(initIndex, 0, GetMV(initIndex, 0) + 1);
	ServerRelease(initIndexLock, 0);
	
	SetMV(appState, myIndex, 1);

	/* --------------------BEGIN APPCLERK STUFF----------------*/
	while(loop == true){
		if (GetMV(shutdown, 0) == 1) {
/*			ClerkTrace("App", myIndex, 0x00, 0, "Shutting down.\n"); 
*/			Exit(0);
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
			Trace("Checking privLine\n", 0x9999);
			SetMV(privACLineLength, 0, GetMV(privACLineLength, 0)-1);

			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0)+1);	/* shows customer that one clerk is waiting */
			ServerRelease(senatorLock, 0);

			ServerAcquire(appLock, myIndex);
			SetMV(appState, myIndex, 0);	/* 0 = AVAILABLE */
			ServerSignal(privACLineCV, 0, acpcLineLock, 0); /* Signals the next customer in priv line */
			ServerRelease(acpcLineLock, 0);
			Trace("Waiting privLine\n", 0x9999);
			ServerWait(appCV, myIndex, appLock, myIndex); /* Waits for the next customer */

			mySSN = GetMV(appData, myIndex);
			ServerAcquire(fileLock, mySSN);

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
/*				ClerkTrace("App", myIndex, "Cust", mySSN,
					"ERROR:Customer doesn't have a picture application or no application. What are you doing here?\n");
*/				ServerRelease(fileLock, mySSN);
			}

			for (i = 0; i < 20; i++){
				Yield();
			}
/*
			if(cType == 0){
				ClerkTrace("App", myIndex, "Cust", mySSN, "Customer's app has been filed.\n");
			}
			else{
				ClerkTrace("App", myIndex, "Sen", mySSN, "Senator's app has been filed.\n");
			}
*/
			ServerAcquire(appMoneyLock, 0);
			SetMV(appMoney, 0, GetMV(appMoney, 0) + 500);
/*
			if(cType == CUSTOMER){
				ClerkTrace("App", myIndex, "Cust", mySSN, "Accepts $500 from Customer.\n");
			}
			else{
				ClerkTrace("App", myIndex, "Sen", mySSN, "Accepts $500 from Senator.\n");
			}
*/
			ServerRelease(appMoneyLock, 0);
			ServerSignal(appCV, myIndex, appLock, myIndex); /* signal customer awake */
			ServerRelease(appLock, myIndex);	/* Release clerk lock */

		}	
		/* Check for regular customer line next.
		* If there are regular customers, do AppClerk tasks 
		*/
		else if(GetMV(regACLineLength,0) > 0){
			Trace("Checking regLine\n", 0x9999);
			SetMV(regACLineLength, 0, GetMV(regACLineLength, 0)-1);

			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0)+1);	/* shows customer that one clerk is waiting */
			ServerRelease(senatorLock, 0);

			ServerAcquire(appLock, myIndex);
			SetMV(appState, myIndex, 0); /* 0 = AVAILABLE */
			ServerSignal(regACLineCV, 0, acpcLineLock, 0); /* Signals the next customer in priv line */
			ServerRelease(acpcLineLock, 0);
			Trace("Waiting regLine\n", 0x9999);
			ServerWait(appCV, myIndex, appLock, myIndex); /* Waits for the next customer */

			mySSN = GetMV(appData, myIndex);
			ServerAcquire(fileLock, mySSN);

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
/*				ClerkTrace("App", myIndex, "Cust", mySSN,
					"ERROR:Customer doesn't have a picture application or no application. What are you doing here?\n");
*/				ServerRelease(fileLock, mySSN);
			}

			for (i = 0; i < 20; i++){
				Yield();
			}
/*
			if(cType == CUSTOMER){
				ClerkTrace("App", myIndex, "Cust", mySSN, "Informs Customer their app has been filed.\n");
			}
			else{
				ClerkTrace("App", myIndex, "Sen", mySSN, "Informs Senator their app has been filed.\n");
			}
*/
			ServerSignal(appCV, myIndex, appLock, myIndex); /* signal customer awake */
			ServerRelease(appLock, myIndex);	/* Release clerk lock */
		}
		/* If there are neither privileged or regular customers, go on break */
		else{
			Trace("Going on break\n", 0x9999);
			ServerRelease(acpcLineLock, 0);
			ServerAcquire(appLock, myIndex);
			SetMV(appState, myIndex, 2); /* 2 = BREAK */
/*			ClerkTrace("App", myIndex, 0x00, 0, "Going on break.\n");
*/
			ServerWait(appCV, myIndex, appLock, myIndex);
/*			ClerkTrace("App", myIndex, 0x00, 0, "Returning from break.\n");
*/			ServerRelease(appLock, myIndex);
		}
	}
}