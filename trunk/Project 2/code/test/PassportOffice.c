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

/* Number of customer/senators in office and their locks*/
int officeCustomer, waitingCustomer, officeSenator; 
int customerLock, senatorLock;

/* Locks and conditions used for waiting */
int custWaitLock, senWaitLock, clerkWaitLock;
int custWaitCV, senWaitCV, clerkWaitCV; 

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

/* Individual customer's file state, type, and its lock */
int fileLock[NUM_CUSTOMERS + NUM_SENATORS];
enum CUST_STATE fileState[NUM_CUSTOMERS + NUM_SENATORS];
enum CUST_TYPE fileType[NUM_CUSTOMERS + NUM_SENATORS];

void InitializeData() {
	int i;
	
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
		
		picLock[i] = CreateLock("PicLock", sizeof("PicLock"));
		picCV[i] = CreateCondition("PicCV", sizeof("PicCV"));
		picData[i] = 0;
		picDataBool[i] = false;
		picState[i] = BUSY;
		
		passLock[i] = CreateLock("PassLock", sizeof("PassLock"));
		passCV[i] = CreateCondition("PassCV", sizeof("PassCV"));
		passData[i] = 0;
		passDataBool[i] = false;
		passState[i] = BUSY;
		
		cashLock[i] = CreateLock("CashLock", sizeof("CashLock"));
		cashCV[i] = CreateCondition("CashCV", sizeof("CashCV"));
		cashData[i] = 0;
		cashDataBool[i] = false;
		cashState[i] = BUSY;
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
	}

}
	
	
int main() {
	Write("Calling InitializeData\n", sizeof("Calling InitializeData\n"), ConsoleOutput);
	InitializeData();
	Write("InitializeData has been called.\n", sizeof("InitializeData has been called.\n"), ConsoleOutput);
	
	Exit(0);
}
 