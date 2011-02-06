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
	
	customerLock = new Lock("customerLock");
	senatorLock = new Lock("senatorLock");
	
	custWaitLock = new Lock("custWaitLock");
	senWaitLock = new Lock("senWaitLock");
	
	custWaitCV = new Condition("custWaitCV");
	senWaitCV = new Condition("senWaitCV");
	
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
		appLock[i] = new Lock("appLock" + i);
		appCV[i] = new Condition("appCV" + i);
		appData[i] = 0;
		appState[i] = BUSY;
	}

	if(numPicClerks > 10) {
		numPicClerks = 10;
	}
	for(int i = 0; i < numPicClerks; i++) {
		picLock[i] = new Lock("picLock" + i);
		picCV[i] = new Condition("picCV" + i);
		picData[i] = 0;
		picDataBool[i] = false;
		picState[i] = BUSY;
	}

	if(numPassClerks > 10) {
		numPassClerks = 10;
	}
	for(int i = 0; i < numPassClerks; i++) {
		passLock[i] = new Lock("passLock" + i);
		passCV[i] = new Condition("passCV" + i);
		passData[i] = 0;
		passDataBool[i] = false;
		passState[i] = BUSY;
	}

	if(numCashiers > 10) {
		numCashiers = 10;
	}
	for(int i = 0; i < numCashiers; i++) {
		cashLock[i] = new Lock("cashLock" + i);
		cashCV[i] = new Condition("cashCV" + i);
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

	printf("Number of Customers = " + officeCust + "\n");
	printf("Number of Senators = " + officeSen + "\n");
	printf("Number of ApplicationClerks = " + numAppClerks + "\n");
	printf("Number of PictureClerks = " + numPicClerks + "\n");
	printf("Number of PassportClerks = " + numPassClerks + "\n");
	printf("Number of Cashiers = " + numCashiers + "\n");

}

void OfficeMonitor::addCustomer(int numC) {	
	int newTotal = totalCustSen + numC;
	if (newTotal > 100) {
		newTotal = 100;
	}
	for (int i = totalCustSen; i < newTotal; i++) {
		fileLock[i] = new Lock("fileLock" + i);
		fileState[i] = NONE;
	}

	totalCustSen = newTotal;
}

void OfficeMonitor::addSenator(int numS) {
	int newTotal = totalCustSen + numS;
	if (newTotal > 100) {
		newTotal = 100;
	}
	for (int i = totalCustSen; i < newTotal; i++) {
		fileLock[i] = new Lock("fileLock" + i);
		fileState[i] = NONE;
	}

	totalCustSen = newTotal;

}

////////////////////////////Operator= override

OfficeMonitor& OfficeMonitor::operator=(const OfficeMonitor& o) {
    if (this != &o) {  // make sure not same object
        //delete [] _name;                     // Delete old name's memory.
        //_name = new char[strlen(p._name)+1]; // Get new space
        //strcpy(_name, p._name);              // Copy new name
        //_id = p._id;                         // Copy id
		numAppClerks = o.numAppClerks;
		numPicClerks = o.numPicClerks;
		numPassClerks = o.numPassClerks;
		numCashiers = o.numCashiers;

		regACLineLength = o.reACLineLength;
		regPCLineLength = o.rePCLineLength;
		regPassLineLength = o.rePassLineLength;
		privACLineLength = o.privACLineLength;
		privPCLineLength = o.privPCLineLength;
		privPassLineLength = o.privPassLineLength;
		cashLineLength = o.cashLineLength;

		totalCust = o.totalCust;
		totalSenator = o.totalSenator;
		officeCust = o.officeCust;
		officeSenator = o.officeSenator;

		acpcLineLock = o.acpcLineLock;
		passLineLock = o.passLineLock;
		cashLineLock = o.cashLineLock;

		regACLineCV = o.regACLineCV;
		regPCLineCV = o.regPCLineCV;
		regPassLineCV = o.regPassLineCV;
		privACLineCV = o.privACLineCV;
		privPCLineCV = o.privPCLineCV;
		privPassLineCV o.privPassLineCV;
		cashLineCV = o.cashLineCV;

		if(numAppClerks > 10) {
			numAppClerks = 10;
		}

		for(int i = 0; i < numAppClerks; i++) {
			appLock[i] = o.appLock[i];
			appCV[i] = o.appCV[i];
			appData[i] = o.appData;
			appState[i] = BUSY;
		}

		if(numPicClerks > 10) {
			numPicClerks = 10;
		}
		for(int i = 0; i < numPicClerks; i++) {
			picLock[i] = o.picLock[i];
			picCV[i] = o.picCV[i];
			picData[i] = o.picData[i];
			picDataBool[i] = o.picDataBool[i];
			picState[i] = BUSY;
		}
		if(numPassClerks > 10) {
			numPassClerks = 10;
		}
		for(int i = 0; i < numPassClerks; i++) {
			passLock[i] = o.passLock[i];
			passCV[i] = o.passCV[i];
			passData[i] = o.passData[i];
			passDataBool[i] = o.passDataBool[i];
			passState[i] = BUSY;
		}
		if(numCashiers > 10) {
			numCashiers = 10;
		}
		for(int i = 0; i < numCashiers; i++) {
			cashLock[i] = o.cashLock[i];
			cashCV[i] = o.cashCV[i];
			cashData[i] = o.cashData[i];
			cashDataBool[i] = o.cashDataBool[i];
			cashState[i] = BUSY;
		}


 		appMoney = o.appMoney;
		picMoney = o.picMoney;
		passMoney = o.passMoney;
  		cashMoney = o.cashMoney;
        
		appMoneyLock = o.appMoneyLock;
		picMoneyLock = o.picMoneyLock;
  		passMoneyLock = o.passMoneyLock;
  		cashMoneyLock = o.cashMoneyLock;

		return *this;    // Return ref for multiple assignment
}
