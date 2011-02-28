/* PassportOffice.c
 *	
 * The passport office
 *	
 */
 
#include "syscall.h"
#define NUM_CLERKS 3
#define NUM_CUSTOMERS 10
#define NUM_SENATORS 0

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
		
		customerIndex[i] = FREE;
		myCustMoney[i] = 1100;
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
			numPassWait++;
			Release(senatorLock);
			
			Acquire(passLock[myIndex]);
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
			numPassWait++;
			Release(senatorLock);
			
			Acquire(passLock[myIndex]);
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
	int myCustomer;
	enum CUST_TYPE myCustType;
	int i;
	enum BOOLEAN loop = true;
	
	Acquire(indexLock);
	for(i = 0; i < NUM_CLERKS; i++) {
		if(cashClerkIndex[i] == FREE) {
			myIndex = i;
			cashClerkIndex[i] = USED;
			break;
		}
	}
	Release(indexLock);
	
	Acquire(cashLock[myIndex]);
	cashState[myIndex] = BUSY;
	Release(cashLock[myIndex]);
	
	/* Main loop for cashier */
	while (loop == true) {
	
		Acquire(senatorLock);
		Acquire(customerLock);
		/* If there are both customers AND senators in the office,
		*	then wait until manager tells you to work again. */
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
		
		/* Checking my line. */
		Acquire(cashLineLock);
		if (cashLineLength > 0) {
			
			cashLineLength--;
			
			Acquire(senatorLock);
			numCashWait++;
			Release(senatorLock);
			
			Acquire(cashLock[myIndex]);
			cashState[myIndex] = AVAILABLE;
			Signal(cashLineCV, cashLineLock);
			Release(cashLineLock);
			Wait(cashCV[myIndex], cashLock[myIndex]);
			
			myCustomer = cashData[myIndex];
			
			Acquire(fileLock[myCustomer]);
			myCustType = fileType[myCustomer];
			
			if (fileState[myCustomer] == PASSDONE) {
				cashDataBool[myIndex] = true;
				for (i = 0; i < 50; i++) {
					Yield();
				}
				
				fileState[myCustomer] = ALLDONE;
				
				Acquire(cashMoneyLock);
				cashMoney += 100;
				Release(cashMoneyLock);
			}
			else {
				cashDataBool[myIndex] = false;
			}
			
			Release(fileLock[myCustomer]);
			Signal(cashCV[myIndex], cashLock[myIndex]);
			cashState[myIndex] = BUSY;
			Release(cashLock[myIndex]);
			Yield();
		}
		else {
		/* Nobody in line, break */
			Write("A cashier is going on break.\n",
				sizeof("A cashier is going on break.\n"),
				ConsoleOutput);
			Release(cashLineLock);
			Acquire(cashLock[myIndex]);
			cashState[myIndex] = BREAK;
			Wait(cashCV[myIndex], cashLock[myIndex]);
			Release(cashLock[myIndex]);
			
			Write("A cashier has returned from break.\n",
				sizeof("A cashier has returned from break.\n"),
				ConsoleOutput);
		}		
	}
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
		/*Write("Manager loop.\n", sizeof("Manager loop.\n"), ConsoleOutput); */
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
				Write("Waking up customers loop.\n", sizeof("Waking up customers loop.\n"), ConsoleOutput);
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
			Halt();
			break;
		}
		Yield();
	}
}


/* Helper function called for when a customer needs to enter the waiting room from inside the passport office */

void DoWaitingRoom() {
	/* Going to waiting room so decrement customer count in office and increase in waiting room before waiting */
	
	Write("A customer has entered the waiting room.\n", sizeof("A customer has entered the waiting room.\n"),
		ConsoleOutput);
	Acquire(customerLock);
	officeCustomer--;
	waitingCustomer++;
	Release(customerLock);
	
	Acquire(custWaitLock);
	Wait(custWaitCV, custWaitLock);
	Release(custWaitLock);
	Write("A customer has woken up from the waiting room.\n", sizeof("A customer has woken up from the waiting room.\n"), ConsoleOutput);
	
	Acquire(customerLock);
	officeCustomer++;
	waitingCustomer--;
	Release(customerLock);
}

/* Helper function for direct interaction between a customer and an application clerk */
void TalkAppClerk(int myIndex, enum BOOLEAN privLine) {
	int myClerk;
	int i;
	/* Search for an available clerk */
	for (i = 0; i < NUM_CLERKS; i++) {
		Acquire(appLock[i]);
		if (appState[i] == AVAILABLE) {
			/* Found him, store it and set him to busy */
			myClerk = i;
			appState[i] = BUSY;
			break;
		}
		else {
			Release(appLock[i]);
		}
	}
	
	/* Give the clerk your index */
	appData[myClerk] = myIndex;
	if (privLine == true) {
		if (fileType[myIndex] == CUSTOMER) {
			Write("A customer is willing to pay 500 to ApplicationClerk to move ahead in line.\n",
				sizeof("A customer is willing to pay 500 to ApplicationClerk to move ahead in line.\n"),
				ConsoleOutput);
		}
		else {
			Write("A senator is willing to pay 500 to ApplicationClerk to move ahead in line.\n",
				sizeof("A senator is willing to pay 500 to ApplicationClerk to move ahead in line.\n"),
				ConsoleOutput);
		}
	}
	if (fileType[myIndex] == CUSTOMER) {
		Write("A customer gives his application to the ApplicationClerk.\n",
			sizeof("A customer gives his application to the ApplicationClerk.\n"),
			ConsoleOutput);
	}
	else {
		Write("A senator gives his application to the ApplicationClerk.\n",
			sizeof("A senator gives his application to the ApplicationClerk.\n"),
			ConsoleOutput);
	}
	
	/* Signal and wait for him to respond */
	Signal(appCV[myClerk], appLock[myClerk]);
	Wait(appCV[myClerk], appLock[myClerk]);
	Release(appLock[myClerk]);
	
	if (fileType[myIndex] == CUSTOMER) {
		Write("A customer is informed by ApplicationClerk that his application is filed.\n",
			sizeof("A customer is informed by ApplicationClerk that his application is filed.\n"),
			ConsoleOutput);
	}
	else {
		Write("A senator is informed by ApplicationClerk that his application is filed.\n",
			sizeof("A senator is informed by ApplicationClerk that his application is filed.\n"),
			ConsoleOutput);
	}
	
	/* Be sure to tell myself I have visited an ApplicationClerk */
	visitedApp[myIndex] = true;
}

/* Helper function for direct interaction between a customer and a picture clerk */
void TalkPicClerk(int myIndex, enum BOOLEAN privLine) {
	int myClerk;
	int i;
	enum BOOLEAN hatePicture = true;
	/* Search for an available clerk */
	for (i = 0; i < NUM_CLERKS; i++) {
		Acquire(picLock[i]);
		if (picState[i] == AVAILABLE) {
			/* Found him, store it and set him to busy */
			myClerk = i;
			picState[i] = BUSY;
			break;
		}
		else {
			Release(picLock[i]);
		}
	}
	
	/* Give the clerk your index */
	picData[myClerk] = myIndex;
	if (privLine == true) {
		if (fileType[myIndex] == CUSTOMER) {
			Write("A customer is willing to pay 500 to PictureClerk to move ahead in line.\n",
				sizeof("A customer is willing to pay 500 to PictureClerk to move ahead in line.\n"),
				ConsoleOutput);
		}
		else {
			Write("A senator is willing to pay 500 to PictureClerk to move ahead in line.\n",
				sizeof("A senator is willing to pay 500 to PictureClerk to move ahead in line.\n"),
				ConsoleOutput);
		}
	}
	
	/* Signal and wait for him to respond */
	Signal(picCV[myClerk], picLock[myClerk]);
	Wait(picCV[myClerk], picLock[myClerk]);
	
	while (picDataBool[myClerk] == false) {
		
		/* Technically, should be random, but don't know how to implement.
		* 	Will just always hate the first picture given for now */
		if (hatePicture == true) {
			picDataBool[myClerk] = false;
			if (fileType[myIndex] == CUSTOMER) {
				Write("A customer doesn't like the picture provided by PictureClerk.\n",
					sizeof("A customer doesn't like the picture provided by PictureClerk.\n"),
					ConsoleOutput);
			}
			else {
				Write("A senator doesn't like the picture provided by PictureClerk.\n",
					sizeof("A senator doesn't like the picture provided by PictureClerk.\n"),
					ConsoleOutput);
			}
			hatePicture = false;
			
			Signal(picCV[myClerk], picLock[myClerk]);
			Wait(picCV[myClerk], picLock[myClerk]);
		}
		else {
			picDataBool[myClerk] = true;
			if (fileType[myIndex] == CUSTOMER) {
				Write("A customer likes the picture provided by PictureClerk.\n",
					sizeof("A customer likes the picture provided by PictureClerk.\n"),
					ConsoleOutput);
			}
			else {
				Write("A senator likes the picture provided by PictureClerk.\n",
					sizeof("A senator likes the picture provided by PictureClerk.\n"),
					ConsoleOutput);
			}
			
			Signal(picCV[myClerk], picLock[myClerk]);
			Wait(picCV[myClerk], picLock[myClerk]);
			break;
		}
	}
	
	Release(picLock[myClerk]);
	if (fileType[myIndex] == CUSTOMER) {
		Write("A customer is told by PictureClerk that the procedure has been completed.\n",
			sizeof("A customer is told by PictureClerk that the procedure has been completed.\n"),
					ConsoleOutput);		
	}
	else {
		Write("A senator is told by PictureClerk that the procedure has been completed.\n",
			sizeof("A senator is told by PictureClerk that the procedure has been completed.\n"),
					ConsoleOutput);		
	}	
	
	/* Be sure to tell myself I have visited an PictureClerk */
	visitedPic[myIndex] = true;
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
			
			while (visitedApp[myIndex] == false) {
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
					
					Write("A customer was in the regular wait queue for ApplicationClerk.\n",
						sizeof("A customer was in the regular wait queue for ApplicationClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the regular wait queue of ApplicationClerk.\n",
						sizeof("A customer rejoins the regular wait queue of ApplicationClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedPic[myIndex] == false) {
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
					
					Write("A customer was in the regular wait queue for PictureClerk.\n",
						sizeof("A customer was in the regular wait queue for PictureClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the regular wait queue of PictureClerk.\n",
						sizeof("A customer rejoins the regular wait queue of PictureClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedApp[myIndex] == false) {
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
					
					Write("A customer was in the privileged wait queue for ApplicationClerk.\n",
						sizeof("A customer was in the privileged wait queue for ApplicationClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the privileged wait queue of ApplicationClerk.\n",
						sizeof("A customer rejoins the privileged wait queue of ApplicationClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedPic[myIndex] == false) {
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
					
					Write("A customer was in the privileged wait queue for PictureClerk.\n",
						sizeof("A customer was in the privileged wait queue for PictureClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the privileged wait queue of PrivilegedClerk.\n",
						sizeof("A customer rejoins the privileged wait queue of PrivilegedClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedApp[myIndex] == false) {
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
					
					Write("A customer was in the privileged wait queue for ApplicationClerk.\n",
						sizeof("A customer was in the privileged wait queue for ApplicationClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the privileged wait queue of ApplicationClerk.\n",
						sizeof("A customer rejoins the privileged wait queue of ApplicationClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedPic[myIndex] == false) {
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
					
					Write("A customer was in the privileged wait queue for PictureClerk.\n",
						sizeof("A customer was in the privileged wait queue for PictureClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the privileged wait queue of PictureClerk.\n",
						sizeof("A customer rejoins the privileged wait queue of PictureClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedApp[myIndex] == false) {
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
					
					Write("A customer was in the regular wait queue for ApplicationClerk.\n",
						sizeof("A customer was in the regular wait queue for ApplicationClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the regular wait queue of ApplicationClerk.\n",
						sizeof("A customer rejoins the regular wait queue of ApplicationClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedPic[myIndex] == false) {
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
					
					Write("A customer was in the regular wait queue for PictureClerk.\n",
						sizeof("A customer was in the regular wait queue for PictureClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the regular wait queue of PictureClerk.\n",
						sizeof("A customer rejoins the regular wait queue of PictureClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedApp[myIndex] == false) {
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
					
					Write("A customer was in the regular wait queue for ApplicationClerk.\n",
						sizeof("A customer was in the regular wait queue for ApplicationClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the regular wait queue of ApplicationClerk.\n",
						sizeof("A customer rejoins the regular wait queue of ApplicationClerk.\n"),
						ConsoleOutput);
					
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
			
			while (visitedPic[myIndex] == false) {
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
					
					Write("A customer was in the regular wait queue for PictureClerk.\n",
						sizeof("A customer was in the regular wait queue for PictureClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the regular wait queue of PictureClerk.\n",
						sizeof("A customer rejoins the regular wait queue of PictureClerk.\n"),
						ConsoleOutput);
					
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

/* Helper function for customer talking to a passport clerk */
void TalkPassClerk(int myIndex, enum BOOLEAN privLine) {
	int myClerk;
	int i;
	
	/* Search for an available clerk */
	for (i = 0; i < NUM_CLERKS; i++) {
		Acquire(passLock[i]);
		if (passState[i] == AVAILABLE) {
			/* Found him, store it and set him to busy */
			myClerk = i;
			passState[i] = BUSY;
			break;
		}
		else {
			Release(passLock[i]);
		}
	}
	
	/* Give the clerk your index */
	passData[myClerk] = myIndex;
	if (privLine == true) {
		if (fileType[myIndex] == CUSTOMER) {
			Write("A customer is willing to pay 500 to PassportClerk to move ahead in line.\n",
				sizeof("A customer is willing to pay 500 to PassportClerk to move ahead in line.\n"),
				ConsoleOutput);
		}
		else {
			Write("A senator is willing to pay 500 to PassportClerk to move ahead in line.\n",
				sizeof("A senator is willing to pay 500 to PassportClerk to move ahead in line.\n"),
				ConsoleOutput);
		}
	}
	
	/* Signal and wait for him to respond */
	Signal(passCV[myClerk], passLock[myClerk]);
	Wait(passCV[myClerk], passLock[myClerk]);
	
	if (passDataBool[myClerk] == true) {
	/* Success, proceed onwards and upwards. */	
		Release(passLock[myClerk]);
		visitedPass[myIndex] = true;
		if (fileType[myIndex] == CUSTOMER) {
			Write("A customer is certified by PassportClerk.\n",
				sizeof("A customer is certified by PassportClerk.\n"),
				ConsoleOutput);
		}
		else {
			Write("A senator is certified by PassportClerk.\n",
				sizeof("A senator is certified by PassportClerk.\n"),
				ConsoleOutput);
		}
	}
	else {
		Release(passLock[myClerk]);
		if (fileType[myIndex] == CUSTOMER) {
			Write("A customer is not certified by PassportClerk.\n",
				sizeof("A customer is not certified by PassportClerk.\n"),
				ConsoleOutput);
		}
		else {
			Write("A senator is not certified by PassportClerk.\n",
				sizeof("A senator is not certified by PassportClerk.\n"),
				ConsoleOutput);
		}
		
		/* TODO: RANDOM AMOUNTS OF YIELDS BETWEEN 100 AND 1000 */
		Yield();
	}
}

/* Helper function for helping a customer choose which line - privileged or regular - to enter
*	when in the passport clerk's line. */
void LinePassClerk(int myIndex) {

	Acquire(passLineLock);
	/* Check money first */
	if (myCustMoney[myIndex] > 500) {
		
		/* If both lines for him are empty, just go into regular */
		if (regPassLineLength == 0 && privPassLineLength == 0) {
			
			while (visitedPass[myIndex] == false) {
				regPassLineLength++;
				Wait(regPassLineCV, passLineLock);
				Release(passLineLock);
			
				Acquire(senatorLock);
				/* As always, check for senators */
				if (officeSenator > 0 && numPassWait == 0) {
					Release(senatorLock);
					
					Acquire(passLineLock);
					regPassLineLength--;
					Release(passLineLock);
					
					Write("A customer was in the regular wait queue for PassportClerk.\n",
						sizeof("A customer was in the regular wait queue for PassportClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the regular wait queue of PassportClerk.\n",
						sizeof("A customer rejoins the regular wait queue of PassportClerk.\n"),
						ConsoleOutput);
						
					Acquire(passLineLock);
				}
				else {
					numPassWait--;
					Release(senatorLock);
					TalkPassClerk(myIndex, false);
					break;
				}
			}
		}
		/* Else just go into privileged line */
		else {
		
			while (visitedPass[myIndex] == false) {
				privPassLineLength++;
				Wait(privPassLineCV, passLineLock);
				Release(passLineLock);
			
				Acquire(senatorLock);
				/* As always, check for senators */
				if (officeSenator > 0 && numPassWait == 0) {
					Release(senatorLock);
					
					Acquire(passLineLock);
					privPassLineLength--;
					Release(passLineLock);
					
					Write("A customer was in the privileged wait queue for PassportClerk.\n",
						sizeof("A customer was in the privileged wait queue for PassportClerk.\n"),
						ConsoleOutput);
					Write("A customer leaves the Passport Office as a Senator has arrived.\n",
						sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
						ConsoleOutput);
					DoWaitingRoom();
					Write("A customer rejoins the privileged wait queue of PassportClerk.\n",
						sizeof("A customer rejoins the privileged wait queue of PassportClerk.\n"),
						ConsoleOutput);
						
					Acquire(passLineLock);
				}
				else {
					numPassWait--;
					Release(senatorLock);
					TalkPassClerk(myIndex, true);
					
					myCustMoney[myIndex] -= 500;
					break;
				}
			}
		}
	}
	/* Not enough money, regular line for me */
	else {
	
		while (visitedPass[myIndex] == false) {
			regPassLineLength++;
			Wait(regPassLineCV, passLineLock);
			Release(passLineLock);
		
			Acquire(senatorLock);
			/* As always, check for senators */
			if (officeSenator > 0 && numPassWait == 0) {
				Release(senatorLock);
				
				Acquire(passLineLock);
				regPassLineLength--;
				Release(passLineLock);
				
				Write("A customer was in the regular wait queue for PassportClerk.\n",
					sizeof("A customer was in the regular wait queue for PassportClerk.\n"),
					ConsoleOutput);
				Write("A customer leaves the Passport Office as a Senator has arrived.\n",
					sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
					ConsoleOutput);
				DoWaitingRoom();
				Write("A customer rejoins the regular wait queue of PassportClerk.\n",
					sizeof("A customer rejoins the regular wait queue of PassportClerk.\n"),
					ConsoleOutput);
					
				Acquire(passLineLock);
			}
			else {
				numPassWait--;
				Release(senatorLock);
				TalkPassClerk(myIndex, false);
				break;
			}
		}
	}
}

/* Helper function to line and talk with cashier. Cashier only has a regular line. */
void LineTalkCashClerk(int myIndex) {
	int myClerk;
	int i;
	
	/* Just get in cashier's only line */
	while (visitedCash[myIndex] == false) {
		
		Acquire(cashLineLock);
		cashLineLength++;
		Wait(cashLineCV, cashLineLock);
		Release(cashLineLock);
		
		Acquire(senatorLock);
		/* Same as always */
		if (officeSenator > 0 && numCashWait == 0) {
			Release(senatorLock);
			
			Acquire(cashLineLock);
			cashLineLength--;
			Release(cashLineLock);
			
			Write("A customer was in the wait queue for Cashier.\n",
				sizeof("A customer was in the wait queue for Cashier.\n"),
				ConsoleOutput);
			Write("A customer leaves the Passport Office as a Senator has arrived.\n",
				sizeof("A customer leaves the Passport Office as a Senator has arrived.\n"),
				ConsoleOutput);
			DoWaitingRoom();
			Write("A customer rejoins the wait queue of Cashier.\n",
				sizeof("A customer rejoins the wait queue of Cashier.\n"),
				ConsoleOutput);
			
			Acquire(cashLineLock);
		}
		else {
			numCashWait--;
			Release(senatorLock);
			break;
		}
	}
	
	/* Search for an available clerk */
	for (i = 0; i < NUM_CLERKS; i++) {
		Acquire(cashLock[i]);
		if (cashState[i] == AVAILABLE) {
			/* Found him, store it and set him to busy */
			myClerk = i;
			cashState[i] = BUSY;
			break;
		}
		else {
			Release(cashLock[i]);
		}
	}
	
	/* Give the clerk your index */
	cashData[myClerk] = myIndex;
	
	/* Signal and wait for him to respond */
	Signal(cashCV[myClerk], cashLock[myClerk]);
	Wait(cashCV[myClerk], cashLock[myClerk]);
	
	if (cashDataBool[myClerk] == true) {
	/* Success, proceed onwards and upwards. */	
		Release(cashLock[myClerk]);
		visitedCash[myIndex] = true;
		Write("A customer gets valid certification from Cashier.\n",
			sizeof("A customer gets valid certification from Cashier.\n"),
			ConsoleOutput);
		Write("A customer pays $100 to Cashier for their passport.\n",
			sizeof("A customer pays $100 to Cashier for their passport.\n"),
			ConsoleOutput);
		Write("A customer's passport is now recorded by Cashier.\n",
			sizeof("A customer's passport is now recorded by Cashier.\n"),
			ConsoleOutput);
		myCustMoney[myIndex] -= 100;
	}
	else {
		Release(cashLock[myClerk]);
		Write("A customer is not certified by Cashier.\n",
			sizeof("A customer is not certified by Cashier.\n"),
			ConsoleOutput);
		Write("A customer is punished to wait by Cashier.\n",
			sizeof("A customer is punished to wait by Cashier.\n"),
			ConsoleOutput);
		
		/* TODO: RANDOM AMOUNTS OF YIELDS BETWEEN 100 AND 1000 */
		Yield();
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
	
	while (visitedApp[myIndex] != true || visitedPic[myIndex] != true || visitedPass[myIndex] != true || visitedCash[myIndex] != true) {
		
		/* Do we need to make a random chance to visit the passport clerk first? */
		/* TODO: VISIT THE PASSPORT CLERK RANDOM CHANCE */
		
		/* Visit application and picture clerks first */
		while (visitedApp[myIndex] != true || visitedPic[myIndex] != true) {
			
			/* Check for senators before getting into line */
			Acquire(senatorLock);
			if (officeSenator > 0) {
				Release(senatorLock);
				DoWaitingRoom();
			}
			else {
				Release(senatorLock);
			}
			Write("Getting in line.\n", sizeof("Getting in line.\n"), ConsoleOutput);
			LineAppPicClerk(myIndex);
		}
		
		/* Next visit the passport clerk */
		while (visitedPass[myIndex] != true) {
			
			/* Check for senators first */
			Acquire(senatorLock);
			if (officeSenator > 0) {
				Release(senatorLock);
				DoWaitingRoom();
			}
			else {
				Release(senatorLock);
			}
			LinePassClerk(myIndex);
		}
		
		/* Finally, visit the cashier */
		while (visitedCash[myIndex] != true) {
			
			/* ...senators */
			Acquire(senatorLock);
			if (officeSenator > 0) {
				Release(senatorLock);
				DoWaitingRoom();
			}
			else {
				Release(senatorLock);
			}
			LineTalkCashClerk(myIndex);
		}
	}
	
	/* Finished with this office, leaving */
	Acquire(customerLock);
	officeCustomer--;
	Release(customerLock);
	Write("A customer leaves the Passport Office.\n", sizeof("A customer leaves the Passport Office.\n"), ConsoleOutput);
	Exit(0);
}

/* Helper function for senator checking which line to enter */
void SenLineAppPicClerk(int myIndex) {

	Acquire(acpcLineLock);
	/* First check if have enough money to enter a privileged line */
	if (myCustMoney[myIndex] > 500) {
		
		/* If haven't gone to application clerk, go there if both of his lines are empty.
		*	Go into regular line to save money because both are empty. */
		if (regACLineLength == 0 && privACLineLength == 0 && visitedApp[myIndex] == false) {
			
			Write("A senator is getting into regular app line.\n",
				sizeof("A senator is getting into regular app line.\n"),
				ConsoleOutput);
			regACLineLength++;
			Wait(regACLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Acquire(senatorLock);
			numAppWait--;
			Release(senatorLock);
			TalkAppClerk(myIndex, false);
		}
		/* Same as above except for pic clerk */
		else if (regPCLineLength == 0 && privPCLineLength == 0 && visitedPic[myIndex] == false) {
			
			regPCLineLength++;
			Wait(regPCLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Acquire(senatorLock);
			numPicWait--;
			Release(senatorLock);
			TalkPicClerk(myIndex, false);
		}
		/* Already been to picture clerk, so go to privileged application clerk line*/
		else if (visitedPic[myIndex] == true) {
			
			privACLineLength++;
			Wait(privACLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Acquire(senatorLock);
			numAppWait--;
			Release(senatorLock);
			TalkAppClerk(myIndex, true);
			myCustMoney[myIndex] -= 500;
		}
		/* If already been to application clerk then go to privileged picture clerk line */
		else if (visitedApp[myIndex] == true) {
			
				privPCLineLength++;
				Wait(privPCLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				numPicWait--;
				Release(senatorLock);
				TalkPicClerk(myIndex, true);
				myCustMoney[myIndex] -= 500;
		}
		/* If application clerk's privileged line length is shorter than picture clerk's */
		else if (privACLineLength <= privPCLineLength) {
			
			privACLineLength++;
			Wait(privACLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Acquire(senatorLock);
			numAppWait--;
			Release(senatorLock);
			TalkAppClerk(myIndex, true);
			
			myCustMoney[myIndex] -= 500;
		}
		/* Else picture's is shorter */
		else {
			privPCLineLength++;
			Wait(privPCLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Acquire(senatorLock);
			numPicWait--;
			Release(senatorLock);
			TalkAppClerk(myIndex, true);
			myCustMoney[myIndex] -= 500;
		}
	}
	/* Don't have enough money, just check regular lines */
	else {
		if (visitedPic[myIndex] == true) {
			
			regACLineLength++;
			Wait(regACLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Acquire(senatorLock);
			numAppWait--;
			Release(senatorLock);
			TalkAppClerk(myIndex, false);
		}
		/* If already been to application clerk then go to privileged picture clerk line */
		else if (visitedApp[myIndex] == true) {
			
				regPCLineLength++;
				Wait(regPCLineCV, acpcLineLock);
				Release(acpcLineLock);
				
				Acquire(senatorLock);
				numPicWait--;
				Release(senatorLock);
				TalkPicClerk(myIndex, false);
		}
		/* If application clerk's privileged line length is shorter than picture clerk's */
		else if (regACLineLength <= regPCLineLength) {
			
			regACLineLength++;
			Wait(regACLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Acquire(senatorLock);
			numAppWait--;
			Release(senatorLock);
			TalkAppClerk(myIndex, false);
		}
		/* Else picture's is shorter */
		else {
			
			regPCLineLength++;
			Wait(regPCLineCV, acpcLineLock);
			Release(acpcLineLock);
			
			Acquire(senatorLock);
			numPicWait--;
			Release(senatorLock);
			TalkAppClerk(myIndex, false);
		}
	}
}

/* Helper function for senator to enter passport clerk line */
void SenLinePassClerk(int myIndex) {

	Acquire(passLineLock);
	/* Check money first */
	if (myCustMoney[myIndex] > 500) {
		
		/* If both lines for him are empty, just go into regular */
		if (regPassLineLength == 0 && privPassLineLength == 0) {
				
			regPassLineLength++;
			Wait(regPassLineCV, passLineLock);
			Release(passLineLock);
		
			Acquire(senatorLock);
			numPassWait--;
			Release(senatorLock);
			TalkPassClerk(myIndex, false);
		}
		/* Else just go into privileged line */
		else {
		
			privPassLineLength++;
			Wait(privPassLineCV, passLineLock);
			Release(passLineLock);
		
			Acquire(senatorLock);
			numPassWait--;
			Release(senatorLock);
			TalkPassClerk(myIndex, true);
			
			myCustMoney[myIndex] -= 500;
		}
	}
	/* Not enough money, regular line for me */
	else {
	
		regPassLineLength++;
		Wait(regPassLineCV, passLineLock);
		Release(passLineLock);
	
		Acquire(senatorLock);
		numPassWait--;
		Release(senatorLock);
		TalkPassClerk(myIndex, false);
	}
}

/* Helper function for senator to line and talk to cashier. */
void SenLineTalkCashClerk(int myIndex) {
	int myClerk;
	int i;
	
	/* Just get in cashier's only line */
	
	Acquire(cashLineLock);
	cashLineLength++;
	Wait(cashLineCV, cashLineLock);
	Release(cashLineLock);
	
	Acquire(senatorLock);
	numCashWait--;
	Release(senatorLock);
	
	/* Search for an available clerk */
	for (i = 0; i < NUM_CLERKS; i++) {
		Acquire(cashLock[i]);
		if (cashState[i] == AVAILABLE) {
			/* Found him, store it and set him to busy */
			myClerk = i;
			cashState[i] = BUSY;
			break;
		}
		else {
			Release(cashLock[i]);
		}
	}
	
	/* Give the clerk your index */
	cashData[myClerk] = myIndex;
	
	/* Signal and wait for him to respond */
	Signal(cashCV[myClerk], cashLock[myClerk]);
	Wait(cashCV[myClerk], cashLock[myClerk]);
	
	if (cashDataBool[myClerk] == true) {
	/* Success, proceed onwards and upwards. */	
		Release(cashLock[myClerk]);
		visitedCash[myIndex] = true;
		Write("A customer gets valid certification from Cashier.\n",
			sizeof("A customer gets valid certification from Cashier.\n"),
			ConsoleOutput);
		Write("A customer pays $100 to Cashier for their passport.\n",
			sizeof("A customer pays $100 to Cashier for their passport.\n"),
			ConsoleOutput);
		Write("A customer's passport is now recorded by Cashier.\n",
			sizeof("A customer's passport is now recorded by Cashier.\n"),
			ConsoleOutput);
		myCustMoney[myIndex] -= 100;
	}
	else {
		Release(cashLock[myClerk]);
		Write("A customer is not certified by Cashier.\n",
			sizeof("A customer is not certified by Cashier.\n"),
			ConsoleOutput);
		Write("A customer is punished to wait by Cashier.\n",
			sizeof("A customer is punished to wait by Cashier.\n"),
			ConsoleOutput);
		
		/* TODO: RANDOM AMOUNTS OF YIELDS BETWEEN 100 AND 1000 */
		Yield();
	}
}

/* Senator's forked function thread.
*	Runs similarly to Customer except only enters when customers are out of the office. */
void Senator() {
	int myIndex;
	int i;
	
	/* Gives this customer the first available customer/senator index and sets their type to CUSTOMER */
	Acquire(indexLock);
	for (i = 0; i < NUM_CUSTOMERS + NUM_SENATORS; i++) {
		if (customerIndex[i] == FREE) {
			myIndex = i;
			customerIndex[i] = USED;
			fileType[i] = SENATOR;
			break;
		}
	}
	Release(indexLock);
	
	/* Before entering, check for customers */
	Acquire(senatorLock);
	Write("A senator is entering the Passport Office.\n", 
		sizeof("A senator is entering the Passport Office.\n"),
		ConsoleOutput);
	officeSenator++;
	Release(senatorLock);
	
	Acquire(customerLock);
	/* If there are customers still inside, wait for them to leave. */
	if (officeCustomer > 0) {
		Release(customerLock);
		
		Acquire(senWaitLock);
		Write("A senator is waiting.\n", sizeof("A senator is waiting.\n"), ConsoleOutput);
		Wait(senWaitCV, senWaitLock);
		Write("A senator has stopped waiting.\n", sizeof("A senator has stopped waiting.\n"), ConsoleOutput);
		Release(senWaitLock);
	}
	else {
		Release(customerLock);
	}
	
	while (visitedApp[myIndex] != true || visitedPic[myIndex] != true || visitedPass[myIndex] != true || visitedCash[myIndex] != true) {
		
		/* Do we need to make a random chance to visit the passport clerk first? */
		/* TODO: VISIT THE PASSPORT CLERK RANDOM CHANCE */
		
		/* Visit application and picture clerks first */
		while (visitedApp[myIndex] != true || visitedPic[myIndex] != true) {
			SenLineAppPicClerk(myIndex);
		}
		
		/* Next visit the passport clerk */
		while (visitedPass[myIndex] != true) {
			SenLinePassClerk(myIndex);
		}
		
		/* Finally, visit the cashier */
		while (visitedCash[myIndex] != true) {
			SenLineTalkCashClerk(myIndex);
		}
	}
	
	/* Finished with this office, leaving */
	Acquire(senatorLock);
	officeSenator--;
	Release(senatorLock);
	Write("A senator leaves the Passport Office.\n", sizeof("A senator leaves the Passport Office.\n"), ConsoleOutput);
	Exit(0);
}
	
int main() {
	int i;
	Write("Calling InitializeData\n", sizeof("Calling InitializeData\n"), ConsoleOutput);
	InitializeData();
	Write("InitializeData has been called.\n", sizeof("InitializeData has been called.\n"), ConsoleOutput);
	
	for (i = 0; i < NUM_CUSTOMERS; i++) {
		Fork(Customer);
	}
	
	for (i = 0; i < NUM_CLERKS; i++) {
		Fork(AppClerk);
		Fork(PicClerk);
		Fork(PassClerk);
		Fork(CashClerk);
	}
	
	for (i = 0; i < NUM_SENATORS; i++) {
		Fork(Senator);
	}
	
	Fork(Manager);
	
	Exit(0);
}
 