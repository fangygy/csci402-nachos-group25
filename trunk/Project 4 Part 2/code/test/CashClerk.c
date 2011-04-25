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
	initIndex = CreateMV("CaIn", sizeof("CaIn"), 1, 0x9999);
	initIndexLock = ServerCreateLock("CaInLk", sizeof("CaInLk"), 1);
	
	ServerAcquire(initIndexLock, 0);
	myIndex = GetMV(initIndex, 0);
	SetMV(initIndex, 0, myIndex + 1);
	ServerRelease(initIndexLock, 0);
	
	/* Locks */
	cashLock = ServerCreateLock("CaLk", sizeof("CaLk"), NUM_CLERKS);
	senatorLock = ServerCreateLock("SeLk", sizeof("SeLk"), 1);
	customerLock = ServerCreateLock("CuLk", sizeof("CuLk"), 1);
	cashLineLock = ServerCreateLock("CaLiLk", sizeof("CaLiLk"), 1);
	clerkWaitLock = ServerCreateLock("ClWaLk", sizeof("ClWaLk"), 1);
	fileLock = ServerCreateLock("FiLk", sizeof("FiLk"), NUM_CUSTOMERS + NUM_SENATORS);
	cashMoneyLock = ServerCreateLock("CaMnLk", sizeof("CaMnLk"), 1);
	
	/* CVs */
	clerkWaitCV = ServerCreateCV("ClWaCV", sizeof("ClWaCV"), 1);
	cashCV = ServerCreateCV("CaCV", sizeof("CaCV"), NUM_CLERKS);
	/*regCashLineCV = ServerCreateCV("regCashLineCV", sizeof("regCashLineCV"), 1);
	privCashLineCV = ServerCreateCV("privCashLineCV", sizeof("privCashLineCV"), 1);*/
	cashLineCV = ServerCreateCV("CaLiCV", sizeof("CaLiCV"), 1);
	
	/* MVs */
	shutdownIndex = CreateMV("shut", sizeof("shut"), 1, 0x9999);
	officeSenatorIndex = CreateMV("OfSe", sizeof("OfSe"), 1, 0x9999);
	officeCustomerIndex = CreateMV("OfCu", sizeof("OfCu"), 1, 0x9999);
	cashLineLengthIndex = CreateMV("CaLn", sizeof("CaLn"), 1, 0x9999);
	numCashWaitIndex = CreateMV("NuCaWa", sizeof("NuCaWa"), 1, 0x9999);
	cashDataIndex = CreateMV("CaDa", sizeof("CaDa"), NUM_CLERKS, 0x9999);
	cashDataBoolIndex = CreateMV("CaDaBo", sizeof("CaDaBo"), NUM_CLERKS, 0x9999);
	cashStateIndex = CreateMV("CaSt", sizeof("CaSt"), NUM_CLERKS, 0x9999);
	fileTypeIndex = CreateMV("FiTp", sizeof("FiTp"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	fileStateIndex = CreateMV("FiSt", sizeof("FiSt"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	cashMoneyIndex = CreateMV("CaMn", sizeof("CaMn"), 1, 0x9999);
	
	traceLock = ServerCreateLock("trace", sizeof("trace"), 1);
	
	ServerAcquire(cashLock, myIndex);
	SetMV(cashStateIndex, myIndex, BUSY);
	ServerRelease(cashLock, myIndex);
	
	while(loop == TRUE){
		shutdown = GetMV(shutdownIndex, 0);
		if (shutdown == TRUE) {
		
			ServerAcquire(traceLock, 0);
			ClerkTrace("Cash", myIndex, 0x00, 0, "Shutting down.\n");
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
			ServerSignal(cashLineCV, 0, cashLineLock, 0);
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

				ServerAcquire(traceLock, 0);
				if(cType == CUSTOMER){
					ClerkTrace("Cash", myIndex, "Cust", mySSN, "Gives valid certification to Customer.\n");
				}
				else{
					ClerkTrace("Cash", myIndex, "Sen", mySSN, "Gives valid certification to Senator.\n");
				}
				ServerRelease(traceLock, 0);
				
			} else {
				SetMV(cashDataBoolIndex, myIndex, FALSE);
				
				ServerAcquire(traceLock, 0);
				if(cType == CUSTOMER){
					ClerkTrace("Cash", myIndex, "Cust", mySSN, "Gives invalid certification to Customer.\n");
					ClerkTrace("Cash", myIndex, "Cust", mySSN, "Punishes Customer to wait.\n");		
				}
				else{
					ClerkTrace("Cash", myIndex, "Sen", mySSN, "Gives invalid certification to Senator.\n");
					ClerkTrace("Cash", myIndex, "Sen", mySSN, "Punishes Senator to wait.\n");
				}
				ServerRelease(traceLock, 0);
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
			
			ServerAcquire(traceLock, 0);
			ClerkTrace("Cash", myIndex, 0x00, 0, "Goin on break.\n");
			ServerRelease(traceLock, 0);
			
			SetMV(cashStateIndex, myIndex, BREAK);
			ServerWait(cashCV, myIndex, cashLock, myIndex);
			
			ServerAcquire(traceLock, 0);
			ClerkTrace("Cash", myIndex, 0x00, 0, "Returning from break.\n");
			ServerRelease(traceLock, 0);
			
			ServerRelease(cashLock, myIndex);
		}
	}
}