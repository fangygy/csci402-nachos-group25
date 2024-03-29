/* PassClerk.c
 *	
 */

#include "syscall.h"
#define NUM_CLERKS 5
#define NUM_CUSTOMERS 30
#define NUM_SENATORS 5
#define NV 0x9999

/* booleans */
#define FALSE 0
#define TRUE 1

/* clerk states */	
#define AVAILABLE 0
#define BUSY 1
#define BREAK 2

/* customer states */
#define NONE 0
#define PICDONE 1
#define APPDONE 2
#define APPPICDONE 3
#define PASSDONE 4
#define ALLDONE 5

/* customer types */
#define CUSTOMER 0
#define SENATOR 1

/* MVs */
int myIndex;
int mySSN;
int i;

int officeCustomer;
int officeSenator;
int privPassLineLength;
int regPassLineLength;
int fileState;
int numPassWait;
int passDataBool;
int passMoney;

int cType;
int doPassport;
int loop;
int shutdown;

/* MV Indices */
int initIndex;
int officeCustomerIndex;
int officeSenatorIndex;
int regPassLineLengthIndex;
int privPassLineLengthIndex;
int numPassWaitIndex;
int passStateIndex;
int passDataIndex;
int fileTypeIndex;
int fileStateIndex;
int shutdownIndex;
int passMoneyIndex;
int passDataBoolIndex;

/* Locks */
int initIndexLock;
int senatorLock;
int customerLock;
int passLock;
int passLineLock;
int clerkWaitLock;
int fileLock;
int passMoneyLock;

/* CVs */
int clerkWaitCV;
int passCV;
int regPassLineCV;
int privPassLineCV;

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
	loop = TRUE;
	/* Handle Creation Stuff */
	
	/* gettin mah Clerk Index */
	initIndex = CreateMV("passIndex", sizeof("passIndex"), 1, 0x9999);
	initIndexLock = ServerCreateLock("passIndexLock", sizeof("passIndexLock"), 1);
	
	ServerAcquire(initIndexLock, 0);
	myIndex = GetMV(initIndex, 0);
	SetMV(initIndex, 0, myIndex + 1);
	ServerRelease(initIndexLock, 0);
	
	/* Locks */
	passLock = ServerCreateLock("passLock", sizeof("passLock"), NUM_CLERKS);
	senatorLock = ServerCreateLock("senatorLock", sizeof("senatorLock"), 1);
	customerLock = ServerCreateLock("customerLock", sizeof("customerLock"), 1);
	passLineLock = ServerCreateLock("passLineLock", sizeof("passLineLock"), 1);
	clerkWaitLock = ServerCreateLock("clerkWaitLock", sizeof("clerkWaitLock"), 1);
	fileLock = ServerCreateLock("fileLock", sizeof("fileLock"), NUM_CUSTOMERS + NUM_SENATORS);
	passMoneyLock = ServerCreateLock("passMoneyLock", sizeof("passMoneyLock"), 1);
	
	/* CVs */
	clerkWaitCV = ServerCreateCV("clerkWaitCV", sizeof("clerkWaitCV"), 1);
	passCV = ServerCreateCV("passCV", sizeof("passCV"), NUM_CLERKS);
	regPassLineCV = ServerCreateCV("regPassLineCV", sizeof("regPassLineCV"), 1);
	privPassLineCV = ServerCreateCV("privPassLineCV", sizeof("privPassLineCV"), 1);
	
	/* MVs */
	shutdownIndex = CreateMV("shutdown", sizeof("shutdown"), 1, 0x9999);
	officeSenatorIndex = CreateMV("officeSenator", sizeof("officeSenator"), 1, 0x9999);
	officeCustomerIndex = CreateMV("officeCustomer", sizeof("officeCustomer"), 1, 0x9999);
	regPassLineLengthIndex = CreateMV("regPassLineLength", sizeof("regPassLineLength"), 1, 0x9999);
	privPassLineLengthIndex = CreateMV("privPassLineLength", sizeof("privPassLineLength"), 1, 0x9999);
	numPassWaitIndex = CreateMV("numPassWait", sizeof("numPassWait"), 1, 0x9999);
	passDataIndex = CreateMV("passData", sizeof("passData"), NUM_CLERKS, 0x9999);
	passDataBoolIndex = CreateMV("passDataBool", sizeof("passDataBool"), NUM_CLERKS, 0x9999);
	passStateIndex = CreateMV("passState", sizeof("passState"), NUM_CLERKS, 0x9999);
	fileTypeIndex = CreateMV("fileType", sizeof("fileType"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	fileStateIndex = CreateMV("fileState", sizeof("fileState"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	passMoneyIndex = CreateMV("passMoney", sizeof("passMoney"), 1, 0x9999);
	
	traceLock = ServerCreateLock("traceLock", sizeof("traceLock"), 1);
	
	ServerAcquire(passLock, myIndex);
	SetMV(passStateIndex, myIndex, BUSY);
	ServerRelease(passLock, myIndex);
	
	while(loop == TRUE){
		shutdown = GetMV(shutdownIndex, 0);
		if (shutdown == TRUE) {
			ServerAcquire(traceLock, 0);
			ClerkTrace("Pass", myIndex, 0x00, 0, "Shutting down.\n");
			ServerRelease(traceLock, 0);
			Exit(0);
		}
		ServerAcquire(senatorLock, 0);		/* only one ServerLock, so acquire index 0? */
		ServerAcquire(customerLock, 0);
		
		officeSenator = GetMV(officeSenatorIndex, 0);
		officeCustomer = GetMV(officeCustomerIndex, 0);
		
		if(officeSenator > 0 && officeCustomer > 0){
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
		/* Check for customers in line */
		ServerAcquire(passLineLock, 0);
		
		privPassLineLength = GetMV(privPassLineLengthIndex, 0);
		regPassLineLength = GetMV(regPassLineLengthIndex, 0);
		if(privPassLineLength > 0){
		/* Decrement line length, set state to AVAIL, signal 1st customer and wait for them */
			privPassLineLength--;
			SetMV(privPassLineLengthIndex, 0, privPassLineLength);
			
			ServerAcquire(senatorLock, 0);
			numPassWait = GetMV(numPassWaitIndex, 0);
			numPassWait++;
			SetMV(numPassWaitIndex, 0, numPassWait);
			ServerRelease(senatorLock, 0);
			
			ServerAcquire(passLock, myIndex);
			SetMV(passStateIndex, myIndex, AVAILABLE);
			ServerSignal(privPassLineCV, 0, passLineLock, 0);
			ServerRelease(passLineLock, 0);
			
			/* wait for customer to signal me */
			ServerWait(passCV, myIndex, passLock, myIndex);
			
			/* customer gave me thier SSN index to check thier file */
			mySSN = GetMV(passDataIndex, myIndex);
			ServerAcquire(fileLock, mySSN);
			/* check customer type */
			
			cType = GetMV(fileTypeIndex, mySSN);
			
			fileState = GetMV(fileStateIndex, mySSN);
			if(fileState == APPPICDONE){
				SetMV(passDataBoolIndex, myIndex, TRUE);
				
				doPassport = TRUE;

				ServerAcquire(traceLock, 0);
				if(cType == CUSTOMER){
					ClerkTrace("Pass", myIndex, "Cust", mySSN, "Gives valid certification to Customer.\n");
				}
				else{
					ClerkTrace("Pass", myIndex, "Sen", mySSN, "Gives valid certification to Senator.\n");
				}
				ServerRelease(traceLock, 0);
			} else {
				SetMV(passDataBoolIndex, myIndex, FALSE);
				doPassport = FALSE;
				ServerAcquire(traceLock, 0);
				if(cType == CUSTOMER){
					ClerkTrace("Pass", myIndex, "Cust", mySSN, "Gives invalid certification to Customer.\n");
					ClerkTrace("Pass", myIndex, "Cust", mySSN, "Punishes Customer to wait.\n");		
				}
				else{
					ClerkTrace("Pass", myIndex, "Sen", mySSN, "Gives invalid certification to Senator.\n");
					ClerkTrace("Pass", myIndex, "Sen", mySSN, "Punishes Senator to wait.\n");
				}
				ServerRelease(traceLock, 0);
			}
			ServerRelease(fileLock, mySSN);

			/* add $500 to passClerk money amount for privileged fee */
			ServerAcquire(passMoneyLock, 0);
			passMoney = GetMV(passMoneyIndex, 0);
			passMoney += 500;
			SetMV(passMoneyIndex, 0, passMoney);
			ServerRelease(passMoneyLock, 0);

			ServerAcquire(traceLock, 0);
			if(cType == CUSTOMER){
				ClerkTrace("Pass", myIndex, "Cust", mySSN, "Accepts $500 from Customer.\n");
				ClerkTrace("Pass", myIndex, "Cust", mySSN, "Informs Customer that the procedure has been completed.\n");
			}
			else{
				ClerkTrace("Pass", myIndex, "Sen", mySSN, "Accepts $500 from Senator.\n");
				ClerkTrace("Pass", myIndex, "Sen", mySSN, "Informs Senator that the produre has been completed.\n");
			}
			ServerRelease(traceLock, 0);

			/* signal customer awake */
			ServerSignal(passCV, myIndex, passLock, myIndex);
			ServerRelease(passLock, myIndex);				/* release clerk lock */

			if(doPassport == TRUE){
				for(i = 0; i < 20; i++){
					/*Yield();	*/
				}

				ServerAcquire(fileLock, mySSN);
				SetMV(fileStateIndex, mySSN, PASSDONE);
				ServerRelease(fileLock, mySSN);
				
				ServerAcquire(traceLock, 0);
				if(cType == CUSTOMER){
					ClerkTrace("Pass", myIndex, "Cust", mySSN, "Finished filing Customer's passport.\n");
				}
				else{
					ClerkTrace("Pass", myIndex, "Sen", mySSN, "Finished filing Senator's passport.\n");
				}
				ServerRelease(traceLock, 0);
			}
			
			doPassport = FALSE;
			mySSN = 0;
		}

		else if (regPassLineLength > 0){
		/* Decrement line length, set state to AVAIL, signal 1st customer and wait for them */
			regPassLineLength--;
			SetMV(regPassLineLengthIndex, 0, regPassLineLength);
			
			ServerAcquire(senatorLock, 0);
			numPassWait = GetMV(numPassWaitIndex, 0);
			numPassWait++;
			SetMV(numPassWaitIndex, 0, numPassWait);
			ServerRelease(senatorLock, 0);
			
			ServerAcquire(passLock, myIndex);
			SetMV(passStateIndex,  myIndex, AVAILABLE);
			ServerSignal(regPassLineCV, 0, passLineLock, 0);
			ServerRelease(passLineLock, 0);
			ServerWait(passCV, myIndex, passLock, myIndex); /* wait for customer to signal me */

			/* customer gave me thier SSN index to check thier file */
			mySSN = GetMV(passDataIndex, myIndex);
			ServerAcquire(fileLock, mySSN);
			/* check customer type */
			cType = GetMV(fileTypeIndex, mySSN);
			fileState = GetMV(fileStateIndex, mySSN);
			
			if(fileState == APPPICDONE){
				SetMV(passDataBoolIndex, myIndex, TRUE);
				doPassport = TRUE;

				ServerAcquire(traceLock, 0);
				if(cType == CUSTOMER){
					ClerkTrace("Pass", myIndex, "Cust", mySSN, "Gives valid certification to Customer.\n");
				}
				else{
					ClerkTrace("Pass", myIndex, "Sen", mySSN, "Gives valid certification to Senator.\n");
				}
				ServerRelease(traceLock, 0);
			} else {
				SetMV(passDataBoolIndex, myIndex, FALSE);
				doPassport = FALSE;
				/*Trace("CUSTOMER'S STATE WAS ", fileState);
				Trace("\n", 0x9999);*/
				
				ServerAcquire(traceLock, 0);
				if(cType == CUSTOMER){
					ClerkTrace("Pass", myIndex, "Cust", mySSN, "Gives invalid certification to Customer.\n");
					ClerkTrace("Pass", myIndex, "Cust", mySSN, "Punishes Customer to wait.\n");	
				}
				else{
					ClerkTrace("Pass", myIndex, "Sen", mySSN, "Gives invalid certification to Senator.\n");
					ClerkTrace("Pass", myIndex, "Sen", mySSN, "Punishes Senator to wait.\n");
				}
				ServerRelease(traceLock, 0);
			}
			
			ServerAcquire(traceLock, 0);
			if(cType == CUSTOMER){
				ClerkTrace("Pass", myIndex, "Cust", mySSN, "Informs Customer that the procedure has been completed.\n");
			}
			else{
				ClerkTrace("Pass", myIndex, "Sen", mySSN, "Informs Senator that the procedure has been completed.\n");
			}
			ServerRelease(traceLock, 0);

			ServerRelease(fileLock, mySSN);

			ServerSignal(passCV, myIndex, passLock, myIndex); /* signal customer awake */
			ServerRelease(passLock, myIndex);				/* release clerk lock */

			if(doPassport){
				for(i = 0; i < 20; i++){
					/*Yield();*/
				}

				ServerAcquire(fileLock, mySSN);
				SetMV(fileStateIndex, mySSN, PASSDONE);
				ServerRelease(fileLock, mySSN);
				
				ServerAcquire(traceLock, 0);
				if(cType == CUSTOMER){
					ClerkTrace("Pass", myIndex, "Cust", mySSN, "Finished filing Customer's passport.\n");
				}
				else{
					ClerkTrace("Pass", myIndex, "Sen", mySSN, "Finished filing Senator's passport.\n");
				}
				ServerRelease(traceLock, 0);
			}
			doPassport = FALSE;
			mySSN = 0;
		}
		else{
			/* No one in line...take a break */
			ServerRelease(passLineLock, 0);
			ServerAcquire(passLock, myIndex);
			
			ServerAcquire(traceLock, 0);
			ClerkTrace("Pass", myIndex, 0x00, 0, "Goin on break.\n");
			ServerRelease(traceLock, 0);
			
			SetMV(passStateIndex, myIndex, BREAK);
			ServerWait(passCV, myIndex, passLock, myIndex);
			
			ServerAcquire(traceLock, 0);
			ClerkTrace("Pass", myIndex, 0x00, 0, "Returning from break.\n");
			ServerRelease(traceLock, 0);

			ServerRelease(passLock, myIndex);
		}
	}
}