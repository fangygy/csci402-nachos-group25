/* CashClerk.c
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
int cashLineLength;
int fileState;
int numCashWait;
int cashDataBool;
int cashMoney;

int cType;
int loop;
int shutdown;

/* MV Indices */
int initIndex;
int officeCustomerIndex;
int officeSenatorIndex;
int cashLineLengthIndex;
int numCashWaitIndex;
int cashStateIndex;
int cashDataIndex;
int fileTypeIndex;
int fileStateIndex;
int shutdownIndex;
int cashMoneyIndex;
int cashDataBoolIndex;

/* Locks */
int initIndexLock;
int senatorLock;
int customerLock;
int cashLock;
int cashLineLock;
int clerkWaitLock;
int fileLock;
int cashMoneyLock;

/* CVs */
int clerkWaitCV;
int cashCV;
/*
int regCashLineCV;
int privCashLineCV;*/
int cashLineCV;
	
int main() {
	loop = TRUE;
	/* Handle Creation Stuff */
	
	/* gettin mah Clerk Index */
	initIndex = CreateMV("cashIndex", sizeof("cashIndex"), 1, 0x9999);
	initIndexLock = ServerCreateLock("cashIndexLock", sizeof("cashIndexLock"), 1);
	
	ServerAcquire(initIndexLock, 0);
	myIndex = GetMV(initIndex, 0);
	SetMV(initIndex, 0, myIndex + 1);
	ServerRelease(initIndexLock, 0);
	
	/* Locks */
	cashLock = ServerCreateLock("cashLock", sizeof("cashLock"), NUM_CLERKS);
	senatorLock = ServerCreateLock("senatorLock", sizeof("senatorLock"), 1);
	customerLock = ServerCreateLock("customerLock", sizeof("customerLock"), 1);
	cashLineLock = ServerCreateLock("cashLineLock", sizeof("cashLineLock"), 1);
	clerkWaitLock = ServerCreateLock("clerkWaitLock", sizeof("clerkWaitLock"), 1);
	fileLock = ServerCreateLock("fileLock", sizeof("fileLock"), NUM_CUSTOMERS + NUM_SENATORS);
	cashMoneyLock = ServerCreateLock("cashMoneyLock", sizeof("cashMoneyLock"), 1);
	
	/* CVs */
	clerkWaitCV = ServerCreateCV("clerkWaitCV", sizeof("clerkWaitCV"), 1);
	cashCV = ServerCreateCV("cashCV", sizeof("cashCV"), NUM_CLERKS);
	/*regCashLineCV = ServerCreateCV("regCashLineCV", sizeof("regCashLineCV"), 1);
	privCashLineCV = ServerCreateCV("privCashLineCV", sizeof("privCashLineCV"), 1);*/
	cashLineCV = ServerCreateCV("cashLineCV", sizeof("cashLineCV"), 1);
	
	/* MVs */
	shutdownIndex = CreateMV("shutdown", sizeof("shutdown"), 1, 0x9999);
	officeSenatorIndex = CreateMV("officeSenator", sizeof("officeSenator"), 1, 0x9999);
	officeCustomerIndex = CreateMV("officeCustomer", sizeof("officeCustomer"), 1, 0x9999);
	cashLineLengthIndex = CreateMV("cashLineLength", sizeof("cashLineLength"), 1, 0x9999);
	numCashWaitIndex = CreateMV("numCashWait", sizeof("numCashWait"), 1, 0x9999);
	cashDataIndex = CreateMV("cashData", sizeof("cashData"), NUM_CLERKS, 0x9999);
	cashDataBoolIndex = CreateMV("cashDataBool", sizeof("cashDataBool"), NUM_CLERKS, 0x9999);
	cashStateIndex = CreateMV("cashState", sizeof("cashState"), NUM_CLERKS, 0x9999);
	fileTypeIndex = CreateMV("fileType", sizeof("fileType"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	fileStateIndex = CreateMV("fileState", sizeof("fileState"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	cashMoneyIndex = CreateMV("cashMoney", sizeof("cashMoney"), 1, 0x9999);
	
	ServerAcquire(cashLock, myIndex);
	SetMV(cashStateIndex, myIndex, BUSY);
	ServerRelease(cashLock, myIndex);
	
	while(loop == TRUE){
		shutdown = GetMV(shutdownIndex, 0);
		if (shutdown == TRUE) {
			/*ClerkTrace("Cash", myIndex, 0x00, 0, "Shutting down.\n");*/
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
			ServerWait(clerkWaitCV, myIndex, clerkWaitLock, myIndex);
			ServerRelease(clerkWaitLock, 0);
		}
		else{
			ServerRelease(senatorLock, 0);
			ServerRelease(customerLock, 0);	
		}
		/* Check for customers in line */
		ServerAcquire(cashLineLock, 0);
		
		cashLineLength = GetMV(cashLineLengthIndex, 0);
		if(cashLineLength > 0){
		/* Decrement line length, set state to AVAIL, signal 1st customer and wait for them */
			cashLineLength--;
			SetMV(cashLineLengthIndex, 0, cashLineLength);
			
			ServerAcquire(senatorLock, 0);
			numCashWait = GetMV(numCashWaitIndex, 0);
			numCashWait++;
			SetMV(numCashWaitIndex, 0, numCashWait);
			ServerRelease(senatorLock, 0);
			
			ServerAcquire(cashLock, myIndex);
			SetMV(cashStateIndex, myIndex, AVAILABLE);
			ServerSignal(cashLineCV, myIndex, cashLineLock, myIndex);
			ServerRelease(cashLineLock, 0);
			
			/* wait for customer to signal me */
			ServerWait(cashCV, myIndex, cashLock, myIndex);
			
			/* customer gave me thier SSN index to check thier file */
			mySSN = GetMV(cashDataIndex, myIndex);
			ServerAcquire(fileLock, mySSN);
			/* check customer type */
			
			cType = GetMV(fileTypeIndex, mySSN);
			
			fileState = GetMV(fileStateIndex, mySSN);
			if(fileState == PASSDONE){
				SetMV(cashDataBoolIndex, myIndex, TRUE);
				
				/* YIELD??? */
				SetMV(fileStateIndex, mySSN, ALLDONE);
				
				ServerAcquire(cashMoneyLock, 0);
				cashMoney = GetMV(cashMoneyIndex, 0);
				cashMoney += 100;
				SetMV(cashMoneyIndex, 0, cashMoney);
				ServerRelease(cashMoneyLock, 0);

				if(cType == CUSTOMER){
					/*ClerkTrace("Cash", myIndex, "Cust", mySSN, "Gives valid certification to Customer.\n");*/
				}
				else{
					/*ClerkTrace("Cash", myIndex, "Sen", mySSN, "Gives valid certification to Senator.\n");*/
				}
			} else {
				SetMV(cashDataBoolIndex, myIndex, FALSE);
				if(cType == CUSTOMER){
					/*ClerkTrace("Cash", myIndex, "Cust", mySSN, "Gives invalid certification to Customer.\n");
					ClerkTrace("Cash", myIndex, "Cust", mySSN, "Punishes Customer to wait.\n");*/		
				}
				else{
					/*ClerkTrace("Cash", myIndex, "Sen", mySSN, "Gives invalid certification to Senator.\n");
					ClerkTrace("Cash", myIndex, "Sen", mySSN, "Punishes Senator to wait.\n");*/
				}
			}
			ServerRelease(fileLock, mySSN);
			
			ServerSignal(cashCV, myIndex, cashLock, myIndex);
			SetMV(cashStateIndex, myIndex, BUSY);
			ServerRelease(cashLock, myIndex);
			/* Yield? */
		}
		else{
			/* No one in line...take a break */
			ServerRelease(cashLineLock, 0);
			ServerAcquire(cashLock, myIndex);
			/*ClerkTrace("Cash", myIndex, 0x00, 0, "Goin on break.\n");*/
			SetMV(cashStateIndex, myIndex, BREAK);
			ServerWait(cashCV, myIndex, cashLock, myIndex);
			
			/*ClerkTrace("Cash", myIndex, 0x00, 0, "Returning from break.\n");*/
			ServerRelease(cashLock, myIndex);
		}
	}
}