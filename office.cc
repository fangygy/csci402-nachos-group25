#include office.h
#include officeMonitor.h

Office::Office() { }

void Office::startOffice(int numCust, int numApp, int numPic,
		         int numPass, int numCash) {
	oMonitor = new OfficeMonitor(numApp, numPic, numPass, numCash);
	Thread *t;

	for(int i = 0; i < numCust; i++) {
		t = new Thread("Cust" + i);
		t->Fork((VoidFunctionPtr) Customer, i);
	} 
	
	for(int i = 0; i < numApp; i++) {
		t = new Thread("AppClerk" + i);
		t->Fork((VoidFunctionPtr) AppClerk, i);
	}

	for(int i = 0; i < numPic; i++) {
		t = new Thread("PicClerk" + i);
		t->Fork((VoidFunctionPtr) PicClerk, i);
	}

	for(int i = 0; i < numPass; i++) {
		t = new Thread("PassClerk" + i);
		t->Fork((VoidFunctionPtr) PassClerk, i);
	}

	for(int i = 0; i < numCash; i++) {
		t = new Thread("Cashier" + i);
		t->Fork((VoidFunctionPtr) Cashier, i);
	}

	t = new Thread("Manager");
	t->Fork((VoidFunctionPtr) Manager, i);  
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
	oMonitor.appState[myIndex] = BUSY;	// start off AppClerk state being BUSY
	oMonitor.appLock[myIndex]->Release();

	// Endless loop to perform AppClerk actions
	while(true){
		oMonitor.acpcLineLock->Acquire();

		// Check for the privileged customer line first
		// If there are privileged customers, do AppClerk tasks, then received $500
		if(oMonitor.privACLineLength > 0){
			oMonitor.privACLineLength--;
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = AVAILABLE;
			oMonitor.privACLineCV->Signal(oMonitor.acpcLineLock);		// Signals the next customer in priv line
			oMonitor.acpcLineLock->Release();
			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);	// Waits for the next customer

			mySSN = oMonitor.appData[myIndex];
			oMonitor.fileLock[mySSN]->Acquire();
			if(oMonitor.fileState[mySSN] == NONE){
				oMonitor.fileState[mySSN] = APPDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else if(oMonitor.fileState[mySSN] == PICDONE){
				oMonitor.fileState[mySSN] = APPPICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else{
				printf("Error. Customer does not have picture application or no application. What are you doing here?");
				oMonitor.fileLock[mySSN]->Release();
			}

			// tell passport ssn

			printf("Now filing application...");
			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}


			oMonitor.appMoneyLock->Acquire();
			oMonitor.appMoney += 500;
			oMonitor.appMoneyLock->Release();

			oMonitor.appCV[myIndex]->Signal(oMonitor.appLock); // signal customer awake
			oMonitor.appLock[myIndex]->Release();     // release clerk lock
		}
		// Check for regular customer line next
		// If there are regular customers, do AppClerk tasks
		else if(oMonitor.regACLineLength > 0){
			oMonitor.regACLineLength--;
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = AVAILABLE;
			oMonitor.regACLineCV->Signal(oMonitor.acpcLineLock);
			oMonitor.acpcLineLock->Release();
			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);

			mySSN = appData[myIndex];
			oMonitor.fileLock[mySSN]->Acquire();
			if(oMonitor.fileState[mySSN] == NONE){
				oMonitor.fileState[mySSN] = .APPDONE;
				oMonitor.fileLock[mySSN]->Release();

			}
			else if(oMonitor.fileState[mySSN] == PICDONE){
				oMonitor.fileState[mySSN] = APPPICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else{
				printf("Error. Customer does not have picture application or no application. What are you doing here?");
				oMonitor.fileLock[mySSN]->Release();
			}

			// tell passport ssn

			printf("Now filing application...");
			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}

			oMonitor.appCV[myIndex]->Signal(oMonitor.appLock); // signal customer awake
			oMonitor.appLock[myIndex]->Release();     // release clerk lock
		}
		// If there are neither privileged or regular customers, go on break
		else{

			oMonitor.acpcLineLock->Release();
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = BREAK;

			oMonitor.appLock[myIndex]->Release();     // release clerk lock

			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);

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
	oMonitor.picState[myIndex] = BUSY;	// start off PicClerk state being BUSY
	oMonitor.picLock[myIndex]->Release();

	// Endless loop for PicClerk actions
	while(true){
		oMonitor.acpcLineLock->Acquire();

		// Check for the privileged customer line first
		// If there are privileged customers, do PicClerk tasks, then received $500
		if(oMonitor.privACLineLength > 0){
			oMonitor.privPCLineLength--;
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = AVAILABLE;	// Sets itself to AVAILABLE
			oMonitor.privPCLineCV->Signal(oMonitor.acpcLineLock);		// Signals the next customer in line
			oMonitor.acpcLineLock->Release();		
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);	// Waits for the next customer

			mySSN = oMonitor.picData[myIndex];
			oMonitor.fileLock[mySSN]->Acquire();
			if(oMonitor.fileState[mySSN] == NONE){
				oMonitor.fileState[mySSN] = PICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else if(oMonitor.fileState[mySSN] == APPDONE){
				oMonitor.fileState[mySSN] = APPPICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else{
				printf("Error. Customer does not have either an application or no application. What are you doing here?");
				oMonitor.fileLock[mySSN]->Release();
			}

			while(oMonitor.picDataBool[myIndex] == false){
				printf("PictureClerk " + myIndex + ": Taking picture.");
				for(int i = 0; i < 4; i++){
					currentThread->Yield();
				}
				oMonitor.picCV[myIndex]->Signal(oMonitor.picLock[myIndex]);	//
				oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);	// Shows the customer the picture
				
				// yield to take picture
				// print statement: "Taking picture"
				// picCV->Signal then picCV->Wait to
				// show customer picture
			}

			// file picture using
			// current thread yield
			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}

			// signal customer awake
			oMonitor.picMoneyLock->Acquire();
			oMonitor.picMoney += 500;
			oMonitor.picMoneyLock->Release();

			oMonitor.picCV[myIndex]->Signal(oMonitor.picLock); // signal customer awake
			oMonitor.picLock[myIndex]->Release();// release clerk lock
		}
		// Check for the regular customer line next
		// If there are regular customers, do PicClerk tasks
		else if(oMonitor.regPCLineLength > 0){
			oMonitor.regPCLineLength--;
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = AVAILABLE;		// sets itself to AVAILABLE
			oMonitor.regPCLineCV->Signal(oMonitor.acpcLineLock);			// Signals the next customer in line
			oMonitor.acpcLineLock->Release();
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);		// Waits for next customer

			mySSN = oMonitor.picData[myIndex];
			oMonitor.fileLock[mySSN]->Acquire();
			if(oMonitor.fileState[mySSN] == NONE){
				oMonitor.fileState[mySSN] = oMonitor.custState.PICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else if(oMonitor.fileState[mySSN] == APPDONE){
				oMonitor.fileState[mySSN] = APPPICDONE;
				oMonitor.fileLock[mySSN]->Release();
			}
			else{
				printf("Error. Customer does not have either an application or no application. What are you doing here?");
				oMonitor.fileLock[mySSN]->Release();
			}

			while(oMonitor.picDataBool[myIndex] == false){
				printf("PictureClerk " + myIndex + ": Taking picture.");
				for(int i = 0; i < 4; i++){
					currentThread->Yield();
				}
				oMonitor.picCV[myIndex]->Signal(oMonitor.picLock[myIndex]);	//
				oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);	// Shows the customer the picture
				
				// yield to take picture
				// print statement: "Taking picture"
				// picCV->Signal then picCV->Wait to
				// show customer picture
			}

			// file picture using
			// current thread yield
			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}

			// signal customer awake
			oMonitor.picCV[myIndex]->Signal(oMonitor.picLock); // signal customer awake
			oMonitor.picLock[myIndex]->Release();// release clerk lock
		}
		// If there are neither privileged or regular customers, go on break
		else{
			oMonitor.acpcLineLock->Release();
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = BREAK;
			oMonitor.picLock[myIndex]->Release();
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);


			oMonitor.picLock[myIndex]->Release();     // release clerk lock
			// go on break
		}
	}
}
   
void Office::Manager(){
	int totalMoney;

	while(true){
		totalMoney = 0;

		// Checks for AppClerk on break
		if(oMonitor.privACLineLength > 3 || oMonitor.regACLineLength > 3){
			for(int i = 0; i < 	oMonitor.numAppClerks; i++){
				if(oMonitor.appState[i] == BREAK){
					oMonitor.appLock[i]->Acquire();
					oMonitor.appCV[i]->Signal(oMonitor.appLock[i]);
					oMonitor.appLock[i]->Release();
				}
			}						
		}
		else if(oMonitor.privACLineLength > 1 || oMonitor.regACLineLength > 1){
			for(int i = 0; i < 	oMonitor.numAppClerks; i++){
				if(oMonitor.appState[i] == BREAK){
					oMonitor.appLock[i]->Acquire();
					oMonitor.appCV[i]->Signal(oMonitor.appLock[i]);
					oMonitor.appLock[i]->Release();
					break;
				}
			}						
		}
		// Checks for PictureClerks on break
		if(oMonitor.privPCLineLength > 3 || oMonitor.regPCLineLength > 3){
			for(int i = 0; i < 	oMonitor.numPicClerks; i++){
				if(oMonitor.picState[i] == BREAK){
					oMonitor.picLock[i]->Acquire();
					oMonitor.picCV[i]->Signal(oMonitor.picLock[i]);
					oMonitor.picLock[i]->Release();
				}
			}						
		}
		else if(oMonitor.privPCLineLength > 1 || oMonitor.regPCLineLength > 1){
			for(int i = 0; i < 	oMonitor.numPicClerks; i++){
				if(oMonitor.picState[i] == BREAK){
					oMonitor.picLock[i]->Acquire();
					oMonitor.picCV[i]->Signal(oMonitor.picLock[i]);
					oMonitor.picLock[i]->Release();
					break;
				}
			}						
		}

		// Checks for PassportClerks on break
		if(oMonitor.privPassLineLength > 3 || oMonitor.regPassLineLength > 3){
			for(int i = 0; i < 	oMonitor.numPassClerks; i++){
				if(oMonitor.passState[i] == BREAK){
					oMonitor.passLock[i]->Acquire();
					oMonitor.passCV[i]->Signal(oMonitor.passLock[i]);
					oMonitor.passLock[i]->Release();
				}
			}						
		}
		else if(oMonitor.privPassLineLength > 1 || oMonitor.regPassLineLength > 1){
			for(int i = 0; i < 	oMonitor.numPassClerks; i++){
				if(oMonitor.passState[i] == BREAK){
					oMonitor.passLock[i]->Acquire();
					oMonitor.passCV[i]->Signal(oMonitor.passLock[i]);
					oMonitor.passLock[i]->Release();
					break;
				}
			}						
		}

		// Checks for Cashier on break
		if(oMonitor.cashLineLength > 3){
			for(int i = 0; i < 	oMonitor.numCashiers; i++){
				if(oMonitor.cashState[i] == BREAK){
					oMonitor.cashLock[i]->Acquire();
					oMonitor.cashCV[i]->Signal(oMonitor.cashLock[i]);
					oMonitor.cashLock[i]->Release();
				}
			}						
		}
		else if(oMonitor.cashLineLength > 1){
			for(int i = 0; i < 	oMonitor.numCashiers; i++){
				if(oMonitor.cashState[i] == BREAK){
					oMonitor.cashLock[i]->Acquire();
					oMonitor.cashCV[i]->Signal(oMonitor.cashLock[i]);
					oMonitor.cashLock[i]->Release();
					break;
				}
			}						
		}

		// Print out money periodically (must figure out Timer stuff)

		oMonitor.appMoneyLock->Acquire();
		printf("Application Clerk's Money: " + oMonitor.appMoney + "\n");
		totalMoney += oMonitor.appMoney;
		oMonitor.appMoneyLock->Release();
		
		oMonitor.picMoneyLock->Acquire();
		printf("Picture Clerk's Money: " + oMonitor.picMoney + "\n");
		totalMoney += oMonitor.picMoney;
		oMonitor.picMoneyLock->Release();

		oMonitor.passMoneyLock->Acquire();
		printf("Passport Clerk's Money: " + oMonitor.passMoney + "\n");
		totalMoney += oMonitor.passMoney;
		oMonitor.passMoneyLock->Release();

		oMonitor.cashMoneyLock->Acquire();
		printf("Cashier Clerk's Money: " + oMonitor.cashMoney + "\n");
		totalMoney += oMonitor.cashMoney;
		oMonitor.cashMoneyLock->Release();

		printf("Total Money: " + totalMoney + "\n");

	}

}


/*
//	Antonio Cade:
//	Passport Clerk function thread
//	Records customer passport
*/

Office::PassClerk(int index) {
	int myIndex = index;
	int myCust;
	bool doPassport = false;

	// set own state to busy
	oMonitor.passLock[myIndex]->Acquire();
	oMonitor.passState[myIndex] = oMonitor.clerkState.BUSY;

	while (true) {
		// Check for customers in line
		oMonitor.passLineLock->Acquire();
		if (oMonitor.privPassLineLength > 0) {
			// Decrement line length, set state to AVAIL, signal 1st customer and wait for them
			oMonitor.privPassLineLength --;
			oMonitor.passState[myIndex] = AVAILABLE;

			oMonitor.privPassLineCV->Signal(oMonitor.passLineLock);
			oMonitor.passLineLock->Release();
			oMonitor.passCV[myIndex]->Wait(oMonitor.passLock[index]);	// wait for customer to signal me
			
			myCust = oMonitor.passData[myIndex];	// customer gave me their SSN/index to check their file
			oMonitor.fileLock[myCust]->Acquire();	// gain access to customer state
			if (oMonitor.fileState[myCust] == APPPICDONE) {
				// customer wasn't a dumbass, DO WORK
				oMonitor.passDataBool[myIndex] = true;
				doPassport = true;
			} else {
				// customer WAS a dumbass.... MAKE THEM PAY
				for (int i = 0; i < 500) {
					currentThread->Yield();
				}
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
				for (int i = 0; i < 500) {
					currentThread->Yield();
				}
				// file dat passport
				oMonitor.fileLock[myCust]->Acquire();
				oMonitor.fileState[myCust] = PASSDONE;
				oMonitor.fileLock[myCust]->Release();
			}
		} else if (regPassLineLength > 0) {
			// Decrement line length, set state to AVAIL, signal 1st customer and wait for them
			oMonitor.regPassLineLength --;
			oMonitor.passState[myIndex] = AVAILABLE;

			oMonitor.regPassLineCV->Signal(oMonitor.passLineLock);
			oMonitor.passLineLock->Release();
			oMonitor.passCV[myIndex]->Wait(oMonitor.passLock[index]);	// wait for customer to signal me
			
			myCust = oMonitor.passData[myIndex];	// customer gave me their SSN/index to check their file
			oMonitor.fileLock[myCust]->Acquire();	// gain access to customer state
			if (oMonitor.fileState[myCust] == APPPICDONE) {
				// customer wasn't a dumbass, DO WORK
				oMonitor.passDataBool[myIndex] = true;
				doPassport = true;
			} else {
				// customer WAS a dumbass.... MAKE THEM PAY
				for (int i = 0; i < 500) {
					currentThread->Yield();
				}
			}
			oMonitor.fileLock[myCust]->Release();

			oMonitor.passCV[myIndex]->Signal(oMonitor.passLock[myIndex]);	// signal customer awake
			oMonitor.passLock[myIndex]->Release();							// release clerk lock

			//if customer wasn't an idiot, process their passport
			if (doPassport) {
				for (int i = 0; i < 500) {
					currentThread->Yield();
				}
				// file dat passport
				oMonitor.fileLock[myCust]->Acquire();
				oMonitor.fileState[myCust] = PASSDONE;
				oMonitor.fileLock[myCust]->Release();
			}
		} else {
			// No one in line... Pull out your DS and take a break
			oMonitor.passLineLock->Release();
			oMonitor.passLock[myIndex]->Acquire();
			oMonitor.passState[myIndex] = BREAK;
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

Office::Cashier(int index) {
	int myIndex = index;
	int myCust;
	bool doCash = false;

	// set own state to busy
	oMonitor.cashLock[myIndex]->Acquire();
	oMonitor.cashState[myIndex] = BUSY;
	//oMonitor.passLock[index]->Release();

	while (true) {
		// Check for customers in line
		oMonitor.cashLineLock->Acquire();
		if (oMonitor.cashLineLength > 0) {
			// Decrement line length, set state to AVAIL, signal 1st customer and wait for them
			oMonitor.cashLineLength --;
			oMonitor.cashState[myIndex] = AVAILABLE;

			oMonitor.cashLineCV->Signal(oMonitor.cashLineLock);		// signal the customer
			oMonitor.cashLineLock->Release();
			oMonitor.cashCV[myIndex]->Wait(oMonitor.cashLock[myIndex]);	// wait for customer to signal me

			myCust = cashData[myIndex];		// customer gave me their SSN
			oMonitor.fileLock[myCust]->Acquire();	// gain access to customer state
			if (oMonitor.fileState[myCust] == PASSDONE) {
				// customer wasn't a dumbass, DO WORK
				oMonitor.passDataBool[myIndex] = true;
				for (int i = 0; i < 50) {
					currentThread->Yield();
				}
				oMonitor.fileState[myCust] == ALLDONE;

				// add $100 to passClerk money amount for passport fee
				oMonitor.cashMoneyLock->Acquire();
				oMonitor.cashMoney += 100;
				oMonitor.cashMoneyLock->Release();
			} else {
				// customer WAS a dumbass.... MAKE THEM PAY
				for (int i = 0; i < 500) {
					currentThread->Yield();
				}
			}
			oMonitor.fileLock[myCust]->Release();

			oMonitor.cashCV[myIndex]->Signal(oMonitor.cashLock[myIndex]);	// signal customer awake
			oMonitor.cashLock[myIndex]->Release();							// release clerk lock
		} else {
			// No one in line... Pull out your DS and take a break
			oMonitor.cashLineLock->Release();
			oMonitor.cashLock[myIndex]->Acquire();
			oMonitor.cashState[myIndex] = BREAK;
			oMonitor.cashCV[myIndex]->Wait(oMonitor.cashLock[myIndex]);

			oMonitor.cashLock[myIndex]->Release();
		}
	}
