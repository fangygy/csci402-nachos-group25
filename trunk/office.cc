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

	oMonitor.appState[myIndex] = oMonitor.clerkState.BUSY;		// start off AppClerk state being BUSY

	// Endless loop to perform AppClerk actions
	while(true){
		oMonitor.acpcLineLock->Acquire();

		// Check for the priveliged customer line first
		// If there are priveliged customers, do AppClerk tasks, then received $500
		if(oMonitor.privACLineLength > 0){
			oMonitor.privACLineLength--;
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = oMonitor.clerkState.AVAILABLE;
			oMonitor.privACLineCV->Signal(oMonitor.acpcLineLock);		// Signals the next customer in priv line
			oMonitor.acpcLineLock->Release();
			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);	// Waits for the next customer

			mySSN = oMonitor.appData[myIndex];
			if(oMonitor.fileState[mySSN] == oMonitor.custState.NONE){
				oMonitor.fileState[mySSN] == oMonitor.custState.APPDONE;
			}
			else if(oMonitor.fileState[mySSN] == oMonitor.custState.PICDONE){
				oMonitor.fileState[mySSN] == oMonitor.custState.APPPICDONE;
			}

			// tell passport ssn

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
			oMonitor.appState[myIndex] = oMonitor.clerkState.AVAILABLE;
			oMonitor.regACLineCV->Signal(oMonitor.acpcLineLock);
			oMonitor.acpcLineLock->Release();
			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);

			mySSN = appData[myIndex];
			if(oMonitor.fileState[mySSN] == oMonitor.custState.NONE){
				oMonitor.fileState[mySSN] == oMonitor.custState.APPDONE;
			}
			else if(oMonitor.fileState[mySSN] == oMonitor.custState.PICDONE){
				oMonitor.fileState[mySSN] == oMonitor.custState.APPPICDONE;
			}

			// tell passport ssn
			for(int i = 0; i < 20; i++){
				currentThread->Yield();
			}

			oMonitor.appCV[myIndex]->Signal(oMonitor.appLock); // signal customer awake
			oMonitor.appLock[myIndex]->Release();     // release clerk lock
		}
		// If there are neither priveliged or regular customers, go on break
		else{
			oMonitor.appState[myIndex] = oMonitor.clerkState.BREAK;
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
	oMonitor.picState[myIndex] = oMonitor.clerkState.BUSY;	// start off PicClerk state being BUSY
	oMonitor.picLock[myIndex]->Release();

	// Endless loop for PicClerk actions
	while(true){
		oMonitor.acpcLineLock->Acquire();

		// Check for the priveliged customer line first
		// If there are priveliged customers, do PicClerk tasks, then received $500
		if(oMonitor.privACLineLength > 0){
			oMonitor.privPCLineLength--;
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = oMonitor.clerkState.AVAILABLE;	// Sets itself to AVAILABLE
			oMonitor.privPCLineCV->Signal(oMonitor.acpcLineLock);		// Signals the next customer in line
			oMonitor.acpcLineLock->Release();		
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);	// Waits for the next customer

			mySSN = oMonitor.picData[myIndex];
			if(oMonitor.fileState[mySSN] == oMonitor.custState.NONE){
				oMonitor.fileState[mySSN] == oMonitor.custState.PICDONE;
			}
			else if(oMonitor.fileState[mySSN] == oMonitor.custState.APPDONE){
				oMonitor.fileState[mySSN] == oMonitor.custState.APPPICDONE;
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
			oMonitor.picState[myIndex] = oMonitor.clerkState.AVAILABLE;		// sets itself to AVAILABLE
			oMonitor.regPCLineCV->Signal(oMonitor.acpcLineLock);			// Signals the next customer in line
			oMonitor.acpcLineLock->Release();
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);		// Waits for next customer

			mySSN = oMonitor.picData[myIndex];
			if(oMonitor.fileState[mySSN] == oMonitor.custState.NONE){
				oMonitor.fileState[mySSN] == oMonitor.custState.PICDONE;
			}
			else if(oMonitor.fileState[mySSN] == oMonitor.custState.APPDONE){
				oMonitor.fileState[mySSN] == oMonitor.custState.APPPICDONE;
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
		// If there are neither priveliged or regular customers, go on break
		else{
			oMonitor.picState[myIndex] = oMonitor.clerkState.BREAK;
			oMonitor.picLock[myIndex]->Release();     // release clerk lock
			// go on break
		}
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
}