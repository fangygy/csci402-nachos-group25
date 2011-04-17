/* Manager.c
*
*
*
*
*
*/

#include "syscall.h"
#define NUM_CLERKS 5
#define NUM_CUSTOMERS 30
#define NUM_SENATORS 5

enum BOOLEAN {
	false,
	true
};

/* Locks */
int senatorLock, customerLock;
int acpcLineLock, passLineLock, cashLineLock;
int senWaitLock, clerkWaitLock, custWaitLock;
int appLock, picLock, passLock, cashLock;
int appMoneyLock, picMoneyLock, passMoneyLock, cashMoneyLock;

/* CVs */
int regACLineCV, regPCLineCV, regPassLineCV, cashLineCV;
int privACLineCV, privPCLineCV, privPassLineCV;
int senWaitCV, clerkWaitCV, custWaitCV;
int appCV, picCV, passCV, cashCV;

/* MVs */
int officeSenator, officeCustomer, waitingCustomer;
int regACLineLength, regPCLineLength, regPassLineLength;
int privACLineLength, privPCLineLength, privPassLineLength;
int cashLineLength;
int appState, picState, passState, cashState;
int appMoney, picMoney, passMoney, cashMoney;
int shutdown;

int main() {
	int totalMoney;
	int i;
	enum BOOLEAN loop = true;
	
	/* Locks */
	senatorLock = ServerCreateLock("senatorLock", sizeof("senatorLock"), 1);
	customerLock = ServerCreateLock("customerLock", sizeof("customerLock"), 1);
	acpcLineLock = ServerCreateLock("acpcLineLock", sizeof("acpcLineLock"), 1);
	passLineLock = ServerCreateLock("passLineLock", sizeof("passLineLock"), 1);
	cashLineLock = ServerCreateLock("cashLineLock", sizeof("cashLineLock"), 1);
	custWaitLock = ServerCreateLock("custWaitLock", sizeof("custWaitLock"), 1);
	senWaitLock = ServerCreateLock("senWaitLock", sizeof("senWaitLock"), 1);
	clerkWaitLock = ServerCreateLock("clerkWaitLock", sizeof("clerkWaitLock"), 1);
	appLock = ServerCreateLock("appLock", sizeof("appLock"), NUM_CLERKS);
	picLock = ServerCreateLock("picLock", sizeof("picLock"), NUM_CLERKS);
	passLock = ServerCreateLock("passLock", sizeof("passLock"), NUM_CLERKS);
	cashLock = ServerCreateLock("cashLock", sizeof("cashLock"), NUM_CLERKS);
	appMoneyLock = ServerCreateLock("appMoneyLock", sizeof("appMoneyLock"), 1);
	picMoneyLock = ServerCreateLock("picMoneyLock", sizeof("picMoneyLock"), 1);
	passMoneyLock = ServerCreateLock("passMoneyLock", sizeof("passMoneyLock"), 1);
	cashMoneyLock = ServerCreateLock("cashMoneyLock", sizeof("cashMoneyLock"), 1);
	
	/* CVs */
	regACLineCV = ServerCreateCV("regACLineCV", sizeof("regACLineCV"), 1);
	regPCLineCV = ServerCreateCV("regPCLineCV", sizeof("regPCLineCV"), 1);
	regPassLineCV = ServerCreateCV("regPassLineCV", sizeof("regPassLineCV"), 1);
	privACLineCV = ServerCreateCV("privACLineCV", sizeof("privACLineCV"), 1);
	privPCLineCV = ServerCreateCV("privPCLineCV", sizeof("privPCLineCV"), 1);
	privPassLineCV = ServerCreateCV("privPassLineCV", sizeof("privPassLineCV"), 1);
	cashLineCV = ServerCreateCV("cashLineCV", sizeof("cashLineCV"), 1);
	custWaitCV = ServerCreateCV("custWaitCV", sizeof("custWaitCV"), 1);
	senWaitCV = ServerCreateCV("senWaitCV", sizeof("senWaitCV"), 1);
	clerkWaitCV = ServerCreateCV("clerkWaitCV", sizeof("clerkWaitCV"), 1);
	appCV = ServerCreateCV("appCV", sizeof("appCV"), NUM_CLERKS);
	picCV = ServerCreateCV("picCV", sizeof("picCV"), NUM_CLERKS);
	passCV = ServerCreateCV("passCV", sizeof("passCV"), NUM_CLERKS);
	cashCV = ServerCreateCV("cashCV", sizeof("cashCV"), NUM_CLERKS);
	
	/* MVs */
	officeSenator = CreateMV("officeSenator", sizeof("officeSenator"), 1, 0x9999);
	officeCustomer = CreateMV("officeCustomer", sizeof("officeCustomer"), 1, 0x9999);
	waitingCustomer = CreateMV("waitingCustomer", sizeof("waitingCustomer"), 1, 0x9999);
	regACLineLength = CreateMV("regACLineLength", sizeof("regACLineLength"), 1, 0x9999);
	regPCLineLength = CreateMV("regPCLineLength", sizeof("regPCLineLength"), 1, 0x9999);
	regPassLineLength = CreateMV("regPassLineLength", sizeof("regPassLineLength"), 1, 0x9999);
	privACLineLength = CreateMV("privACLineLength", sizeof("privACLineLength"), 1, 0x9999);
	privPCLineLength = CreateMV("privPCLineLength", sizeof("privPCLineLength"), 1, 0x9999);
	privPassLineLength = CreateMV("privPassLineLength", sizeof("privPassLineLength"), 1, 0x9999);
	cashLineLength = CreateMV("cashLineLength", sizeof("cashLineLength"), 1, 0x9999);
	appState = CreateMV("appState", sizeof("appState"), NUM_CLERKS, 0x9999);
	picState = CreateMV("picState", sizeof("picState"), NUM_CLERKS, 0x9999);
	passState = CreateMV("passState", sizeof("passState"), NUM_CLERKS, 0x9999);
	cashState = CreateMV("cashState", sizeof("cashState"), NUM_CLERKS, 0x9999);
	appMoney = CreateMV("appMoney", sizeof("appMoney"), 1, 0x9999);
	picMoney = CreateMV("picMoney", sizeof("picMoney"), 1, 0x9999);
	passMoney = CreateMV("passMoney", sizeof("passMoney"), 1, 0x9999);
	cashMoney = CreateMV("cashMoney", sizeof("cashMoney"), 1, 0x9999);
	shutdown = CreateMV("shutdown", sizeof("shutdown"), 1, 0x9999);
	
	while (loop == true) {
	
		ServerAcquire(senatorLock, 0);
		if (GetMV(officeSenator, 0) > 0) {
			
			ServerRelease(senatorLock, 0);
			ServerAcquire(customerLock, 0);
			
			while (GetMV(officeCustomer, 0) > 0) {
				ServerRelease(customerLock, 0);
				
				ServerAcquire(acpcLineLock, 0);
				ServerBroadcast(regACLineCV, 0, acpcLineLock, 0);
				ServerBroadcast(privACLineCV, 0, acpcLineLock, 0);
				ServerBroadcast(regPCLineCV, 0, acpcLineLock, 0);
				ServerBroadcast(privPCLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(passLineLock, 0);
				ServerBroadcast(regPassLineCV, 0, passLineLock, 0);
				ServerBroadcast(privPassLineCV, 0, passLineLock, 0);
				ServerRelease(passLineLock, 0);
				
				ServerAcquire(cashLineLock, 0);
				ServerBroadcast(cashLineCV, 0, cashLineLock, 0);
				ServerRelease(cashLineLock, 0);
				
				Yield();
				ServerAcquire(customerLock, 0);
				/* */
			}
			ServerRelease(customerLock, 0);
			
			ServerAcquire(senWaitLock, 0);
			ServerBroadcast(senWaitCV, 0, senWaitLock, 0);
			ServerRelease(senWaitLock, 0);
			
			ServerAcquire(clerkWaitLock, 0);
			ServerBroadcast(clerkWaitCV, 0, clerkWaitLock, 0);
			ServerRelease(clerkWaitLock, 0);
		}
		else if (GetMV(officeSenator, 0) == 0 && GetMV(waitingCustomer, 0) > 0) {
		
			ServerRelease(senatorLock, 0);
			ServerAcquire(customerLock, 0);
			while (GetMV(waitingCustomer, 0) > 0) {
				
				ServerRelease(customerLock, 0);
				ServerAcquire(custWaitLock, 0);
				ServerBroadcast(custWaitCV, 0, custWaitLock, 0);
				ServerRelease(custWaitLock, 0);
				Yield();
				ServerAcquire(customerLock, 0);
			}
			ServerRelease(customerLock, 0);
		}
		else {
			ServerRelease(senatorLock, 0);
		}
		
		totalMoney = 0;
		
		ServerAcquire(acpcLineLock, 0);
		if (GetMV(privACLineLength, 0) >= 3 || GetMV(regACLineLength, 0) >= 3) {
			ServerRelease(acpcLineLock, 0);
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(appLock, i);
				if (GetMV(appState, i) == 2) {
					ServerSignal(appCV, i, appLock, i);
					SetMV(appState, i, 1);
					ServerRelease(appLock, i);
					/* */
				}
				else {
					ServerRelease(appLock, i);
				}
			}
		}
		else if (GetMV(privACLineLength, 0) >= 1 || GetMV(regACLineLength, 0) >= 1) {
			ServerRelease(acpcLineLock, 0);
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(appLock, i);
				if (GetMV(appState, i) == 2) {
					ServerSignal(appCV, i, appLock, i);
					SetMV(appState, i, 1);
					ServerRelease(appLock, i);
					/* */
					break;
				}
				else {
					ServerRelease(appLock, i);
				}
			}
		}
		else {
			ServerRelease(acpcLineLock, 0);
		}
		
		ServerAcquire(acpcLineLock, 0);
		if (GetMV(privPCLineLength, 0) >= 3 || GetMV(regPCLineLength, 0) >= 3) {
			ServerRelease(acpcLineLock, 0);
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(picLock, i);
				if (GetMV(picState, i) == 2) {
					ServerSignal(picCV, i, picLock, i);
					SetMV(picState, i, 1);
					ServerRelease(picLock, i);
					/* */
				}
				else {
					ServerRelease(picLock, i);
				}
			}
		}
		else if (GetMV(privPCLineLength, 0) >= 1 || GetMV(regPCLineLength, 0) >= 1) {
			ServerRelease(acpcLineLock, 0);
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(picLock, i);
				if (GetMV(picState, i) == 2) {
					ServerSignal(picCV, i, picLock, i);
					SetMV(picState, i, 1);
					ServerRelease(picLock, i);
					/* */
					break;
				}
				else {
					ServerRelease(picLock, i);
				}
			}
		}
		else {
			ServerRelease(acpcLineLock, 0);
		}
		
		ServerAcquire(passLineLock, 0);
		if (GetMV(privPassLineLength, 0) >= 3 || GetMV(regPassLineLength, 0) >= 3) {
			ServerRelease(passLineLock, 0);
		
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(passLock, i);
				if (GetMV(passState, i) == 2) {
					ServerSignal(passCV, i, passLock, i);
					SetMV(passState, i, 1);
					ServerRelease(passLock, i);
					/* */
				}
				else {
					ServerRelease(passLock, i);
				}
			}
		}
		else if (GetMV(privPassLineLength, 0) >= 1 || GetMV(regPassLineLength, 0) >= 1) {
			ServerRelease(passLineLock, 0);
		
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(passLock, i);
				if (GetMV(passState, i) == 2) {
					ServerSignal(passCV, i, passLock, i);
					SetMV(passState, i, 1);
					ServerRelease(passLock, i);
					/* */
					break;
				}
				else {
					ServerRelease(passLock, i);
				}
			}
		}
		else {
			ServerRelease(passLineLock, 0);
		}
		
		ServerAcquire(cashLineLock, 0);
		if (GetMV(cashLineLength, 0) >= 3) {
			ServerRelease(cashLineLock, 0);
			
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(cashLock, i);
				if (GetMV(cashState, i) == 2) {
					ServerSignal(cashCV, i, cashLock, i);
					SetMV(cashState, i, 1);
					ServerRelease(cashLock, i);
					/* */
				}
				else {
					ServerRelease(cashLock, i);
				}
			}
		}
		else if (GetMV(cashLineLength, 0) >= 1) {
			ServerRelease(cashLineLock, 0);
			
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(cashLock, i);
				if (GetMV(cashState, i) == 2) {
					ServerSignal(cashCV, i, cashLock, i);
					SetMV(cashState, i, 1);
					ServerRelease(cashLock, i);
					/* */
					break;
				}
				else {
					ServerRelease(cashLock, i);
				}
			}
		}
		else {
			ServerRelease(cashLineLock, 0);
		}
		
		ServerAcquire(senatorLock, 0);
		ServerAcquire(customerLock, 0);
		if ((GetMV(officeCustomer, 0) + GetMV(officeSenator, 0) + GetMV(waitingCustomer, 0) ) == 0) {
			ServerRelease(senatorLock, 0);
			ServerRelease(customerLock, 0);
			
			ServerAcquire(appMoneyLock, 0);
			/* */
			/* */
			totalMoney += GetMV(appMoney, 0);
			ServerRelease(appMoneyLock, 0);
			
			ServerAcquire(picMoneyLock, 0);
			/* */
			/* */
			totalMoney += GetMV(picMoney, 0);
			ServerRelease(picMoneyLock, 0);
			
			ServerAcquire(passMoneyLock, 0);
			/* */
			/* */
			totalMoney += GetMV(passMoney, 0);
			ServerRelease(passMoneyLock, 0);
			
			ServerAcquire(cashMoneyLock, 0);
			/* */
			/* */
			totalMoney += GetMV(cashMoney, 0);
			ServerRelease(cashMoneyLock, 0);
			
			/* */
			/* */
			SetMV(shutdown, 0, 1);
			
			/* */
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(appLock, i);
				if (GetMV(appState, i) == 2) {
					ServerSignal(appCV, i, appLock, i);
					SetMV(appState, i, 1);
					ServerRelease(appLock, i);
				}
			}
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(picLock, i);
				if (GetMV(picState, i) == 2) {
					ServerSignal(picCV, i, picLock, i);
					SetMV(picState, i, 1);
					ServerRelease(picLock, i);
				}
			}
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(passLock, i);
				if (GetMV(passState, i) == 2) {
					ServerSignal(passCV, i, passLock, i);
					SetMV(passState, i, 1);
					ServerRelease(passLock, i);
				}
			}
			for (i = 0; i < NUM_CLERKS; i++) {
				ServerAcquire(cashLock, i);
				if (GetMV(cashState, i) == 2) {
					ServerSignal(cashCV, i, cashLock, i);
					SetMV(cashState, i, 1);
					ServerRelease(cashLock, i);
				}
			}
			/* */
			Exit(0);
		}
		ServerRelease(senatorLock, 0);
		ServerRelease(customerLock, 0);
		Yield();
	}
}