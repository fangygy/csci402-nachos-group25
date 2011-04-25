/* PicClerk.c
 */

#include "syscall.h"
#define NUM_CLERKS 5
#define NUM_CUSTOMERS 30
#define NUM_SENATORS 5
#define NV 0x9999

/*
*	Yinlerthai Chan:
*	Picture Clerk function thread
*	Takes a "picture" of the Customer
*	If the Customer dislikes the picture, will continue to take pictures until the Customer approves it
*	Once approved, the Picture Clerk will file the picture, then dismiss the Customer
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

	int initIndex = CreateMV("PiIn", sizeof("PiIn"), 1, 0x9999);
	int initIndexLock = ServerCreateLock("PiInLk", sizeof("PiInLk"), 1);

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
	int numPicWait = CreateMV("NuPiWa", sizeof("NuPiWa"), 1, 0x9999);

	/* Reg and priv line lengths, line locks, and reg and priv line conditions. */
	int regPCLineLength = CreateMV("RgPiLn", sizeof("RgPiLn"), 1, 0x9999);
	int privPCLineLength = CreateMV("PrPiLn", sizeof("PrPiLn"), 1, 0x9999);
	int acpcLineLock = ServerCreateLock("ACPCLk", sizeof("ACPCLk"), 1);
	int regPCLineCV = ServerCreateCV("RgPiCV", sizeof("RgPiCV"), 1);
	int privPCLineCV = ServerCreateCV("PrPiCV", sizeof("PrPiCV"), 1);


	/* Individual clerk locks, conditions, data, data booleans, and states */
	int picLock = ServerCreateLock("PiLk", sizeof("PiLk"), NUM_CLERKS);
	int picCV = ServerCreateCV("PiCV", sizeof("PiCV"), NUM_CLERKS);
	int picDataBool = CreateMV("PiDaBo", sizeof("PiDaBo"), NUM_CLERKS, 0x9999);
	int picState = CreateMV("PiSt", sizeof("PiSt"), NUM_CLERKS, 0x9999);
	int picData = CreateMV("PiDa", sizeof("PiDa"), NUM_CLERKS, 0x9999);

	/* Money data and locks for each clerk type */
	int picMoney = CreateMV("PiMn", sizeof("PiMn"), 1, 0x9999);
	int picMoneyLock = ServerCreateLock("PiMnLk", sizeof("PiMnLk"), 1);

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
	
	SetMV(picState, myIndex, 1);
	Write("PicClerk starting up\n", sizeof("PicClerk starting up\n"), ConsoleOutput);
	Write("PicClerk about to loop forever\n", sizeof("PicClerk about to loop forever\n"), ConsoleOutput);
	/* --------------------BEGIN PICCLERK STUFF----------------*/
	while(loop == true){
		Write("PicClerk starting loop\n", sizeof("PicClerk starting loop\n"), ConsoleOutput);

		if (GetMV(shutdown, 0) == 1) {
			ServerAcquire(traceLock, 0);
			ClerkTrace("Pic", myIndex, 0x00, 0, "Shutting down.\n");
			ServerRelease(traceLock, 0);
			Exit(0);
		}
		ServerAcquire(senatorLock, 0);
		ServerAcquire(customerLock, 0);
		if (GetMV(officeSenator, 0) > 0 && GetMV(officeCustomer, 0) > 0) {
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
		* If there are privileged customers, do PicClerk tasks, then receive $500
		*/
		if(GetMV(privPCLineLength, 0) > 0){
			Write("PicClerk checking priv line\n", sizeof("PicClerk checking priv line\n"), ConsoleOutput);
			SetMV(privPCLineLength, 0, GetMV(privPCLineLength, 0)-1);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numPicWait, 0, GetMV(numPicWait, 0)+1);		/* Shows customer that one clerk is waiting */
			ServerRelease(senatorLock, 0);

			ServerAcquire(picLock, myIndex);
			SetMV(picState, myIndex, 0); /* 0 = AVAILABLE */
			ServerSignal(privPCLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerWait(picCV, myIndex, picLock, myIndex);	/* Waits for the next customer */

			mySSN = GetMV(picData, myIndex);
			Write("PicClerk gotten SSN from priv customer\n", sizeof("PicClerk gotten SSN from priv customer\n"), ConsoleOutput);
			/* check the customer type */
			ServerAcquire(fileLock, mySSN);
			if (GetMV(fileType, mySSN) == 0) { /* 0 = CUSTOMER */
				cType = 0;
			} else {
				cType = 1;	/* 1 = SENATOR */
			}

			if(GetMV(fileState, mySSN) == 0){	/* 0 = NONE */
				SetMV(fileState, mySSN, 1);/* 1 = PICDONE */
				ServerRelease(fileLock, mySSN);
			}
			else if(GetMV(fileState, mySSN) == 2 ){ /* 2 = APPDONE */
				SetMV(fileState, mySSN, 3); /* 3 = APPPICDONE */
				ServerRelease(fileLock, mySSN);
			}
			else{
				ServerAcquire(traceLock, 0);
				ClerkTrace("Pic", myIndex, "Cust", mySSN,
					"ERROR: Customer does not have either an application or no application. What are you doing here?\n");
				ServerRelease(traceLock, 0);
				ServerRelease(fileLock, mySSN);
			}

			ServerAcquire(traceLock, 0);
			if (cType == 0) {
				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Takes picture of Customer.\n");
			} else {
				ClerkTrace("Pic", myIndex, "Sen", mySSN, "Takes picture of Senator.\n");
			}
			ServerRelease(traceLock, 0);

			/* yield to take picture
			* print statement: "Taking picture"
			* picCV->Signal then picCV->Wait to
			* show customer picture
			*/
			while(GetMV(picDataBool, myIndex) == 0){ /* 0 = false */
				for(i = 0; i < 4; i++){
					Yield();
				}
				ServerSignal(picCV, myIndex, picLock, myIndex);	
				ServerWait(picCV, myIndex, picLock, myIndex);	/* Shows the customer the picture */

				if(GetMV(picDataBool, myIndex) == 0){
					ServerAcquire(traceLock, 0);
					if (cType == 0) {
						ClerkTrace("Pic", myIndex, "Cust", mySSN, "Takes picture of Customer again.\n");

					} else {
						ClerkTrace("Pic", myIndex, "Sen", mySSN, "Takes picture of Senator again.\n");
					}
					ServerRelease(traceLock, 0);
				}
			}


			/* file picture using
			* current thread yield
			
			for(i = 0; i < 20; i++){
				Yield();
			}*/
			ServerAcquire(traceLock, 0);
			if (cType == 0) {
				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Informs Customer that the picture has been completed.\n");
			} else {
				ClerkTrace("Pic", myIndex, "Sen", mySSN, "Informs Senator that the picture has been completed.\n");
			}
			ServerRelease(traceLock, 0);

			/* signal customer awake */
			ServerAcquire(picMoneyLock, 0);
			SetMV(picMoney, 0, GetMV(picMoney, 0) + 500);

			ServerAcquire(traceLock, 0);
			if (cType == 0) {
				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Accepts $500 from Customer.\n");
			} else {
				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Accepts $500 from Senator.\n");
			}
			ServerRelease(traceLock, 0);

			ServerRelease(picMoneyLock, 0);

			SetMV(picDataBool, myIndex, 0);
			ServerSignal(picCV, myIndex, picLock, myIndex); /* signal customer awake */
			ServerRelease(picLock, myIndex); /* release clerk lock */
			Write("PicClerk done with priv customer\n", sizeof("PicClerk done with priv customer\n"), ConsoleOutput);
		}
		/* Check for regular customer line next
		* If there are regular customers, do PicClerk tasks
		*/
		else if(GetMV(regPCLineLength,0) > 0){
			Write("PicClerk checking reg line\n", sizeof("PicClerk checking reg line\n"), ConsoleOutput);
			SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0)-1);

			ServerAcquire(senatorLock, 0);
			SetMV(numPicWait, 0, GetMV(numPicWait, 0)+1);	/* shows customer that one clerk is waiting */
			ServerRelease(senatorLock, 0);

			ServerAcquire(picLock, myIndex);
			SetMV(picState, myIndex, 0); /* 0  = AVAILABLE */
			ServerSignal(regPCLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerWait(picCV, myIndex, picLock, myIndex);	/* Waits for the next customer */

			mySSN = GetMV(picData, myIndex);
			Write("PicClerk gotten SSN from reg customer\n", sizeof("PicClerk gotten SSN from reg customer\n"), ConsoleOutput);
			/* check the customer type */
			ServerAcquire(fileLock, mySSN);
			if (GetMV(fileType, mySSN) == 0) {
				cType = 0;
			} else {
				cType = 1;
			}

			if(GetMV(fileState, mySSN) == 0){	/* 0 = NONE */
				SetMV(fileState, mySSN, 1);/* 1 = PICDONE */
				ServerRelease(fileLock, mySSN);
			}
			else if(GetMV(fileState, mySSN) == 2 ){ /* 2 = APPDONE */
				SetMV(fileState, mySSN, 3); /* 3 = APPPICDONE */
				ServerRelease(fileLock, mySSN);
			}
			else{
				ServerAcquire(traceLock, 0);
				ClerkTrace("Pic", myIndex, "Cust", mySSN,
					"ERROR: Customer does not have either an application or no application. What are you doing here?\n");
				ServerRelease(traceLock, 0);
				ServerRelease(fileLock, mySSN);
			}

			ServerAcquire(traceLock, 0);
			if (cType == 0) {
				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Takes picture of Customer.\n");
			} else {
				ClerkTrace("Pic", myIndex, "Sen", mySSN, "Takes picture of Senator.\n");
			}
			ServerRelease(traceLock, 0);
			
				/* yield to take picture
				* print statement: "Taking picture"
				* picCV->Signal then picCV->Wait to
				* show customer picture
				*/
			while(GetMV(picDataBool, myIndex) == 0){
				for(i = 0; i < 4; i++){
					Yield();
				}
				ServerSignal(picCV, myIndex, picLock, myIndex);	
				ServerWait(picCV, myIndex, picLock, myIndex);	/* Shows the customer the picture */

				if(GetMV(picDataBool, myIndex) == 0){
					ServerAcquire(traceLock, 0);
					if (cType == 0) {
						ClerkTrace("Pic", myIndex, "Cust", mySSN, "Takes picture of Customer again.\n");
					} else {
						ClerkTrace("Pic", myIndex, "Sen", mySSN, "Takes picture of Senator again.\n");
					}
					ServerRelease(traceLock, 0);
				}
			}


			/* file picture using
			* current thread yield
			
			for(i = 0; i < 20; i++){
				Yield();
			}*/
			ServerAcquire(traceLock, 0);
			if (cType == 0) {
				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Informs Customer that the picture has been completed.\n");
			} else {
				ClerkTrace("Pic", myIndex, "Sen", mySSN, "Informs Senator that the picture has been completed.\n");
			}
			ServerRelease(traceLock, 0);

			SetMV(picDataBool, myIndex, 0);
			ServerSignal(picCV, myIndex, picLock, myIndex); /* signal customer awake */
			ServerRelease(picLock, myIndex); /* release clerk lock */
			Write("PicClerk done with reg customer\n", sizeof("PicClerk done with reg customer\n"), ConsoleOutput);
		}		
		/* If there are neither privileged or regular customers, go on break */
		else{
			Write("PicClerk going on break\n", sizeof("PicClerk going on break\n"), ConsoleOutput);
			ServerRelease(acpcLineLock, 0);
			ServerAcquire(picLock, myIndex);
			SetMV(picState, myIndex, 2); /* 2 = BREAK */
			
			ServerAcquire(traceLock, 0);
			ClerkTrace("Pic", myIndex, 0x00, 0, "Going on break.\n");
			ServerRelease(traceLock, 0);

			ServerWait(picCV, myIndex, picLock, myIndex);
			
			ServerAcquire(traceLock, 0);
			ClerkTrace("Pic", myIndex, 0x00, 0, "Returning from break.\n");
			ServerRelease(traceLock, 0);
			
			ServerRelease(picLock, myIndex);
		}
	}
}