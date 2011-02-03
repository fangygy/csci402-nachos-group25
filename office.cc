#include office.h

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

   
