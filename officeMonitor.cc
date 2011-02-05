#include "officeMonitor.h"

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

	totalCust = 0;
	totalSenator = 0;
	officeCust = 0;
	officeSenator = 0;

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
