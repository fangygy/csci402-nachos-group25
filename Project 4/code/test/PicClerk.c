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

int main() {
	/* ------------------Local data -------------------------*/
	int myIndex;

	int initIndex = CreateMV("PicIndex", sizeof("PicIndex"), 1, 0x9999);
	int initIndexLock = ServerCreateLock("PicIndexLock", sizeof("PicIndexLock"), 1);

	int mySSN;
	int i;
	int cType = 0;

	enum BOOLEAN loop = true;
	enum BOOLEAN shutdown;
	/* -----------------Shared Data------------------------*/

	/* Number of customers/senators in office and thier locks */
	int officeSenator = CreateMV("officeSenator", sizeof("officeSenator"), 1, 0x9999);
	int officeCustomer = CreateMV("officeCustomer", sizeof("officeCustomer"), 1, 0x9999);	
	int senatorLock = ServerCreateLock("senatorLock", sizeof("senatorLock"), 1);
	int customerLock = ServerCreateLock("customerLock", sizeof("customerLock"), 1);

	/*Locks, conditions, and data used for waitinng */
	int clerkWaitLock = ServerCreateLock("clerkWaitLock", sizeof("clerkWaitLock"), 1);
	int clerkWaitCV = ServerCreateCV("clerkWaitCV", sizeof("clerkWaitCV"), 1);
	int numPicWait = CreateMV("numPicWait", sizeof("numPicWait"), 1, 0x9999);

	/* Reg and priv line lengths, line locks, and reg and priv line conditions. */
	int regPCLineLength = CreateMV("regPCLineLength", sizeof("regPCLineLength"), 1, 0x9999);
	int privPCLineLength = CreateMV("privPCLineLength", sizeof("privPCLineLength"), 1, 0x9999);
	int acpcLineLock = ServerCreateLock("acpcLineLock", sizeof("acpcLineLock"), 1);
	int regPCLineCV = ServerCreateCV("regPCLineCV", sizeof("regPCLineCV"), 1);
	int privPCLineCV = ServerCreateCV("privPCLineCV", sizeof("privPCLineCV"), 1);


	/* Individual clerk locks, conditions, data, data booleans, and states */
	int picLock = ServerCreateLock("picLock", sizeof("picLock"), NUM_CLERKS);
	int picCV = ServerCreateCV("picCV", sizeof("picCV"), NUM_CLERKS);
	int picDataBool = CreateMV("picDataBool", sizeof("picDataBool"), NUM_CLERKS, 0x9999);
	int picState = CreateMV("picState", sizeof("picState"), NUM_CLERKS, 0x9999);
	int picData = CreateMV("picData", sizeof("picData"), NUM_CLERKS, 0x9999);

	/* Money data and locks for each clerk type */
	int picMoney = CreateMV("picMoney", sizeof("picMoney"), 1, 0x9999);
	int picMoneyLock = ServerCreateLock("picMoneyLock", sizeof("picMoneyLock"), 1);

	/* Individual customer's file state, type, and its lock */
	int fileLock = ServerCreateLock("fileLock", sizeof("fileLock"), NUM_CUSTOMERS + NUM_SENATORS);
	int fileState = CreateMV("fileState", sizeof("fileState"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	int fileType = CreateMV("fileType", sizeof("fileType"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);

	/* initializes myIndex */
	ServerAcquire(initIndexLock, 0);
	myIndex = GetMV(initIndex, 0);
	SetMV(initIndex, 0, GetMV(initIndex, 0) + 1);
	ServerRelease(initIndexLock, 0);

	/* --------------------BEGIN PICCLERK STUFF----------------*/
	while(loop == true){
		if (shutdown == true) {
/*			ClerkTrace("Pic", myIndex, 0x00, 0, "Shutting down.\n");
*/			Exit(0);
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
/*				ClerkTrace("Pic", myIndex, "Cust", mySSN,
					"ERROR: Customer does not have either an application or no application. What are you doing here?\n");
*/				ServerRelease(fileLock, mySSN);
			}
/*
			if (cType == CUSTOMER) {
				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Takes picture of Customer.\n");
			} else {
				ClerkTrace("Pic", myIndex, "Sen", mySSN, "Takes picture of Senator.\n");
			}*/

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
					if (cType == 0) {
/*						ClerkTrace("Pic", myIndex, "Cust", mySSN, "Takes picture of Customer again.\n");
*/
					} else {
/*						ClerkTrace("Pic", myIndex, "Sen", mySSN, "Takes picture of Senator again.\n");
*/					}
				}
			}


			/* file picture using
			* current thread yield
			
			for(i = 0; i < 20; i++){
				Yield();
			}*/
			if (cType == 0) {
/*				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Informs Customer that the picture has been completed.\n");
*/			} else {
/*				ClerkTrace("Pic", myIndex, "Sen", mySSN, "Informs Senator that the picture has been completed.\n");
*/			}

			/* signal customer awake */
			ServerAcquire(picMoneyLock, 0);
			SetMV(picMoney, 0, GetMV(picMoney, 0) + 500);

			if (cType == 0) {
/*				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Accepts $500 from Customer.\n");
*/			} else {
/*				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Accepts $500 from Senator.\n");
*/			}

			ServerRelease(picMoneyLock, 0);

			SetMV(picDataBool, myIndex, 0);
			ServerSignal(picCV, myIndex, picLock, myIndex); /* signal customer awake */
			ServerRelease(picLock, myIndex); /* release clerk lock */
		}
		/* Check for regular customer line next
		* If there are regular customers, do PicClerk tasks
		*/
		else if(GetMV(regPCLineLength,0) > 0){
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
/*				ClerkTrace("Pic", myIndex, "Cust", mySSN,
					"ERROR: Customer does not have either an application or no application. What are you doing here?\n");
*/				ServerRelease(fileLock, mySSN);
			}

			if (cType == 0) {
/*				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Takes picture of Customer.\n");
*/			} else {
/*				ClerkTrace("Pic", myIndex, "Sen", mySSN, "Takes picture of Senator.\n");
*/			}
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
					if (cType == 0) {
/*						ClerkTrace("Pic", myIndex, "Cust", mySSN, "Takes picture of Customer again.\n");
*/					} else {
/*						ClerkTrace("Pic", myIndex, "Sen", mySSN, "Takes picture of Senator again.\n");
*/					}
				}
			}


			/* file picture using
			* current thread yield
			
			for(i = 0; i < 20; i++){
				Yield();
			}*/
			if (cType == 0) {
/*				ClerkTrace("Pic", myIndex, "Cust", mySSN, "Informs Customer that the picture has been completed.\n");
*/			} else {
/*				ClerkTrace("Pic", myIndex, "Sen", mySSN, "Informs Senator that the picture has been completed.\n");
*/			}

			SetMV(picDataBool, myIndex, 0);
			ServerSignal(picCV, myIndex, picLock, myIndex); /* signal customer awake */
			ServerRelease(picLock, myIndex); /* release clerk lock */
		}		
		/* If there are neither privileged or regular customers, go on break */
		else{
			ServerRelease(acpcLineLock, 0);
			ServerAcquire(picLock, myIndex);
			SetMV(picState, myIndex, 2); /* 2 = BREAK */
/*			ClerkTrace("Pic", myIndex, 0x00, 0, "Going on break.\n");
*/
			ServerWait(picCV, myIndex, picLock, myIndex);
/*			ClerkTrace("Pic", myIndex, 0x00, 0, "Returning from break.\n");
*/			ServerRelease(picLock, myIndex);
		}
	}
}