#include "officeMonitor.h"

OfficeMonitor::OfficeMonitor(){}
OfficeMonitor::~OfficeMonitor(){}

OfficeMonitor::OfficeMonitor(int numAC, int numPC, 
			     int numPassC, int numCash) {
	numAppClerks = numAC;
	numPicClerks = numPC;
	numPassClerks = numPassC;
	numCashiers = numCash;

	regACLineLength = 0;
	regPCLineLength = 0;
	regPassLineLength = 0;
	privACLineLength = 0;
	privPCLineLength = 0;
	privPassLineLength = 0;
	cashLineLength = 0;

	totalCustSen = 0;
	
	officeCust = 0;
	waitCust = 0;
	officeSenator = 0;
	
	numAppWait = 0;
	numPicWait = 0;
	numPassWait = 0;
	numCashWait = 0;
	
	customerLock = new Lock("customerLock");
	senatorLock = new Lock("senatorLock");
	
	custWaitLock = new Lock("custWaitLock");
	senWaitLock = new Lock("senWaitLock");
	clerkWaitLock = new Lock("clerkWaitLock");
	
	custWaitCV = new Condition("custWaitCV");
	senWaitCV = new Condition("senWaitCV");
	clerkWaitCV = new Condition("clerkWaitCV");
	
	acpcLineLock = new Lock("acpcLineLock");
	passLineLock = new Lock("passLineLock");
	cashLineLock = new Lock("cashLineLock");

	regACLineCV = new Condition("regACLineCV");
	regPCLineCV = new Condition("regPCLineCV");
	regPassLineCV = new Condition("regPassLineCV");
	privACLineCV = new Condition("privACLineCV");
	privPCLineCV = new Condition("privPCLineCV");
	privPassLineCV = new Condition("privPassLineCV");
	cashLineCV = new Condition("cashLineCV");

	if(numAppClerks > 10) {
		numAppClerks = 10;
	}
	for(int i = 0; i < numAppClerks; i++) {
		char* lockName = "appLock" + i;
		char* cvName = "appCV" + i;
		appLock[i] = new Lock(lockName);
		appCV[i] = new Condition(cvName);
		appData[i] = 0;
		appState[i] = BUSY;
	}

	if(numPicClerks > 10) {
		numPicClerks = 10;
	}
	for(int i = 0; i < numPicClerks; i++) {
		char* lockName = "picLock" + i;
		char* cvName = "picCV" + i;
		picLock[i] = new Lock(lockName);
		picCV[i] = new Condition(cvName);
		picData[i] = 0;
		picDataBool[i] = false;
		picState[i] = BUSY;
	}

	if(numPassClerks > 10) {
		numPassClerks = 10;
	}
	for(int i = 0; i < numPassClerks; i++) {
		char* lockName = "passLock" + i;
		char* cvName = "passCV" + i;
		passLock[i] = new Lock(lockName);
		passCV[i] = new Condition(cvName);
		passData[i] = 0;
		passDataBool[i] = false;
		passState[i] = BUSY;
	}

	if(numCashiers > 10) {
		numCashiers = 10;
	}
	for(int i = 0; i < numCashiers; i++) {
		char* lockName = "cashLock" + i;
		char* cvName = "cashCV" + i;
		cashLock[i] = new Lock(lockName);
		cashCV[i] = new Condition(cvName);
		cashData[i] = 0;
		cashDataBool[i] = false;
		cashState[i] = BUSY;
	}

 	appMoney = 0;
	picMoney = 0;
	passMoney = 0;
  	cashMoney = 0;
        
	appMoneyLock = new Lock("appMoneyLock");
	picMoneyLock = new Lock("picMoneyLock");
  	passMoneyLock = new Lock("passMoneyLock");
  	cashMoneyLock = new Lock("cashMoneyLock");

	printf("Number of Customers = %d\n", officeCust);
	printf("Number of Senators = %d\n",officeSenator);
	printf("Number of ApplicationClerks = %d\n",numAppClerks);
	printf("Number of PictureClerks = %d\n",numPicClerks);
	printf("Number of PassportClerks = %d\n",numPassClerks);
	printf("Number of Cashiers = %d\n",numCashiers);

}
