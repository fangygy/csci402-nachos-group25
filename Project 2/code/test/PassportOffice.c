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
	PICDONE,
	APPDONE,
	APPPICDONE,
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

/*
*	Yinlerthai Chan
*	Application Clerk function thread
*	Receives an "application" from the Customer in the form of an SSN
*	Passes the SSN data to the Passport Clerk
*	Files the Customer's application, then dismisses the Customer
*	
*/
void AppClerk() {
	int myIndex;
	int mySSN;
	int i;
	enum CUST_TYPE cType = CUSTOMER;
	enum BOOLEAN loop = true;
	
	Acquire(indexLock);
	for(i = 0; i < NUM_CLERKS; i++) {
		if(appClerkIndex[i] == FREE) {
			myIndex = i;
			appClerkIndex[i] = USED;
			appState[i] = BUSY;
			break;
		}
	}
	Release(indexLock);
	
	while(loop == true){
		Acquire(senatorLock);
		Acquire(customerLock);
		if(officeSenator > 0 && officeCustomer > 0){
			Release(senatorLock);
			Release(customerLock);

			Acquire(clerkWaitLock);
			Wait(clerkWaitCV, clerkWaitLock);
			Release(clerkWaitLock);
		}
		else{
			Release(senatorLock);
			Release(customerLock);
		}

		Acquire(acpcLineLock);
		
		/* Check for the privileged customer line first
		*  If there are privileged customers, do AppClerk tasks, then received $500
		*/
		if(privACLineLength > 0){
			privACLineLength--;
			Acquire(senatorLock);
			numAppWait++;			/* shows customer that one clerk is waiting */
			Release(senatorLock);

			Acquire(appLock[myIndex]);
			appState[myIndex] = AVAILABLE;
			Signal(privACLineCV, acpcLineLock); /* Signals the next customer in priv line */
			Release(acpcLineLock);
			Wait(appCV[myIndex], appLock[myIndex]); /* Waits for the next customer */

			mySSN = appData[myIndex];
			Acquire(fileLock[mySSN]);

			/* check the customer type */
			if(fileType[mySSN] == CUSTOMER){
				cType = CUSTOMER;
			}
			else{
				cType = SENATOR;
			}

			if(fileState[mySSN] == NONE){
				fileState[mySSN] = APPDONE;
				Release(fileLock[mySSN]);
			}
			else if(fileState[mySSN] == PICDONE){
				fileState[mySSN] = APPPICDONE;
				Release(fileLock[mySSN]);
			}
			else{
				Write("Error. Customer does not have picture application or no application. What are you doing here?\n", sizeof("Error. Customer does not have picture application or no application. What are you doing here?\n"), ConsoleOutput);
				Release(fileLock[mySSN]);
			}

			for (i = 0; i < 20; i++){
				Yield();
			}

			if(cType == CUSTOMER){
				Write("AppClerk informs Customer that the app has been filed.\n", sizeof("AppClerk informs Customer that the app has been filed.\n"), ConsoleOutput);
			}
			else{
				Write("AppClerk informs Senator that the app has been filed.\n", sizeof("AppClerk informs Senator that the app has been filed.\n"), ConsoleOutput);
			}

			Acquire(appMoneyLock);
			appMoney += 500;

			if(cType == CUSTOMER){
				Write("AppClerk accepts money = 500 from Customer\n", sizeof("AppClerk accepts money = 500 from Customer\n"), ConsoleOutput);
			}
			else{
				Write("AppClerk accepts money = 500 from Senator\n", sizeof("AppClerk accepts money = 500 from Senator\n"), ConsoleOutput);
			}

			Release(appMoneyLock);
			Signal(appCV[myIndex], appLock[myIndex]); /* signal customer awake */
			Release(appLock[myIndex]);	/* Release clerk lock */

		}	
		/* Check for regular customer line next.
		* If there are regular customers, do AppClerk tasks 
		*/
		else if(regACLineLength > 0){
			regACLineLength--;
			Acquire(senatorLock);
			numAppWait++;			/* shows customer that one clerk is waiting */
			Release(senatorLock);

			Acquire(appLock[myIndex]);
			appState[myIndex] = AVAILABLE;
			Signal(regACLineCV, acpcLineLock); /* Signals the next customer in priv line */
			Release(acpcLineLock);
			Wait(appCV[myIndex], appLock[myIndex]); /* Waits for the next customer */

			mySSN = appData[myIndex];
			Acquire(fileLock[mySSN]);

			/* check the customer type */
			if(fileType[mySSN] == CUSTOMER){
				cType = CUSTOMER;
			}
			else{
				cType = SENATOR;
			}

			if(fileState[mySSN] == NONE){
				fileState[mySSN] = APPDONE;
				Release(fileLock[mySSN]);
			}
			else if(fileState[mySSN] == PICDONE){
				fileState[mySSN] = APPPICDONE;
				Release(fileLock[mySSN]);
			}
			else{
				Write("Error. Customer does not have picture application or no application. What are you doing here?\n", sizeof("Error. Customer does not have picture application or no application. What are you doing here?\n"), ConsoleOutput);
				Release(fileLock[mySSN]);
			}

			for (i = 0; i < 20; i++){
				Yield();
			}

			if(cType == CUSTOMER){
				Write("AppClerk informs Customer that the app has been filed.\n", sizeof("AppClerk informs Customer that the app has been filed.\n"), ConsoleOutput);
			}
			else{
				Write("AppClerk informs Senator that the app has been filed.\n", sizeof("AppClerk informs Senator that the app has been filed.\n"), ConsoleOutput);
			}

			Signal(appCV[myIndex], appLock[myIndex]); /* signal customer awake */
			Release(appLock[myIndex]);	/* Release clerk lock */
		}
		/* If there are neither privileged or regular customers, go on break */
		else{
			Release(acpcLineLock);
			Acquire(appLock[myIndex]);
			appState[myIndex] = BREAK;
			Write("ApplicationClerk is going on break\n", sizeof("ApplicationClerk is going on break\n"), ConsoleOutput);

			Wait(appCV[myIndex], appLock[myIndex]);
			Write("ApplicationClerk returned from break\n", sizeof("ApplicationClerk returned from break\n"), ConsoleOutput);
			Release(appLock[myIndex]);
		}
	}
}

/*
*	Yinlerthai Chan:
*	Picture Clerk function thread
*	Takes a "picture" of the Customer
*	If the Customer dislikes the picture, will continue to take pictures until the Customer approves it
*	Once approved, the Picture Clerk will file the picture, then dismiss the Customer
*/
void PicClerk() {
	int myIndex;
	int mySSN;
	int i;
	enum CUST_TYPE cType = CUSTOMER;
	enum BOOLEAN loop = true;	

	Acquire(indexLock);
	for(i = 0; i < NUM_CLERKS; i++) {
		if(picClerkIndex[i] == FREE) {
			myIndex = i;
			picClerkIndex[i] = USED;
			picState[i] = BUSY;
			break;
		}
	}
	Release(indexLock);

	while(loop == true){
		Acquire(senatorLock);
		Acquire(customerLock);
		if (officeSenator > 0 && officeCustomer > 0) {
			Release(senatorLock);
			Release(customerLock);
			
			Acquire(clerkWaitLock);
			Wait(clerkWaitCV, clerkWaitLock);
			Release(clerkWaitLock);
		}	
		else {
			Release(senatorLock);
			Release(customerLock);
		}
		
		Acquire(acpcLineLock);

		/* Check for the privileged customer line first
		* If there are privileged customers, do PicClerk tasks, then receive $500
		*/
		if(privPCLineLength > 0){
			privPCLineLength--;
			
			Acquire(senatorLock);
			numPicWait++;		/* Shows customer that one clerk is waiting */
			Release(senatorLock);

			Acquire(picLock[myIndex]);
			picState[myIndex] = AVAILABLE;
			Signal(privPCLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Wait(picCV[myIndex], picLock[myIndex]);	/* Waits for the next customer */

			mySSN = picData[myIndex];
			/* check the customer type */
			Acquire(fileLock[mySSN]);
			if (fileType[mySSN] == CUSTOMER) {
				cType = CUSTOMER;
			} else {
				cType = SENATOR;
			}

			if(fileState[mySSN] == NONE){
				fileState[mySSN] = PICDONE;
				Release(fileLock[mySSN]);
			}
			else if(fileState[mySSN] == APPDONE){
				fileState[mySSN] = APPPICDONE;
				Release(fileLock[mySSN]);
			}
			else{
				Write("Error. Customer does not have either an application or no application. What are you doing here?\n", sizeof("Error. Customer does not have either an application or no application. What are you doing here?\n"), ConsoleOutput);
				Release(fileLock[mySSN]);
			}

			if (cType == CUSTOMER) {
				Write("PictureClerk takes picture of Customer\n",sizeof("PictureClerk takes picture of Customer\n"),ConsoleOutput);
			} else {
				Write("PictureClerk takes picture of Senator\n",sizeof("PictureClerk takes picture of Senator\n"),ConsoleOutput);
			}
				/* yield to take picture
				* print statement: "Taking picture"
				* picCV->Signal then picCV->Wait to
				* show customer picture
				*/
			while(picDataBool[myIndex] == false){
				for(i = 0; i < 4; i++){
					Yield();
				}
				Signal(picCV[myIndex], picLock[myIndex]);	
				Wait(picCV[myIndex], picLock[myIndex]);	/* Shows the customer the picture */

				if(picDataBool[myIndex] ==false){
					if (cType == CUSTOMER) {
						Write("PictureClerk takes picture of Customer again\n",sizeof("PictureClerk takes picture of Customer again\n"),ConsoleOutput);

					} else {
						Write("PictureClerk takes picture of Senator again\n",sizeof("PictureClerk takes picture of Senator again\n"),ConsoleOutput);
					}
				}
			}


			/* file picture using
			* current thread yield
			*/
			for(i = 0; i < 20; i++){
				Yield();
			}
			if (cType == CUSTOMER) {
				Write("Picture informs Customer that the procedure has been completed\n", sizeof("Picture informs Customer that the procedure has been completed\n"), ConsoleOutput);
			} else {
				Write("Picture informs Senator that the procedure has been completed\n", sizeof("Picture informs Senator that the procedure has been completed\n"), ConsoleOutput);
			}

			/* signal customer awake */
			Acquire(picMoneyLock);
			picMoney += 500;

			if (cType == CUSTOMER) {
				Write("PictureClerk accepts money = $500 from Customer\n",sizeof("PictureClerk accepts money = $500 from Customer\n"),ConsoleOutput);
			} else {
				Write("PictureClerk accepts money = $500 from Senator\n",sizeof("PictureClerk accepts money = $500 from Senator\n"),ConsoleOutput);
			}

			Release(picMoneyLock);

			picDataBool[myIndex] = false;
			Signal(picCV[myIndex], picLock[myIndex]); /* signal customer awake */
			Release(picLock[myIndex]); /* release clerk lock */
		}
		/* Check for regular customer line next
		* If there are regular customers, do PicClerk tasks
		*/
		else if (regPCLineLength > 0){
			regPCLineLength--;
			
			Acquire(senatorLock);
			numPicWait++;		/* Shows customer that one clerk is waiting */
			Release(senatorLock);

			Acquire(picLock[myIndex]);
			picState[myIndex] = AVAILABLE;
			Signal(regPCLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Wait(picCV[myIndex], picLock[myIndex]);	/* Waits for the next customer */

			mySSN = picData[myIndex];
			/* check the customer type */
			Acquire(fileLock[mySSN]);
			if (fileType[mySSN] == CUSTOMER) {
				cType = CUSTOMER;
			} else {
				cType = SENATOR;
			}

			if(fileState[mySSN] == NONE){
				fileState[mySSN] = PICDONE;
				Release(fileLock[mySSN]);
			}
			else if(fileState[mySSN] == APPDONE){
				fileState[mySSN] = APPPICDONE;
				Release(fileLock[mySSN]);
			}
			else{
				Write("Error. Customer does not have either an application or no application. What are you doing here?\n", sizeof("Error. Customer does not have either an application or no application. What are you doing here?\n"), ConsoleOutput);
				Release(fileLock[mySSN]);
			}

			if (cType == CUSTOMER) {
				Write("PictureClerk takes picture of Customer\n",sizeof("PictureClerk takes picture of Customer\n"),ConsoleOutput);
			} else {
				Write("PictureClerk takes picture of Senator\n",sizeof("PictureClerk takes picture of Senator\n"),ConsoleOutput);
			}
				/* yield to take picture
				* print statement: "Taking picture"
				* picCV->Signal then picCV->Wait to
				* show customer picture
				*/
			while(picDataBool[myIndex] == false){
				for(i = 0; i < 4; i++){
					Yield();
				}
				Signal(picCV[myIndex], picLock[myIndex]);	
				Wait(picCV[myIndex], picLock[myIndex]);	/* Shows the customer the picture */

				if(picDataBool[myIndex] ==false){
					if (cType == CUSTOMER) {
						Write("PictureClerk takes picture of Customer again\n",sizeof("PictureClerk takes picture of Customer again\n"),ConsoleOutput);

					} else {
						Write("PictureClerk takes picture of Senator again\n",sizeof("PictureClerk takes picture of Senator again\n"),ConsoleOutput);
					}
				}
			}


			/* file picture using
			* current thread yield
			*/
			for(i = 0; i < 20; i++){
				Yield();
			}
			if (cType == CUSTOMER) {
				Write("Picture informs Customer that the procedure has been completed\n", sizeof("Picture informs Customer that the procedure has been completed\n"), ConsoleOutput);
			} else {
				Write("Picture informs Senator that the procedure has been completed\n", sizeof("Picture informs Senator that the procedure has been completed\n"), ConsoleOutput);
			}

			picDataBool[myIndex] = false;
			Signal(picCV[myIndex], picLock[myIndex]); /* signal customer awake */
			Release(picLock[myIndex]); /* release clerk lock */
		}		
		/* If there are neither privileged or regular customers, go on break */
		else{
			Release(acpcLineLock);
			Acquire(picLock[myIndex]);
			picState[myIndex] = BREAK;
			Write("PictureClerk is going on break\n",sizeof("PictureClerk is going on break\n"), ConsoleOutput);
			Wait(picCV[myIndex], picLock[myIndex]);
			Write("PictureClerk returned from break\n",sizeof("PictureClerk returned from break\n"), ConsoleOutput);

			Release(picLock[myIndex]);     /* release clerk lock */
		}
	}
}

void PassClerk() {
	int myIndex;
	int mySSN;
	int i;
	enum CUST_TYPE cType = CUSTOMER;
	enum BOOLEAN doPassport = false;
	enum BOOLEAN loop = true;

	Acquire(indexLock);
	for(i = 0; i < NUM_CLERKS; i++) {
		if(passClerkIndex[i] == FREE) {
			myIndex = i;
			passClerkIndex[i] = USED;
			passState[i] = BUSY;
			break;
		}
	}
	Release(indexLock);

	while(loop == true){
		Acquire(senatorLock);
		Acquire(customerLock);
		if(officeSenator > 0 && officeCustomer > 0){
			Release(senatorLock);
			Release(customerLock);

			Acquire(clerkWaitLock);
			Wait(clerkWaitCV, clerkWaitLock);
			Release(clerkWaitLock);
		}
		else{
			Release(senatorLock);
			Release(customerLock);	
		}
		/* Check for customers in line */
		Acquire(passLineLock);
		if(privPassLineLength > 0){
		/* Decrement line length, set state to AVAIL, signal 1st customer and wait for them */
			privPassLineLength--;
			Acquire(senatorLock);
			passState[myIndex] = AVAILABLE;
			
			Signal(privPassLineCV, passLineLock);
			Release(passLineLock);
			Wait(passCV[myIndex], passLock[myIndex]); /* wait for customer to signal me */

			mySSN = passData[myIndex]; /* customer gave me thier SSN index to check thier file */
			Acquire(fileLock[mySSN]);
			/* check customer type */
			if(fileType[mySSN] == CUSTOMER){
				cType = CUSTOMER;
			}
			else{
				cType = SENATOR;
			}
			if(fileState[mySSN] == APPPICDONE){
				passDataBool[myIndex] = true;
				doPassport = true;

				if(cType == CUSTOMER){
					Write("PassClerk gives valid certification to Customer\n", sizeof("PassClerk gives valid certification to Customer\n"), ConsoleOutput);
				}
				else{
					Write("PassClerk gives valid certification to Senator\n", sizeof("PassClerk gives valid certification to Senator\n"), ConsoleOutput);
				}
			} else {
				passDataBool[myIndex] = false;
				doPassport = false;
				if(cType == CUSTOMER){
					Write("PassClerk gives invalid certification to Customer\n", sizeof("PassClerk gives invalid certification to Customer\n"), ConsoleOutput);
					Write("PassClerk punishes Customer to wait\n", sizeof("PassClerk punishes Customer to wait\n"), ConsoleOutput);				
				}
				else{
					Write("PassClerk gives invalid certification to Senator\n", sizeof("PassClerk gives invalid certification to Senator\n"), ConsoleOutput);
					Write("PassClerk punishes Senator to wait\n", sizeof("PassClerk punishes Senator to wait\n"), ConsoleOutput);
				}
			}
			Release(fileLock[mySSN]);

			/* add $500 to passClerk money amount for privileged fee */
			Acquire(passMoneyLock);
			passMoney += 500;
			Release(passMoneyLock);

			if(cType == CUSTOMER){
				Write("PassportClerk accepts money = $500 from Customer\n", sizeof("PassportClerk accepts money = $500 from Customer\n"), ConsoleOutput);
				Write("PassportClerk informs Customer that the procedure has been completed\n", sizeof("PassportClerk informs Customer that the procedure has been completed\n"), ConsoleOutput);
			}
			else{
				Write("PassportClerk accepts money = $500 from Senator\n", sizeof("PassportClerk accepts money = $500 from Senator\n"), ConsoleOutput);
				Write("PassportClerk informs Senator that the procedure has been completed\n", sizeof("PassportClerk informs Senator that the procedure has been completed\n"), ConsoleOutput);
			}

			Signal(passCV[myIndex], passLock[myIndex]); /* signal customer awake */
			Release(passLock[myIndex]);				/* release clerk lock */

			if(doPassport){
				for(i = 0; i < 20; i++){
					Yield();
				}

				Acquire(fileLock[mySSN]);
				fileState[mySSN] = PASSDONE;
				Release(fileLock[mySSN]);
				if(cType = CUSTOMER){
					Write("PassportClerk has finished filing Customer's passport\n", sizeof("PassportClerk has finished filing Customer's passport\n"), ConsoleOutput);
				}
				else{
					Write("PassportClerk has finished filing Senator's passport\n", sizeof("PassportClerk has finished filing Senator's passport\n"), ConsoleOutput);
				}
			}
		}

		else if (regPassLineLength > 0){
		/* Decrement line length, set state to AVAIL, signal 1st customer and wait for them */
			regPassLineLength--;
			Acquire(senatorLock);
			passState[myIndex] = AVAILABLE;
			
			Signal(regPassLineCV, passLineLock);
			Release(passLineLock);
			Wait(passCV[myIndex], passLock[myIndex]); /* wait for customer to signal me */

			mySSN = passData[myIndex]; /* customer gave me thier SSN index to check thier file */
			Acquire(fileLock[mySSN]);
			/* check customer type */
			if(fileType[mySSN] == CUSTOMER){
				cType = CUSTOMER;
			}
			else{
				cType = SENATOR;
			}
			if(fileState[mySSN] == APPPICDONE){
				passDataBool[myIndex] = true;
				doPassport = true;

				if(cType == CUSTOMER){
					Write("PassClerk gives valid certification to Customer\n", sizeof("PassClerk gives valid certification to Customer\n"), ConsoleOutput);
				}
				else{
					Write("PassClerk gives valid certification to Senator\n", sizeof("PassClerk gives valid certification to Senator\n"), ConsoleOutput);
				}
			} else {
				passDataBool[myIndex] = false;
				doPassport = false;
				if(cType == CUSTOMER){
					Write("PassClerk gives invalid certification to Customer\n", sizeof("PassClerk gives invalid certification to Customer\n"), ConsoleOutput);
					Write("PassClerk punishes Customer to wait\n", sizeof("PassClerk punishes Customer to wait\n"), ConsoleOutput);				
				}
				else{
					Write("PassClerk gives invalid certification to Senator\n", sizeof("PassClerk gives invalid certification to Senator\n"), ConsoleOutput);
					Write("PassClerk punishes Senator to wait\n", sizeof("PassClerk punishes Senator to wait\n"), ConsoleOutput);
				}
			}
			if(cType == CUSTOMER){
				Write("PassportClerk informs Customer that the procedure has been completed\n", sizeof("PassportClerk informs Customer that the procedure has been completed\n"), ConsoleOutput);
			}
			else{
				Write("PassportClerk informs Senator that the procedure has been completed\n", sizeof("PassportClerk informs Senator that the procedure has been completed\n"), ConsoleOutput);
			}

			Release(fileLock[mySSN]);

			Signal(passCV[myIndex], passLock[myIndex]); /* signal customer awake */
			Release(passLock[myIndex]);				/* release clerk lock */

			if(doPassport){
				for(i = 0; i < 20; i++){
					Yield();
				}

				Acquire(fileLock[mySSN]);
				fileState[mySSN] = PASSDONE;
				Release(fileLock[mySSN]);
				if(cType = CUSTOMER){
					Write("PassportClerk has finished filing Customer's passport\n", sizeof("PassportClerk has finished filing Customer's passport\n"), ConsoleOutput);
				}
				else{
					Write("PassportClerk has finished filing Senator's passport\n", sizeof("PassportClerk has finished filing Senator's passport\n"), ConsoleOutput);
				}
			}
		}
		else{
			/* No one in line...take a break */
			Release(passLineLock);
			Acquire(passLock[myIndex]);
			passState[myIndex] = BREAK;
			Wait(passCV[myIndex], passLock[myIndex]);
			Write("PassportClerk returned from break\n", sizeof("PassportClerk returned from break\n"), ConsoleOutput);
			Release(passLock[myIndex]);
		}
	}
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
/*
*	Yinlerthai Chan & Jasper Lee:
*	Manager function thread
*	Constantly checks over customer lines to see if there are any customers
*	Wakes up Clerks on break if so
* Also periodically prints out a money report, stating how much money each Clerk has, and total money overall
*/
void Manager(){
	int totalMoney;
	int i;
	enum BOOLEAN loop = true;

	while(loop == true){
		/* Check for any senators present */
		Acquire(senatorLock);
		/* If there are senators present and customers in office, then 
		* wake up all customers and tell them to go to the wait room. 
		* Wait until all customers are out of the passport office 
		* before proceeding.
		*/
		if(officeSenator > 0){
			Release(senatorLock);
			Acquire(customerLock);
			while(officeCustomer > 0){
				/* Wake up all customers waiting in every line */
				Release(customerLock);

				/* AppClerk and PicClerk Lines */
				Acquire(acpcLineLock);
				Broadcast(regACLineCV, acpcLineLock);
				Broadcast(privACLineCV, acpcLineLock);
				Broadcast(regPCLineCV, acpcLineLock);
				Broadcast(privACLineCV, acpcLineLock);
				Release(acpcLineLock);

				/* PassClerk Lines */
				Acquire(passLineLock);
				Broadcast(regPassLineCV, passLineLock);
				Broadcast(privPassLineCV, passLineLock);
				Release(passLineLock);

				/* Cashier Lines */
				Acquire(cashLineLock);
				Broadcast(cashLineCV, cashLineLock);
				Release(cashLineLock);

				Yield();
				Acquire(customerLock);
			}
			Release(customerLock);

			/* Now wake up any waiting senators */
			Acquire(senWaitLock);
			Broadcast(senWaitCV, senWaitLock);
			Release(senWaitLock);

			Acquire(clerkWaitLock);
			Broadcast(clerkWaitCV, clerkWaitLock);
			Release(clerkWaitLock);
		}
		else if(officeSenator == 0 && waitingCustomer > 0){
			/* If there are no senators, but customers in the wating room
			* Wake up all waiting roo customers
			*/
			Release(senatorLock);
			Acquire(customerLock);
			while(waitingCustomer > 0){
				Release(customerLock);
				Acquire(custWaitLock);
				Broadcast(custWaitCV, custWaitLock);
				Release(custWaitLock);
				Yield();
				Acquire(customerLock);
			}
			Release(customerLock);
		}
		else{
			/* Else just proceed */
			Release(senatorLock);
		}

		totalMoney = 0;

		/* Checks for AppClerk on break */
		Acquire(acpcLineLock);
		if(privACLineLength >= 3 || regACLineLength >= 3){
			Release(acpcLineLock);
			for(i = 0; i < NUM_CLERKS; i++){
				if(appState[i] == BREAK){
					Acquire(appLock[i]);
					Signal(appCV[i], appLock[i]);
					appState[i] = BUSY;
					Release(appLock[i]);
					Write("Manager calls back an ApplicationClerk from break\n", sizeof("Manager calls back an ApplicationClerk from break\n"), ConsoleOutput);
				}
			}
		}
		else if(privACLineLength >= 1 || regACLineLength >= 1){
			Release(acpcLineLock);
			for(i = 0; i < NUM_CLERKS; i++){
				if(appState[i] == BREAK){
					Acquire(appLock[i]);
					Signal(appCV[i], appLock[i]);
					appState[i] = BUSY;
					Release(appLock[i]);
					Write("Manager calls back an ApplicationClerk from break\n", sizeof("Manager calls back an ApplicationClerk from break\n"), ConsoleOutput);
					break;
				}
			}
		}
		else{
			Release(acpcLineLock);
		}
			/* Checks for PicClerk on break */
		Acquire(acpcLineLock);
		if(privPCLineLength >= 3 || regPCLineLength >= 3){
			Release(acpcLineLock);
			for(i = 0; i < NUM_CLERKS; i++){
				if(picState[i] == BREAK){
					Acquire(picLock[i]);
					Signal(picCV[i], appLock[i]);
					picState[i] = BUSY;
					Release(picLock[i]);
					Write("Manager calls back a PictureClerk from break\n", sizeof("Manager calls back a PictureClerk from break\n"), ConsoleOutput);
				}
			}
		}
		else if(privPCLineLength >= 1 || regPCLineLength >= 1){
			Release(acpcLineLock);
			for(i = 0; i < NUM_CLERKS; i++){
				if(picState[i] == BREAK){
					Acquire(picLock[i]);
					Signal(picCV[i], picLock[i]);
					picState[i] = BUSY;
					Release(picLock[i]);
					Write("Manager calls back a PictureClerk from break\n", sizeof("Manager calls back a PictureClerk from break\n"), ConsoleOutput);
					break;
				}
			}
		}
		else{
			Release(acpcLineLock);
		}
			/* Checks for PassClerk on break */
		Acquire(passLineLock);
		if(privPassLineLength >= 3 || regPassLineLength >= 3){
			Release(passLineLock);
			for(i = 0; i < NUM_CLERKS; i++){
				if(passState[i] == BREAK){
					Acquire(passLock[i]);
					Signal(passCV[i], passLock[i]);
					passState[i] = BUSY;
					Release(passLock[i]);
					Write("Manager calls back a PassporClerk from break\n", sizeof("Manager calls back a PassportClerk from break\n"), ConsoleOutput);
				}
			}
		}
		else if(privPassLineLength >= 1 || regPassLineLength >= 1){
			Release(passLineLock);
			for(i = 0; i < NUM_CLERKS; i++){
				if(passState[i] == BREAK){
					Acquire(passLock[i]);
					Signal(passCV[i], passLock[i]);
					passState[i] = BUSY;
					Release(passLock[i]);
					Write("Manager calls back a PassporClerk from break\n", sizeof("Manager calls back a PassportClerk from break\n"), ConsoleOutput);
					break;
				}
			}
		}
		else{
			Release(passLineLock);
		}
		/* Checks for Cashier on break */
		Acquire(cashLineLock);
		if(cashLineLength >= 3){
			Release(cashLineLock);
			for(i = 0; i < NUM_CLERKS; i++){
				if(cashState[i] == BREAK){
					Acquire(cashLock[i]);
					Signal(cashCV[i], cashLock[i]);
					cashState[i] = BUSY;
					Release(cashLock[i]);
					Write("Manager calls back a Cashier from break\n", sizeof("Manager calls back a Cashier from break\n"), ConsoleOutput);
				}
			}
		}
		else if(cashLineLength >= 1){
			Release(cashLineLock);
			for(i = 0; i < NUM_CLERKS; i++){
				if(cashState[i] == BREAK){
					Acquire(cashLock[i]);
					Signal(cashCV[i], cashLock[i]);
					cashState[i] = BUSY;
					Release(cashLock[i]);
					Write("Manager calls back a Cashier from break\n", sizeof("Manager calls back a Cashier from break\n"), ConsoleOutput);
					break;
				}
			}
		}
		else{
			Release(cashLineLock);
		}

		if(officeCustomer + officeSenator + waitingCustomer == 0){
			/* Print out stuff */
			Write("Manager: Print out stuff.\n", sizeof("Manager: Print out stuff.\n"), ConsoleOutput);
			break;
		}
		Yield();
	}
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
 