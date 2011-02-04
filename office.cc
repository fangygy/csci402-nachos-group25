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

void Office::AppClerk(int index){
	int myIndex = index;
	int mySSN;

	oMonitor.appState[myIndex] = oMonitor.clerkState.BUSY;
	while(true){
		oMonitor.acpcLineLock->Acquire();
		if(oMonitor.privACLineLength > 0){
			oMonitor.privACLineLength--;
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = oMonitor.clerkState.AVAILABLE;
			oMonitor.privACLineCV->Signal(oMonitor.acpcLineLock);
			oMonitor.acpcLineLock->Release();
			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);
			mySSN = oMonitor.appData[myIndex];
			
			// current thread yield stuff
			// tell passport ssn

			oMonitor.appMoneyLock->Acquire();
			oMonitor.appMoney += 500;
			oMonitor.appMoneyLock->Release();

			oMonitor.appCV[myIndex]->Signal(oMonitor.appLock); // signal customer awake
			oMonitor.appLock[myIndex]->Release();     // release clerk lock
		}
		else if(oMonitor.regACLineLength > 0){
			oMonitor.regACLineLength--;
			oMonitor.appLock[myIndex]->Acquire();
			oMonitor.appState[myIndex] = oMonitor.clerkState.AVAILABLE;
			oMonitor.regACLineCV->Signal(oMonitor.acpcLineLock);
			oMonitor.acpcLineLock->Release();
			oMonitor.appCV[myIndex]->Wait(oMonitor.appLock[myIndex]);
			mySSN = appData[myIndex];

			// current thread yield
			// tell passport ssn

			oMonitor.appCV[myIndex]->Signal(oMonitor.appLock); // signal customer awake
			oMonitor.appLock[myIndex]->Release();     // release clerk lock
		}
		else{

			// go on break
		}
	}
}

void Office::PicClerk(int index){
	int myIndex = index;
	int mySSN;
	bool pictureLiked = false;

	oMonitor.picState[myIndex] = oMonitor.clerkState.BUSY;
	while(true){
		oMonitor.acpcLineLock->Acquire();
		if(oMonitor.privACLineLength > 0){
			oMonitor.privPCLineLength--;
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = oMonitor.clerkState.AVAILABLE;
			oMonitor.privPCLineCV->Signal(oMonitor.acpcLineLock);
			oMonitor.acpcLineLock->Release();
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);

			// show picture
			// current thread yield

			// if picture liked, continue
			// else, repeat
			// maybe, while picture is not liked, do until liked

			// signal customer awake
			oMonitor.picMoneyLock->Acquire();
			oMonitor.picMoney += 500;
			oMonitor.picMoneyLock->Release();

			oMonitor.picCV[myIndex]->Signal(oMonitor.picLock); // signal customer awake
			oMonitor.picLock[myIndex]->Release();// release clerk lock
		}
		else if(oMonitor.regPCLineLength > 0){
			oMonitor.regPCLineLength--;
			oMonitor.picLock[myIndex]->Acquire();
			oMonitor.picState[myIndex] = oMonitor.clerkState.AVAILABLE;
			oMonitor.regPCLineCV->Signal(oMonitor.acpcLineLock);
			oMonitor.acpcLineLock->Release();
			oMonitor.picCV[myIndex]->Wait(oMonitor.picLock[myIndex]);

			// show picture
			// current thread yield

			// if picture liked, continue
			// else, repeat
			// maybe, while picture is not liked, do until liked

			// signal customer awake
			oMonitor.picCV[myIndex]->Signal(oMonitor.picLock); // signal customer awake
			oMonitor.picLock[myIndex]->Release();// release clerk lock
		}
		else{
			// go on break
		}
	}
}
   
