#include "office.h"
#include "officeMonitor.h"
#include "system.h"
#include <ctime>		// For seeding random
#include <cstdlib>	// For generating random


Office::Office(){
	srand(time(0));		//Initializing random number generator
}

void Office::startOffice(int numCust, int numApp, int numPic,
		         int numPass, int numCash) {
	//oMonitor = new OfficeMonitor(numApp, numPic, numPass, numCash);
	OfficeMonitor oMonitor(numApp, numPic, numPass, numCash);
	//oMonitor = new OfficeMonitor();
	Thread *t;

	oMonitor.addCustomer(numCust);
	for(int i = 0; i < numCust; i++) {
		t = new Thread("Cust%d",i);
		t->Fork((VoidFunctionPtr) Customer, i);
	} 
	
	for(int i = 0; i < numApp; i++) {
		t = new Thread("AppClerk%d",i);
		t->Fork((VoidFunctionPtr) AppClerk, i);
	}

	for(int i = 0; i < numPic; i++) {
		t = new Thread("PicClerk%d",i);
		t->Fork((VoidFunctionPtr) PicClerk, i);
	}

	for(int i = 0; i < numPass; i++) {
		t = new Thread("PassClerk%d",i);
		t->Fork((VoidFunctionPtr) PassClerk, i);
	}

	for(int i = 0; i < numCash; i++) {
		t = new Thread("Cashier%d",i);
		t->Fork((VoidFunctionPtr) Cashier, i);
	}

	t = new Thread("Manager");
	t->Fork((VoidFunctionPtr) Manager, 0);  
}

/*
//	Yinlerthai Chan
//	Application Clerk function thread
//	Receives an "application" from the Customer in the form of an SSN
//	Passes the SSN data to the Passport Clerk
//	Files the Customer's application, then dismisses the Customer
//	
*/
void Office::AppClerk(int index){
	int myIndex = index;		// index of AppClerk	
	int mySSN;			// current SSN being filed + index of Customer

	oMonitor.appLock[myIndex]->Acquire();
	oMonitor.appState[myIndex] = oMonitor.BUSY;	// start off AppClerk state being BUSY
	oMonitor.appLock[myIndex]->Release();

	// Endless loop to perform AppClerk actions
	while(true){
		oMonitor.acpcLineLock->Acquire();

		// Check for the privileged customer line first
		// If there are privileged customers, do AppClerk tasks, then received $500
		if(oMonitor.privACLineLength > 0){
			oMonitor.privACLineLength--;
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = oMonitor.AVAILABLE;
			oMonitor.privACLineCV->Signal(oMonitor.acpcLineLock);		// Signals the next customer in priv line
			oMonitor.acpcLineLock->Release();
			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);	// Waits for the next customer

			mySSN = oMonitor.appData[myIndex];
			oMonitor.fileLock[mySSN]->Acquire();
			if(oMonitor.fileState[mySSN] == oMonitor.NONE){
				oMonitor.fileState[mySSN] = oMonitor.APPDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else if(oMonitor.fileState[mySSN] == oMonitor.PICDONE){
				oMonitor.fileState[mySSN] = oMonitor.APPPICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else{
				printf("Error. Customer does not have picture application or no application. What are you doing here? \n");
				oMonitor.fileLock[mySSN]->Release();
			}

			// tell passport ssn

			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}
			printf("ApplicationClerk%d informs Customer%d that the application has been filed\n", myIndex, mySSN);


			oMonitor.appMoneyLock->Acquire();
			oMonitor.appMoney += 500;
			printf("ApplicationClerk%d accepts money = $500 from Customer%d\n",myIndex,mySSN);

			oMonitor.appMoneyLock->Release();

			oMonitor.appCV[myIndex]->Signal(oMonitor.appLock[myIndex]); // signal customer awake
			oMonitor.appLock[myIndex]->Release();     // release clerk lock
		}
		// Check for regular customer line next
		// If there are regular customers, do AppClerk tasks
		else if(oMonitor.regACLineLength > 0){
			oMonitor.regACLineLength--;
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = oMonitor.AVAILABLE;
			oMonitor.regACLineCV->Signal(oMonitor.acpcLineLock);
			oMonitor.acpcLineLock->Release();
			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);

			mySSN = oMonitor.appData[myIndex];
			oMonitor.fileLock[mySSN]->Acquire();
			if(oMonitor.fileState[mySSN] == oMonitor.NONE){
				oMonitor.fileState[mySSN] = oMonitor.APPDONE;
				oMonitor.fileLock[mySSN]->Release();

			}
			else if(oMonitor.fileState[mySSN] == oMonitor.PICDONE){
				oMonitor.fileState[mySSN] = oMonitor.APPPICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else{
				printf("Error. Customer does not have picture application or no application. What are you doing here?\n");
				oMonitor.fileLock[mySSN]->Release();
			}

			// tell passport ssn

			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}
			printf("ApplicationClerk%d informs Customer%d that the application has been filed\n", myIndex, mySSN);

			oMonitor.appCV[myIndex]->Signal(oMonitor.appLock[myIndex]); // signal customer awake
			oMonitor.appLock[myIndex]->Release();     // release clerk lock
		}
		// If there are neither privileged or regular customers, go on break
		else{

			oMonitor.acpcLineLock->Release();
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = oMonitor.BREAK;
			printf("ApplicationClerk%d is going on break\n",myIndex);

			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);
			printf("ApplicationClerk%d returned from break\n",myIndex);

			oMonitor.appLock[myIndex]->Release();     // release clerk lock

			// go on break
		}
	}
}

/*
//	Yinlerthai Chan:
//	Picture Clerk function thread
//	Takes a "picture" of the Customer
//	If the Customer dislikes the picture, will continue to take pictures until the Customer approves it
//	Once approved, the Picture Clerk will file the picture, then dismiss the Customer
*/

void Office::PicClerk(int index){
	int myIndex = index;		// index of PicClerk
	int mySSN;			// index of Customer

	oMonitor.picLock[myIndex]->Acquire();
	oMonitor.picState[myIndex] = oMonitor.BUSY;	// start off PicClerk state being BUSY
	oMonitor.picLock[myIndex]->Release();

	// Endless loop for PicClerk actions
	while(true){
		oMonitor.acpcLineLock->Acquire();

		// Check for the privileged customer line first
		// If there are privileged customers, do PicClerk tasks, then received $500
		if(oMonitor.privACLineLength > 0){
			oMonitor.privPCLineLength--;
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = oMonitor.AVAILABLE;	// Sets itself to AVAILABLE
			oMonitor.privPCLineCV->Signal(oMonitor.acpcLineLock);		// Signals the next customer in line
			oMonitor.acpcLineLock->Release();		
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);	// Waits for the next customer

			mySSN = oMonitor.picData[myIndex];

			printf("PictureClerk%d takes picture of Customer%d\n",myIndex,mySSN);
			while(oMonitor.picDataBool[myIndex] == false){
				for(int i = 0; i < 4; i++){
					currentThread->Yield();
				}
				oMonitor.picCV[myIndex]->Signal(oMonitor.picLock[myIndex]);	//
				oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);	// Shows the customer the picture

				if(oMonitor.picDataBool[myIndex] ==false){
					printf("PictureClerk%d takes picture of Customer%d again\n",myIndex,mySSN);
		}
				// yield to take picture
				// print statement: "Taking picture"
				// picCV->Signal then picCV->Wait to
				// show customer picture
			}

			oMonitor.fileLock[mySSN]->Acquire();
			if(oMonitor.fileState[mySSN] == oMonitor.NONE){
				oMonitor.fileState[mySSN] = oMonitor.PICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else if(oMonitor.fileState[mySSN] == oMonitor.APPDONE){
				oMonitor.fileState[mySSN] = oMonitor.APPPICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else{
				printf("Error. Customer does not have either an application or no application. What are you doing here?\n");
				oMonitor.fileLock[mySSN]->Release();
			}
			// file picture using
			// current thread yield
			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}
			printf("Picture%d informs Customer%d that the procedure has been completed\n", myIndex, mySSN);

			// signal customer awake
			oMonitor.picMoneyLock->Acquire();
			oMonitor.picMoney += 500;
			printf("PictureClerk%d accepts money = $500 from Customer%d\n",myIndex,mySSN);
			oMonitor.picMoneyLock->Release();

			oMonitor.picCV[myIndex]->Signal(oMonitor.picLock[myIndex]); // signal customer awake
			oMonitor.picLock[myIndex]->Release();// release clerk lock
		}
		// Check for the regular customer line next
		// If there are regular customers, do PicClerk tasks
		else if(oMonitor.regPCLineLength > 0){
			oMonitor.regPCLineLength--;
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = oMonitor.AVAILABLE;		// sets itself to AVAILABLE
			oMonitor.regPCLineCV->Signal(oMonitor.acpcLineLock);			// Signals the next customer in line
			oMonitor.acpcLineLock->Release();
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);		// Waits for next customer

			mySSN = oMonitor.picData[myIndex];

			printf("PictureClerk%d takes picture of Customer%d\n",myIndex,mySSN);
			while(oMonitor.picDataBool[myIndex] == false){
				for(int i = 0; i < 4; i++){
					currentThread->Yield();
				}
				oMonitor.picCV[myIndex]->Signal(oMonitor.picLock[myIndex]);	//
				oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);	// Shows the customer the picture
				
				if(oMonitor.picDataBool[myIndex] ==false){
					printf("PictureClerk%d takes picture of Customer%d again\n",myIndex,mySSN);
				}
				// yield to take picture
				// print statement: "Taking picture"
				// picCV->Signal then picCV->Wait to
				// show customer picture
			}

			oMonitor.fileLock[mySSN]->Acquire();
			if(oMonitor.fileState[mySSN] == oMonitor.NONE){
				oMonitor.fileState[mySSN] = oMonitor.PICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else if(oMonitor.fileState[mySSN] == oMonitor.APPDONE){
				oMonitor.fileState[mySSN] = oMonitor.APPPICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else{
				printf("Error. Customer does not have either an application or no application. What are you doing here?\n");
				oMonitor.fileLock[mySSN]->Release();
			}

			// file picture using
			// current thread yield
			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}
			printf("Picture%d informs Customer%d that the procedure has been completed\n", myIndex, mySSN);

			// signal customer awake
			oMonitor.picCV[myIndex]->Signal(oMonitor.picLock[myIndex]); // signal customer awake
			oMonitor.picLock[myIndex]->Release();// release clerk lock
		}
		// If there are neither privileged or regular customers, go on break
		else{
			oMonitor.acpcLineLock->Release();
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = oMonitor.BREAK;
			printf("ApplicationClerk%d is going on break\n",myIndex);
			oMonitor.picLock[myIndex]->Release();
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);

			printf("ApplicationClerk%d returned from break\n",myIndex);

			oMonitor.picLock[myIndex]->Release();     // release clerk lock
			// go on break
		}
	}
}

/*
//	Yinlerthai Chan:
//	Manager function thread
//	Constantly checks over customer lines to see if there are any customers
//	Wakes up Clerks on break if so
// Also periodically prints out a money report, stating how much money each Clerk has, and total money overall
*/

void Office::Manager(){
	int totalMoney;

	while(true){
		totalMoney = 0;

		// Checks for AppClerk on break
		if(oMonitor.privACLineLength > 3 || oMonitor.regACLineLength > 3){
			for(int i = 0; i < oMonitor.numAppClerks; i++){
				if(oMonitor.appState[i] == oMonitor.BREAK){
					oMonitor.appLock[i]->Acquire();
					oMonitor.appCV[i]->Signal(oMonitor.appLock[i]);
					oMonitor.appState[i] = oMonitor.BUSY;
					oMonitor.appLock[i]->Release();
					printf("Manager calls back an ApplicationClerk from break\n");
				}
			}						
		}
		else if(oMonitor.privACLineLength > 1 || oMonitor.regACLineLength > 1){
			for(int i = 0; i < oMonitor.numAppClerks; i++){
				if(oMonitor.appState[i] == oMonitor.BREAK){
					oMonitor.appLock[i]->Acquire();
					oMonitor.appCV[i]->Signal(oMonitor.appLock[i]);
					oMonitor.appState[i] = oMonitor.BUSY;
					oMonitor.appLock[i]->Release();
					printf("Manager calls back an ApplicationClerk from break\n");
					break;
				}
			}						
		}
		// Checks for PictureClerks on break
		if(oMonitor.privPCLineLength > 3 || oMonitor.regPCLineLength > 3){
			for(int i = 0; i < oMonitor.numPicClerks; i++){
				if(oMonitor.picState[i] == oMonitor.BREAK){
					oMonitor.picLock[i]->Acquire();
					oMonitor.picCV[i]->Signal(oMonitor.picLock[i]);
					oMonitor.picState[i] = oMonitor.BUSY;
					oMonitor.picLock[i]->Release();
					printf("Manager calls back an PictureClerk from break\n");
				}
			}						
		}
		else if(oMonitor.privPCLineLength > 1 || oMonitor.regPCLineLength > 1){
			for(int i = 0; i < oMonitor.numPicClerks; i++){
				if(oMonitor.picState[i] == oMonitor.BREAK){
					oMonitor.picLock[i]->Acquire();
					oMonitor.picCV[i]->Signal(oMonitor.picLock[i]);
					oMonitor.picState[i] = oMonitor.BUSY;
					oMonitor.picLock[i]->Release();
					printf("Manager calls back an PictureClerk from break\n");
					break;
				}
			}						
		}

		// Checks for PassportClerks on break
		if(oMonitor.privPassLineLength > 3 || oMonitor.regPassLineLength > 3){
			for(int i = 0; i < oMonitor.numPassClerks; i++){
				if(oMonitor.passState[i] == oMonitor.BREAK){
					oMonitor.passLock[i]->Acquire();
					oMonitor.passCV[i]->Signal(oMonitor.passLock[i]);
					oMonitor.passState[i] = oMonitor.BUSY;
					oMonitor.passLock[i]->Release();
					printf("Manager calls back an PassportClerk from break\n");
				}
			}						
		}
		else if(oMonitor.privPassLineLength > 1 || oMonitor.regPassLineLength > 1){
			for(int i = 0; i < 	oMonitor.numPassClerks; i++){
				if(oMonitor.passState[i] == oMonitor.BREAK){
					oMonitor.passLock[i]->Acquire();
					oMonitor.passCV[i]->Signal(oMonitor.passLock[i]);
					oMonitor.passState[i] = oMonitor.BUSY;
					oMonitor.passLock[i]->Release();
					printf("Manager calls back an PassportClerk from break\n");
					break;
				}
			}						
		}

		// Checks for Cashier on break
		if(oMonitor.cashLineLength > 3){
			for(int i = 0; i < 	oMonitor.numCashiers; i++){
				if(oMonitor.cashState[i] == oMonitor.BREAK){
					oMonitor.cashLock[i]->Acquire();
					oMonitor.cashCV[i]->Signal(oMonitor.cashLock[i]);
					oMonitor.cashState[i] = oMonitor.BUSY;
					oMonitor.cashLock[i]->Release();
					printf("Manager calls back an Cashier from break\n");
				}
			}						
		}
		else if(oMonitor.cashLineLength > 1){
			for(int i = 0; i < 	oMonitor.numCashiers; i++){
				if(oMonitor.cashState[i] == oMonitor.BREAK){
					oMonitor.cashLock[i]->Acquire();
					oMonitor.cashCV[i]->Signal(oMonitor.cashLock[i]);
					oMonitor.cashState[i] = oMonitor.BUSY;
					oMonitor.cashLock[i]->Release();
					printf("Manager calls back an Cashier from break\n");
					break;
				}
			}						
		}

		// Print out money periodically (must figure out Timer stuff)

		oMonitor.appMoneyLock->Acquire();
		printf("Total money received from ApplicationClerk = %d\n",oMonitor.appMoney);
		totalMoney += oMonitor.appMoney;
		oMonitor.appMoneyLock->Release();
		
		oMonitor.picMoneyLock->Acquire();
		printf("Total money received from PictureClerk = %d\n",oMonitor.picMoney);
		totalMoney += oMonitor.picMoney;
		oMonitor.picMoneyLock->Release();

		oMonitor.passMoneyLock->Acquire();
		printf("Total money received from PassportClerk = %d\n",oMonitor.passMoney);
		totalMoney += oMonitor.passMoney;
		oMonitor.passMoneyLock->Release();

		oMonitor.cashMoneyLock->Acquire();
		printf("Total money received from Cashier = %d\n",oMonitor.cashMoney);
		totalMoney += oMonitor.cashMoney;
		oMonitor.cashMoneyLock->Release();

		printf("Total money made by office = %d\n",totalMoney);

	}

}


/*
//	Antonio Cade:
//	Passport Clerk function thread
//	Records customer passport
*/

void Office::PassClerk(int index) {
	int myIndex = index;
	int myCust;
	bool doPassport = false;

	// set own state to busy
	oMonitor.passLock[myIndex]->Acquire();
	oMonitor.passState[myIndex] = oMonitor.BUSY;

	while (true) {
		// Check for customers in line
		oMonitor.passLineLock->Acquire();
		if (oMonitor.privPassLineLength > 0) {
			// Decrement line length, set state to AVAIL, signal 1st customer and wait for them
			oMonitor.privPassLineLength --;
			oMonitor.passState[myIndex] = oMonitor.AVAILABLE;

			oMonitor.privPassLineCV->Signal(oMonitor.passLineLock);
			oMonitor.passLineLock->Release();
			oMonitor.passCV[myIndex]->Wait(oMonitor.passLock[index]);	// wait for customer to signal me
			
			myCust = oMonitor.passData[myIndex];	// customer gave me their SSN/index to check their file
			oMonitor.fileLock[myCust]->Acquire();	// gain access to customer state
			if (oMonitor.fileState[myCust] == oMonitor.APPPICDONE) {
				// customer wasn't a dumbass, DO WORK
				oMonitor.passDataBool[myIndex] = true;
				doPassport = true;
			} else {
				// customer WAS a dumbass.... MAKE THEM PAY
				//for (int i = 0; i < 500) {
				//	currentThread->Yield();
				//}

				oMonitor.passDataBool[myIndex] = false;
				doPassport = false;
			}
			oMonitor.fileLock[myCust]->Release();
			
			// add $500 to passClerk money amount for privilaged fee
			// Dolla dolla bill, y'all
			oMonitor.passMoneyLock->Acquire();
			oMonitor.passMoney += 500;
			oMonitor.passMoneyLock->Release();

			oMonitor.passCV[myIndex]->Signal(oMonitor.passLock[myIndex]);	// signal customer awake
			oMonitor.passLock[myIndex]->Release();							// release clerk lock

			//if customer wasn't an idiot, process their passport
			if (doPassport) {
				for (int i = 0; i < 300; i++) {
					currentThread->Yield();
				}
				// file dat passport
				oMonitor.fileLock[myCust]->Acquire();
				oMonitor.fileState[myCust] = oMonitor.PASSDONE;
				oMonitor.fileLock[myCust]->Release();
			}
		} else if (oMonitor.regPassLineLength > 0) {
			// Decrement line length, set state to AVAIL, signal 1st customer and wait for them
			oMonitor.regPassLineLength --;
			oMonitor.passState[myIndex] = oMonitor.AVAILABLE;

			oMonitor.regPassLineCV->Signal(oMonitor.passLineLock);
			oMonitor.passLineLock->Release();
			oMonitor.passCV[myIndex]->Wait(oMonitor.passLock[index]);	// wait for customer to signal me
			
			myCust = oMonitor.passData[myIndex];	// customer gave me their SSN/index to check their file
			oMonitor.fileLock[myCust]->Acquire();	// gain access to customer state
			if (oMonitor.fileState[myCust] == oMonitor.APPPICDONE) {
				// customer wasn't a dumbass, DO WORK
				oMonitor.passDataBool[myIndex] = true;
				doPassport = true;
			} else {
				// customer WAS a dumbass.... MAKE THEM PAY
				//for (int i = 0; i < 500) {
				//	currentThread->Yield();
				//}

				oMonitor.passDataBool[myIndex] = false;
				doPassport = false;
			}
			oMonitor.fileLock[myCust]->Release();

			oMonitor.passCV[myIndex]->Signal(oMonitor.passLock[myIndex]);	// signal customer awake
			oMonitor.passLock[myIndex]->Release();							// release clerk lock

			//if customer wasn't an idiot, process their passport
			if (doPassport) {
				for (int i = 0; i < 300; i++) {
					currentThread->Yield();
				}
				// file dat passport
				oMonitor.fileLock[myCust]->Acquire();
				oMonitor.fileState[myCust] = oMonitor.PASSDONE;
				oMonitor.fileLock[myCust]->Release();
			}
		} else {
			// No one in line... Pull out your DS and take a break
			oMonitor.passLineLock->Release();
			oMonitor.passLock[myIndex]->Acquire();
			oMonitor.passState[myIndex] = oMonitor.BREAK;
			oMonitor.passCV[myIndex]->Wait(oMonitor.passLock[myIndex]);

			oMonitor.passLock[myIndex]->Release();
		}
	}
}

/*
//	Antonio Cade:
//	Cashier function thread
//	Takes the customer's monies
*/

void Office::Cashier(int index) {
	int myIndex = index;
	int myCust;
	bool doCash = false;

	// set own state to busy
	oMonitor.cashLock[myIndex]->Acquire();
	oMonitor.cashState[myIndex] = oMonitor.BUSY;
	//oMonitor.passLock[index]->Release();

	while (true) {
		// Check for customers in line
		oMonitor.cashLineLock->Acquire();
		if (oMonitor.cashLineLength > 0) {
			// Decrement line length, set state to AVAIL, signal 1st customer and wait for them
			oMonitor.cashLineLength --;
			oMonitor.cashState[myIndex] = oMonitor.AVAILABLE;

			oMonitor.cashLineCV->Signal(oMonitor.cashLineLock);		// signal the customer
			oMonitor.cashLineLock->Release();
			oMonitor.cashCV[myIndex]->Wait(oMonitor.cashLock[myIndex]);	// wait for customer to signal me

			myCust = oMonitor.cashData[myIndex];		// customer gave me their SSN
			oMonitor.fileLock[myCust]->Acquire();	// gain access to customer state
			if (oMonitor.fileState[myCust] == oMonitor.PASSDONE) {
				// customer wasn't a dumbass, DO WORK
				oMonitor.passDataBool[myIndex] = true;
				for (int i = 0; i < 50; i++) {
					currentThread->Yield();
				}
				oMonitor.fileState[myCust] = oMonitor.ALLDONE;

				// add $100 to passClerk money amount for passport fee
				oMonitor.cashMoneyLock->Acquire();
				oMonitor.cashMoney += 100;
				oMonitor.cashMoneyLock->Release();
			} else {
				// customer WAS a dumbass.... MAKE THEM PAY
				//for (int i = 0; i < 500) {
				//	currentThread->Yield();
				//}

				oMonitor.passDataBool[myIndex] = false;
			}
			oMonitor.fileLock[myCust]->Release();

			oMonitor.cashCV[myIndex]->Signal(oMonitor.cashLock[myIndex]);	// signal customer awake
			oMonitor.cashLock[myIndex]->Release();							// release clerk lock
		} else {
			// No one in line... Pull out your DS and take a break
			oMonitor.cashLineLock->Release();
			oMonitor.cashLock[myIndex]->Acquire();
			oMonitor.cashState[myIndex] = oMonitor.BREAK;
			oMonitor.cashCV[myIndex]->Wait(oMonitor.cashLock[myIndex]);

			oMonitor.cashLock[myIndex]->Release();
		}
	}
}

// Jasper Lee:
//	Function for the customer thread
//	Takes in an index that acts as the customer's unique ID
//	Runs infinitely in a while loop until has visited all clerks

void Office::Customer(int index) {
	int myCash = doRandomCash(); // Random amount of cash: 100, 600, 1100, 1600
	int SSN = index; // SSN is the index passed in, determined by order of creation
	
	// boolean flags to remember which clerks have been visited.
	bool visitedApp = false, visitedPic = false, visitedPass = false, visitedCash = false; 

	//Data for being a stupid customer and visiting passport before app/pic clerks
	int chanceToBeStupid = rand() % 2; // 0 or 1
	bool beenStupid = false; // Has only one chance to be stupid
	
	while(!visitedApp || !visitedPic || !visitedPass || !visitedCash) {	
	//to keep thread running, while any of the visited bool flags are false

		if (chanceToBeStupid == 1 && !beenStupid) {

			linePassClerk(myCash, SSN, visitedPass);
			beenStupid = true;
		}
		else {
			beenStupid = true;
		}

		while (!visitedApp && !visitedPic) {

			lineAppPicClerk(myCash, SSN, visitedApp, visitedPic);
		}

		while (!visitedPass) {

			linePassClerk(myCash, SSN, visitedPass);
		}

		while (!visitedCash) {

			lineCashier(myCash, SSN, visitedCash);
		}
	}
}

// Jasper Lee:
// 	Helper function for Customer/Senator to get in line for the
// 	Application OR Picture clerks. Takes in reference to customer's cash
// 	SSN, and visited boolean flags for these two clerks.
void Office::lineAppPicClerk(int& myCash, int& SSN, bool& visitedApp,
							bool& visitedPic) {
	oMonitor.acpcLineLock->Acquire();
	if (myCash > 500) {
	// If have enough cash to go into a priv line

		if (oMonitor.regACLineLength == 0 && oMonitor.privACLineLength == 0 
			&& !visitedApp) {
		// If both priv and reg lines are empty for appClerk, and haven't
		// visited him, just enter his regular line.
			
			oMonitor.regACLineLength++;
			oMonitor.regACLineCV->Wait(oMonitor.acpcLineLock);
			talkAppClerk(SSN, visitedApp);
		}
		else if (oMonitor.regPCLineLength == 0 && oMonitor.privPCLineLength == 0 
				 && !visitedPic) {
		// If both priv and reg lines are empty for picClerk, and haven't
		// visited him, just enter his regular line.

			oMonitor.regPCLineLength++;
			oMonitor.regPCLineCV->Wait(oMonitor.acpcLineLock);
			talkPicClerk(SSN, visitedPic);
		}
		else if (visitedPic) {
		// If already visited picClerk, go into privLine for appClerk

			oMonitor.privACLineLength++;
			oMonitor.privACLineCV->Wait(oMonitor.acpcLineLock);
			talkAppClerk(SSN, visitedApp);

			myCash -= 500;
		}
		else if (visitedApp) {
		// If already visited appClerk, go into privLine for picClerk 

			oMonitor.privPCLineLength++;
			oMonitor.privPCLineCV->Wait(oMonitor.acpcLineLock);
			talkPicClerk(SSN, visitedPic);

			myCash -= 500;
		}
		else if (oMonitor.privACLineLength <= oMonitor.privPCLineLength) {
		// If appClerk's privLine is less than  or equal to 
		// picClerk's privLine, then go into appClerk's privLine

			oMonitor.privACLineLength++;
			oMonitor.privACLineCV->Wait(oMonitor.acpcLineLock);
			talkAppClerk(SSN, visitedApp);

			myCash -= 500;
		}
		else {
		// Else picClerk's privLine is shorter, so go in there
			
			oMonitor.privPCLineLength++;
			oMonitor.privPCLineCV->Wait(oMonitor.acpcLineLock);
			talkPicClerk(SSN, visitedPic);

			myCash -= 500;
		}
	}
	else {
	// Else don't have enough money, just go into regular lines
		
		if (visitedPic) {
		// If already been to picClerk, go into appClerk's line

			oMonitor.regACLineLength++;
			oMonitor.regACLineCV->Wait(oMonitor.acpcLineLock);
			talkAppClerk(SSN, visitedApp);
		}
		else if (visitedApp) {
		// If already visited appClerk, go into picClerk's line

			oMonitor.regPCLineLength++;
			oMonitor.regPCLineCV->Wait(oMonitor.acpcLineLock);
			talkPicClerk(SSN, visitedPic);
		}
		else if (oMonitor.regACLineLength <= oMonitor.regPCLineLength) {
		// If appClerk's regLine is shorter or equal to picClerk's
		// regLine, go into appClerk's line
			
			oMonitor.regACLineLength++;
			oMonitor.regACLineCV->Wait(oMonitor.acpcLineLock);
			talkAppClerk(SSN, visitedApp);
		}
		else {
		// Else picClerk's regLine is shorter than appClerk's, so
		// go there
			
			oMonitor.regPCLineLength++;
			oMonitor.regPCLineCV->Wait(oMonitor.acpcLineLock);
			talkPicClerk(SSN, visitedPic);
		}
	}
}

// Jasper Lee:
// 	Helper function for Customer/AppClerk interaction.
// 	Called by lineAppPicClerk() after choosing which line to enter.
void Office::talkAppClerk(int& SSN, bool& visitedApp) {
	int myClerk = -1;
	for (int i = 0; i < oMonitor.numAppClerks; i++) {
		oMonitor.appLock[i]->Acquire(); // Prevents race condition for clerks
		if (oMonitor.appState[i] != oMonitor.AVAILABLE) {
			oMonitor.appLock[i]->Release(); // Release if not AVAILABLE
		}
		else {
			// Otherwise, set that clerk's state to BUSY and keep index for
			// future use
			myClerk = i;
			oMonitor.appState[i] = oMonitor.BUSY;
			break;				
		}
	}

	oMonitor.acpcLineLock->Release();
	oMonitor.appData[myClerk] = SSN; // Giving appClerk my SSN
	oMonitor.appCV[myClerk]->Signal(oMonitor.appLock[myClerk]);
	oMonitor.appCV[myClerk]->Wait(oMonitor.appLock[myClerk]);
	oMonitor.appLock[myClerk]->Release();
		
	visitedApp = true;
}

// Jasper Lee:
// 	Helper function for Customer/PicClerk interaction.
// 	Called by lineAppPicClerk() after choosing which line to enter.
void Office::talkPicClerk(int& SSN, bool& visitedPic) {
	int myClerk = -1;
	for (int i = 0; i < oMonitor.numPicClerks; i++) {
		oMonitor.picLock[i]->Acquire(); // Prevents race condition for clerks
		if (oMonitor.picState[i] != oMonitor.AVAILABLE) {
			oMonitor.picLock[i]->Release(); // Release if not AVAILABLE
		}
		else {
			// Otherwise, set that clerk's state to BUSY and keep index for
			// future use
			myClerk = i;
			oMonitor.picState[i] = oMonitor.BUSY;
			break;				
		}
	}

	oMonitor.acpcLineLock->Release();
	oMonitor.picData[myClerk] = SSN;
	oMonitor.picCV[myClerk]->Signal(oMonitor.picLock[myClerk]);
	oMonitor.picCV[myClerk]->Wait(oMonitor.picLock[myClerk]); // Ready for picture

	while (true) {
	// Loop for checking if customer hates his picture
		int chanceToHate = rand() % 2; // 50/50 chance to hate his picture
		if (chanceToHate == 0) {
			// If hates his picture
			oMonitor.picDataBool[myClerk] = false;
			oMonitor.picCV[myClerk]->Signal(oMonitor.picLock[myClerk]);
			oMonitor.picCV[myClerk]->Wait(oMonitor.picLock[myClerk]);
		}
		else {
			// Else he decides not to be an ass and just accepts it
			oMonitor.picDataBool[myClerk] = true;
			oMonitor.picCV[myClerk]->Signal(oMonitor.picLock[myClerk]);
			oMonitor.picCV[myClerk]->Wait(oMonitor.picLock[myClerk]);
			break;
		}
	}
	oMonitor.picLock[myClerk]->Release();
		
	visitedPic = true;
}

// Jasper Lee:
// 	Helper function for Customer/Senator to get in line for the
// 	PassportClerk. Takes in a reference to the customer's cash, SSN, and 
// 	visited boolean flags.
void Office::linePassClerk(int& myCash, int& SSN, bool& visitedPass) {
	oMonitor.passLineLock->Acquire();
	if (myCash > 500) {

		if (oMonitor.regPassLineLength == 0 &&	
		    oMonitor.privPassLineLength == 0) { 
		// If both priv and reg lines are empty, just go into the
		// the empty regular line and save money.
			
			oMonitor.regPassLineLength++;
			oMonitor.regPassLineCV->Wait(oMonitor.passLineLock);
			talkPassClerk(SSN, visitedPass);
		}
		else {
		// Else, just go into the rich people line
			
			oMonitor.privPassLineLength++;
			oMonitor.privPassLineCV->Wait(oMonitor.passLineLock);
			talkPassClerk(SSN, visitedPass);
			myCash -= 500;
		}
	}
	else {
		// Doesn't have enough cash for privileged, just go into regular
		oMonitor.regPassLineLength++;
		oMonitor.regPassLineCV->Wait(oMonitor.passLineLock);
		talkPassClerk(SSN, visitedPass);
	}
}

// Jasper Lee:
// 	Helper function for Customer/PassportClerk interaction.
// 	Called by linePassClerk() after checking which line to enter
void Office::talkPassClerk(int& SSN, bool& visitedPass) {
	int myClerk = -1;
	for (int i = 0; i < oMonitor.numPassClerks; i++) {
		oMonitor.passLock[i]->Acquire(); // Prevents race condition for clerks
		if (oMonitor.passState[i] != oMonitor.AVAILABLE) {
			oMonitor.passLock[i]->Release(); // Release if not AVAILABLE
		}
		else {
			// Otherwise, set that clerk's state to BUSY and keep index for
			// future use
			myClerk = i;
			oMonitor.passState[i] = oMonitor.BUSY;
			break;				
		}
	}

	oMonitor.passLineLock->Release();
	oMonitor.passData[myClerk] = SSN;
	oMonitor.passCV[myClerk]->Signal(oMonitor.passLock[myClerk]);
	oMonitor.passCV[myClerk]->Wait(oMonitor.passLock[myClerk]);

	if (oMonitor.passDataBool) {
	// If customer had previous files completed, go on
		oMonitor.passLock[myClerk]->Release();
		visitedPass = true;
	}
	else {
	// Else am an idiot and is forced to wait several seconds before getting
	// into another line.
		oMonitor.passLock[myClerk]->Release();
		int randWait = rand() % 900 + 101; // Random wait time between 100 and 1000
		for (int i = 0; i < randWait; i++) {
			currentThread->Yield();
		}
	}
}

// Jasper Lee:
// 	Helper function for Customer/Senator to get in line for the
// 	Cashier. Takes in a reference to the customer's cash, SSN, and 
// 	visited boolean flags.			
void Office::lineCashier(int& myCash, int& SSN, bool& visitedCash) {
	oMonitor.cashLineLock->Acquire();
	oMonitor.cashLineLength++;
	oMonitor.cashLineCV->Wait(oMonitor.cashLineLock);

	int myClerk = -1;
	for (int i = 0; i < oMonitor.numCashiers; i++) {
		oMonitor.cashLock[i]->Acquire(); // Prevents race condition for clerks
		if (oMonitor.cashState[i] != oMonitor.AVAILABLE) {
			oMonitor.cashLock[i]->Release(); // Release if not AVAILABLE
		}
		else {
			// Otherwise, set that clerk's state to BUSY and keep index for
			// future use
			myClerk = i;
			oMonitor.cashState[i] = oMonitor.BUSY;
			break;				
		}
	}

	oMonitor.cashLineLock->Release();
	oMonitor.cashData[myClerk] = SSN;
	oMonitor.cashCV[myClerk]->Signal(oMonitor.passLock[myClerk]);
	oMonitor.cashCV[myClerk]->Wait(oMonitor.passLock[myClerk]);

	if (oMonitor.cashDataBool) {
	// If customer had previous files completed, go on
		oMonitor.cashLock[myClerk]->Release();
		visitedCash = true;
		myCash -= 100;
	}
	else {
	// Else am an idiot and is forced to wait several seconds before getting
	// into another line.
		oMonitor.cashLock[myClerk]->Release();
		int randWait = rand() % 900 + 101; // Random wait time between 100 and 1000
		for (int i = 0; i < randWait; i++) {
			currentThread->Yield();
		}
	}
}			

// Jasper Lee:
// 	Helper function for Customer that returns the amount of cash he/she has,
// 	chosen between 100, 600, 1100, and 1600.
int Office::doRandomCash() {
	int randomCash = rand() % 4;
	switch(randomCash) {
		case 0:
			return 100;
		case 1:
			return 600;
		case 2:
			return 1100;
		case 3:
			return 1600;
		default:
			printf("This shouldn't happen.");
			return 1600;
	}
}
