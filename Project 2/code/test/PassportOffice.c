/* PassportOffice.c
 *	
 * The passport office
 *	
 */
 
#include "syscall.h"
#define NUM_CLERKS 3
#define NUM_CUSTOMERS 3
#define NUM_SENATORS 2

/* enum for booleans */
enum BOOLEAN {  
	false,
	true
};

/* enum for the state of a clerk */
enum CLERK_STATE {
	AVAILABLE,
	BUSY,
	BREAK
};

/* enum for the state of a customer/senator */
enum CUST_STATE {
	NONE,
	PIODONE,
	APPDONE,
	PICAPPDONE,
	PASSDONE,
	ALLDONE
};

/* enum for the type of a customer/senator */
enum CUST_TYPE {
	CUSTOMER,
	SENATOR
};

enum INDEX_USED {
	USED,
	FREE
};

/* Index initialization lock and data*/
int indexLock;
enum INDEX_USED customerIndex[NUM_CUSTOMERS + NUM_SENATORS], appClerkIndex[NUM_CLERKS], picClerkIndex[NUM_CLERKS],
	passClerkIndex[NUM_CLERKS], cashClerkIndex[NUM_CLERKS];
	
/* Number of customer/senators in office and their locks*/
int officeCustomer, waitingCustomer, officeSenator; 
int customerLock, senatorLock;

/* Locks, conditions, and data used for waiting */
int custWaitLock, senWaitLock, clerkWaitLock;
int custWaitCV, senWaitCV, clerkWaitCV; 
int numAppWait, numPicWait, numPassWait, numCashWait;

/* Reg and priv line lengths, line locks, and reg and priv line conditions. */
int regACLineLength, regPCLineLength, regPassLineLength;
int privACLineLength, privPCLineLength, privPassLineLength;
int cashLineLength;
int acpcLineLock, passLineLock, cashLineLock;
int regACLineCV, regPCLineCV, regPassLineCV;
int privACLineCV, privPCLineCV, privPassLineCV;
int cashLineCV; 

/* Individual clerk locks, conditions, data, data booleans, and states */
int appLock[NUM_CLERKS], picLock[NUM_CLERKS], passLock[NUM_CLERKS], cashLock[NUM_CLERKS];
int appCV[NUM_CLERKS], picCV[NUM_CLERKS], passCV[NUM_CLERKS], cashCV[NUM_CLERKS];
int appData[NUM_CLERKS], picData[NUM_CLERKS], passData[NUM_CLERKS], cashData[NUM_CLERKS];
enum BOOLEAN picDataBool[NUM_CLERKS], passDataBool[NUM_CLERKS], cashDataBool[NUM_CLERKS];
enum CLERK_STATE appState[NUM_CLERKS], picState[NUM_CLERKS], passState[NUM_CLERKS], cashState[NUM_CLERKS];

/* Money data and locks for each clerk type */
int appMoney, picMoney, passMoney, cashMoney;
int appMoneyLock, picMoneyLock, passMoneyLock, cashMoneyLock;

/* Individual customer's file state, type, and its lock - USED BY CLERKS*/
int fileLock[NUM_CUSTOMERS + NUM_SENATORS];
enum CUST_STATE fileState[NUM_CUSTOMERS + NUM_SENATORS];
enum CUST_TYPE fileType[NUM_CUSTOMERS + NUM_SENATORS];

/* Individual customer information - USED BY CUSTOMERS */
int myCustMoney[NUM_CUSTOMERS + NUM_SENATORS];
enum BOOLEAN visitedApp[NUM_CUSTOMERS + NUM_SENATORS], visitedPic[NUM_CUSTOMERS + NUM_SENATORS],
	visitedPass[NUM_CUSTOMERS + NUM_SENATORS], visitedCash[NUM_CUSTOMERS + NUM_SENATORS];

void InitializeData() {
	int i;
	
	indexLock = CreateLock("IndexLock", sizeof("IndexLock"));
	
	officeCustomer = 0;
	waitingCustomer = 0;
	officeSenator = 0;
	customerLock = CreateLock("CustomerLock", sizeof("CustomerLock"));
	senatorLock = CreateLock("SenatorLock", sizeof("SenatorLock"));
	
	custWaitLock = CreateLock("CustWaitLock", sizeof("CustWaitLock"));
	senWaitLock = CreateLock("SenWaitLock", sizeof("SenWaitLock"));
	clerkWaitLock = CreateLock("ClerkWaitLock", sizeof("ClerkWaitLock"));
	custWaitCV = CreateCondition("CustWaitCV", sizeof("CustWaitCV"));
	senWaitCV = CreateCondition("SenWaitCV", sizeof("SenWaitCV"));
	clerkWaitCV = CreateCondition("ClerkWaitCV", sizeof("ClerkWaitCV"));
	numAppWait = 0;
	numPicWait = 0;
	numPassWait = 0;
	numCashWait = 0;
	
	regACLineLength = 0;
	privACLineLength = 0;
	regPCLineLength = 0;
	privPCLineLength = 0;
	regPassLineLength = 0;
	privPassLineLength = 0;
	cashLineLength = 0;
	acpcLineLock = CreateLock("ACPCLineLock", sizeof("ACPCLineLock"));
	passLineLock = CreateLock("PassLineLock", sizeof("PassLineLock"));
	cashLineLock = CreateLock("CashLineLock", sizeof("CashLineLock"));
	regACLineCV = CreateCondition("RegACLineCV", sizeof("RegACLineCV"));
	privACLineCV = CreateCondition("PrivACLineCV", sizeof("PrivACLineCV"));
	regPCLineCV = CreateCondition("RegPCLineCV", sizeof("RegPCLineCV"));
	privPCLineCV = CreateCondition("PrivPCLineCV", sizeof("PrivPCLineCV"));
	regPassLineCV = CreateCondition("RegPassLineCV", sizeof("RegPassLineCV"));
	privPassLineCV = CreateCondition("PrivPassLineCV", sizeof("PrivPassLineCV"));
	cashLineCV = CreateCondition("CashLineCV", sizeof("CashLineCV"));
	
	for(i = 0; i < NUM_CLERKS; i++) {
		appLock[i] = CreateLock("AppLock",  sizeof("AppLock"));
		appCV[i] = CreateCondition("AppCV", sizeof("AppCV"));
		appData[i] = 0;
		appState[i] = BUSY;
		appClerkIndex[i] = FREE;
		
		picLock[i] = CreateLock("PicLock", sizeof("PicLock"));
		picCV[i] = CreateCondition("PicCV", sizeof("PicCV"));
		picData[i] = 0;
		picDataBool[i] = false;
		picState[i] = BUSY;
		picClerkIndex[i] = FREE;
		
		passLock[i] = CreateLock("PassLock", sizeof("PassLock"));
		passCV[i] = CreateCondition("PassCV", sizeof("PassCV"));
		passData[i] = 0;
		passDataBool[i] = false;
		passState[i] = BUSY;
		passClerkIndex[i] = FREE;
		
		cashLock[i] = CreateLock("CashLock", sizeof("CashLock"));
		cashCV[i] = CreateCondition("CashCV", sizeof("CashCV"));
		cashData[i] = 0;
		cashDataBool[i] = false;
		cashState[i] = BUSY;
		cashClerkIndex[i] = FREE;
	}
	
	appMoney = 0;
	picMoney = 0;
	passMoney = 0;
	cashMoney = 0;
	appMoneyLock = CreateLock("AppMoneyLock", sizeof("AppMoneyLock"));
	picMoneyLock = CreateLock("PicMoneyLock", sizeof("PicMoneyLock"));
	passMoneyLock = CreateLock("PassMoneyLock", sizeof("PassMoneyLock"));
	cashMoneyLock = CreateLock("CashMoneyLock", sizeof("CashMoneyLock"));
	
	for(i = 0; i < NUM_CUSTOMERS + NUM_SENATORS; i++) {
		fileLock[i] = CreateLock("FileLock", sizeof("FileLock"));
		fileState[i] = NONE;
		if(i < NUM_CUSTOMERS) {
			fileType[i] = CUSTOMER;
		}
		else {
			fileType[i] = SENATOR;
		}
		
		myCustMoney[i] = 1600;
		visitedApp[i] = false;
		visitedPic[i] = false;
		visitedPass[i] = false;
		visitedCash[i] = false;
	}
}

void AppClerk() {
	int myIndex;
	int i;
	
	Acquire(indexLock);
	for(i = 0; i < NUM_CLERKS; i++) {
		if(appClerkIndex[i] == FREE) {
			myIndex = i;
			appClerkIndex[i] = USED;
			break;
		}
	}
	Release(indexLock);
}

void PicClerk() {
	int myIndex;
	int i;
	
	Acquire(indexLock);
	for(i = 0; i < NUM_CLERKS; i++) {
		if(picClerkIndex[i] == FREE) {
			myIndex = i;
			picClerkIndex[i] = USED;
			break;
		}
	}
	Release(indexLock);
}

void PassClerk() {
	int myIndex;
	int i;
	
	Acquire(indexLock);
	for(i = 0; i < NUM_CLERKS; i++) {
		if(passClerkIndex[i] == FREE) {
			myIndex = i;
			passClerkIndex[i] = USED;
			break;
		}
	}
	Release(indexLock);
}

void CashClerk() {
	int myIndex;
	int i;
	
	Acquire(indexLock);
	for(i = 0; i < NUM_CLERKS; i++) {
		if(cashClerkIndex[i] == FREE) {
			myIndex = i;
			cashClerkIndex[i] = USED;
			break;
		}
	}
	Release(indexLock);
}

/* Helper function called for when a customer needs to enter the waiting room from inside the passport office */

void DoWaitingRoom() {
	/* Going to waiting room so decrement customer count in office and increase in waiting room before waiting */
	
	Acquire(customerLock);
	officeCustomer--;
	waitingCustomer++;
	Release(customerLock);
	
	Acquire(custWaitLock);
	Wait(custWaitCV, custWaitLock);
	Release(custWaitLock);
	
	Acquire(customerLock);
	officeCustomer++;
	waitingCustomer--;
	Release(customerLock);
}

/* Helper function for direct interaction between a customer and an application clerk */
void TalkAppClerk(int myIndex, enum BOOLEAN privLine) {

}

/* Helper function for direct interaction between a customer and a picture clerk */
void TalkPicClerk(int myIndex, enum BOOLEAN privLine) {

}

/* Helper function for a customer choosing which clerk - app or pic - to go to, and then which line - 
*	regular or privileged - to enter. Customer will always choose the shorter line and, if he has enough 
*	money and there are other customers waiting, he will enter the privileged line. Otherwise, he will just
*	save money and enter the regular line.
*/
void LineAppPicClerk(int myIndex) {
	
	Acquire(acpcLineLock);
	/* First check if have enough money to enter a privileged line */
	if (myCustMoney[myIndex] > 500) {
		
		/* If haven't gone to application clerk, go there if both of his lines are empty.
		*	Go into regular line to save money because both are empty. */
		if (regACLineLength == 0 && privACLineLength == 0 && visitedApp[myIndex] == false) {
			
			while (visitedApp[myIndex] = false) {
				regACLineLength++;
				Wait(regACLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numAppWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					regACLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numAppWait--;
					Release(senatorLock);
					TalkAppClerk(myIndex, false);
				}
			}
		}
		/* Same as above except for pic clerk */
		else if (regPCLineLength == 0 && privPCLineLength == 0 && visitedPic[myIndex] == false) {
			
			while (visitedPic[myIndex] = false) {
				regPCLineLength++;
				Wait(regPCLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numPicWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					regPCLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numPicWait--;
					Release(senatorLock);
					TalkPicClerk(myIndex, false);
				}
			}
		}
		/* Already been to picture clerk, so go to privileged application clerk line*/
		else if (visitedPic[myIndex] == true) {
			
			while (visitedApp[myIndex] = false) {
				privACLineLength++;
				Wait(privACLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numAppWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					privACLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numAppWait--;
					Release(senatorLock);
					TalkAppClerk(myIndex, true);
					myCustMoney[myIndex] -= 500;
				}
			}
		}
		/* If already been to application clerk then go to privileged picture clerk line */
		else if (visitedApp[myIndex] == true) {
			
			while (visitedPic[myIndex] = false) {
				privPCLineLength++;
				Wait(privPCLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numPicWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					privPCLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numPicWait--;
					Release(senatorLock);
					TalkPicClerk(myIndex, true);
					myCustMoney[myIndex] -= 500;
				}
			}
		}
		/* If application clerk's privileged line length is shorter than picture clerk's */
		else if (privACLineLength <= privPCLineLength) {
			
			while (visitedApp[myIndex] = false) {
				privACLineLength++;
				Wait(privACLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numAppWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					privACLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numAppWait--;
					Release(senatorLock);
					TalkAppClerk(myIndex, true);
					
					myCustMoney[myIndex] -= 500;
				}
			}
		}
		/* Else picture's is shorter */
		else {
			
			while (visitedPic[myIndex] = false) {
				privPCLineLength++;
				Wait(privPCLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numPicWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					privPCLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numPicWait--;
					Release(senatorLock);
					TalkAppClerk(myIndex, true);
					myCustMoney[myIndex] -= 500;
				}
			}
		}
	}
	/* Don't have enough money, just check regular lines */
	else {
		if (visitedPic[myIndex] == true) {
			
			while (visitedApp[myIndex] = false) {
				regACLineLength++;
				Wait(regACLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numAppWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					regACLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numAppWait--;
					Release(senatorLock);
					TalkAppClerk(myIndex, false);
				}
			}
		}
		/* If already been to application clerk then go to privileged picture clerk line */
		else if (visitedApp[myIndex] == true) {
			
			while (visitedPic[myIndex] = false) {
				regPCLineLength++;
				Wait(regPCLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numPicWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					regPCLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numPicWait--;
					Release(senatorLock);
					TalkPicClerk(myIndex, false);
				}
			}
		}
		/* If application clerk's privileged line length is shorter than picture clerk's */
		else if (regACLineLength <= regPCLineLength) {
			
			while (visitedApp[myIndex] = false) {
				regACLineLength++;
				Wait(regACLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numAppWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					regACLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numAppWait--;
					Release(senatorLock);
					TalkAppClerk(myIndex, false);
				}
			}
		}
		/* Else picture's is shorter */
		else {
			
			while (visitedPic[myIndex] = false) {
				regPCLineLength++;
				Wait(regPCLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				/* If there are senators in the office and no clerks waiting for me, go to waiting room */
				if (officeSenator > 0 && numPicWait == 0) {
					Release(senatorLock);
					
					Acquire(acpcLineLock);
					regPCLineLength--;
					Release(acpcLineLock);
					
					DoWaitingRoom();
					
					Acquire(acpcLineLock);
				}
				else {
				/* Else, a clerk is waiting for me, so move ahead */
					numPicWait--;
					Release(senatorLock);
					TalkAppClerk(myIndex, false);
				}
			}
		}
	}
}

/* Function to be forked for how a customer interacts throughout the office */
void Customer() {
	int myIndex;
	int i;
	
	/* Gives this customer the first available customer/senator index and sets their type to CUSTOMER */
	Acquire(indexLock);
	for (i = 0; i < NUM_CUSTOMERS + NUM_SENATORS; i++) {
		if (customerIndex[i] == FREE) {
			myIndex = i;
			customerIndex[i] = USED;
			fileType[i] = CUSTOMER;
			break;
		}
	}
	Release(indexLock);
	
	/* Checks if a senator is in the office, if so, go directly to the waiting room */
	Acquire(senatorLock);
	if (officeSenator > 0) {
		Release(senatorLock);
		
		Acquire(customerLock);
		waitingCustomer++;
		Release(customerLock);
		
		Acquire(custWaitLock);
		Wait(custWaitCV, custWaitLock);
		Release(custWaitLock);
		
		Acquire(customerLock);
		waitingCustomer--;
		Release(customerLock);
	}
	else {
	/* Else, just proceed into the office */
		Release(senatorLock);
	}
	
	Acquire(customerLock);
	officeCustomer++;
	Release(customerLock);
	
	while (visitedApp[i] != true || visitedPic[i] != true || visitedPass[i] != true || visitedCash[i] != true) {
		
		/* Do we need to make a random chance to visit the passport clerk first? */
		
		/* Visit application and picture clerks first */
		while (visitedApp[i] != true || visitedPic[i] != true) {
			
			/* Check for senators before getting into line */
			Acquire(senatorLock);
			if (officeSenator > 0) {
				Release(senatorLock);
				DoWaitingRoom();
			}
			else {
				Release(senatorLock);
			}
			LineAppPicClerk(myIndex);
		}
	}
}
	
int main() {
	Write("Calling InitializeData\n", sizeof("Calling InitializeData\n"), ConsoleOutput);
	InitializeData();
	Write("InitializeData has been called.\n", sizeof("InitializeData has been called.\n"), ConsoleOutput);
	
	Exit(0);
}
 