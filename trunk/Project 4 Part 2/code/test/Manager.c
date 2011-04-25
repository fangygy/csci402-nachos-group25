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
#define NV 0x9999

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
int traceLock;

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

void ClerkTrace(char* clerkType, int myIndex, char* custType, int myCust, char* msg) {
	/* Example */
	/* ClerkTrace("App", myIndex, "Sen", myClerk, "Here is mah message to a Senator.\n") */
	/* ClerkTrace("App", myIndex, 0x00, 0, "Here is mah message to no one in particular.\n") */

	Trace(clerkType, myIndex);
	if (custType != 0x00) {
		Trace(" -> ", NV);
		Trace(custType, myCust);
	}
	Trace(": ", NV);
	Trace(msg, NV);
}

int main() {
	int totalMoney;
	int i;
	enum BOOLEAN loop = true;
	
	/* Locks */
	senatorLock = ServerCreateLock("SeLk", sizeof("SeLk"), 1);
	customerLock = ServerCreateLock("CuLk", sizeof("CuLk"), 1);
	acpcLineLock = ServerCreateLock("ACPCLk", sizeof("ACPCLk"), 1);
	passLineLock = ServerCreateLock("PaLiLk", sizeof("PaLiLk"), 1);
	cashLineLock = ServerCreateLock("CaLiLk", sizeof("CaLiLk"), 1);
	custWaitLock = ServerCreateLock("CuWaLk", sizeof("CuWaLk"), 1);
	senWaitLock = ServerCreateLock("SeWaLk", sizeof("SeWaLk"), 1);
	clerkWaitLock = ServerCreateLock("ClWaLk", sizeof("ClWaLk"), 1);
	appLock = ServerCreateLock("ApLk", sizeof("ApLk"), NUM_CLERKS);
	picLock = ServerCreateLock("PiLk", sizeof("PiLk"), NUM_CLERKS);
	passLock = ServerCreateLock("PaLk", sizeof("PaLk"), NUM_CLERKS);
	cashLock = ServerCreateLock("CaLk", sizeof("CaLk"), NUM_CLERKS);
	appMoneyLock = ServerCreateLock("ApMnLk", sizeof("ApMnLk"), 1);
	picMoneyLock = ServerCreateLock("PiMnLk", sizeof("PiMnLk"), 1);
	passMoneyLock = ServerCreateLock("PaMnLk", sizeof("PaMnLk"), 1);
	cashMoneyLock = ServerCreateLock("CaMnLk", sizeof("CaMnLk"), 1);
	traceLock = ServerCreateLock("trace", sizeof("trace"), 1);
	
	/* CVs */
	regACLineCV = ServerCreateCV("RgApCV", sizeof("RgApCV"), 1);
	regPCLineCV = ServerCreateCV("RgPiCV", sizeof("RgPiCV"), 1);
	regPassLineCV = ServerCreateCV("RgPaCV", sizeof("RgPaCV"), 1);
	privACLineCV = ServerCreateCV("PrApCV", sizeof("PrApCV"), 1);
	privPCLineCV = ServerCreateCV("PrPiCV", sizeof("PrPiCV"), 1);
	privPassLineCV = ServerCreateCV("PrPaCV", sizeof("PrPaCV"), 1);
	cashLineCV = ServerCreateCV("CaLiCV", sizeof("CaLiCV"), 1);
	custWaitCV = ServerCreateCV("CuWaCV", sizeof("CuWaCV"), 1);
	senWaitCV = ServerCreateCV("SeWaCV", sizeof("SeWaCV"), 1);
	clerkWaitCV = ServerCreateCV("ClWaCV", sizeof("ClWaCV"), 1);
	appCV = ServerCreateCV("ApCV", sizeof("ApCV"), NUM_CLERKS);
	picCV = ServerCreateCV("PiCV", sizeof("PiCV"), NUM_CLERKS);
	passCV = ServerCreateCV("PaCV", sizeof("PaCV"), NUM_CLERKS);
	cashCV = ServerCreateCV("CaCV", sizeof("CaCV"), NUM_CLERKS);
	
	/* MVs */
	officeSenator = CreateMV("OfSe", sizeof("OfSe"), 1, 0x9999);
	officeCustomer = CreateMV("OfCu", sizeof("OfCu"), 1, 0x9999);
	waitingCustomer = CreateMV("WaCu", sizeof("WaCu"), 1, 0x9999);
	regACLineLength = CreateMV("RgApLn", sizeof("RgApLn"), 1, 0x9999);
	regPCLineLength = CreateMV("RgPiLn", sizeof("RgPiLn"), 1, 0x9999);
	regPassLineLength = CreateMV("RgPaLn", sizeof("RgPaLn"), 1, 0x9999);
	privACLineLength = CreateMV("PrApLn", sizeof("PrApLn"), 1, 0x9999);
	privPCLineLength = CreateMV("PrPiLn", sizeof("PrPiLn"), 1, 0x9999);
	privPassLineLength = CreateMV("PrPaLn", sizeof("PrPaLn"), 1, 0x9999);
	cashLineLength = CreateMV("CaLn", sizeof("CaLn"), 1, 0x9999);
	appState = CreateMV("ApSt", sizeof("ApSt"), NUM_CLERKS, 0x9999);
	picState = CreateMV("PiSt", sizeof("PiSt"), NUM_CLERKS, 0x9999);
	passState = CreateMV("PaSt", sizeof("PaSt"), NUM_CLERKS, 0x9999);
	cashState = CreateMV("CaSt", sizeof("CaSt"), NUM_CLERKS, 0x9999);
	appMoney = CreateMV("ApMn", sizeof("ApMn"), 1, 0x9999);
	picMoney = CreateMV("PiMn", sizeof("PiMn"), 1, 0x9999);
	passMoney = CreateMV("PaMn", sizeof("PaMn"), 1, 0x9999);
	cashMoney = CreateMV("CaMn", sizeof("CaMn"), 1, 0x9999);
	shutdown = CreateMV("shut", sizeof("shut"), 1, 0x9999);
	
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
				ServerAcquire(traceLock, 0);
				ClerkTrace("Mgr", 0, 0x00, 0, "Waking up customers loop.\n");
				ServerRelease(traceLock, 0);
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
					ServerAcquire(traceLock, 0);
					ClerkTrace("Mgr", 0, 0x00, 0, "Calling an AppClerk back from break.\n");
					ServerRelease(traceLock, 0);
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
					ServerAcquire(traceLock, 0);
					ClerkTrace("Mgr", 0, 0x00, 0, "Calling an AppClerk back from break.\n");
					ServerRelease(traceLock, 0);
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
					ServerAcquire(traceLock, 0);
					ClerkTrace("Mgr", 0, 0x00, 0, "Calling an PicClerk back from break.\n");
					ServerRelease(traceLock, 0);
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
					ServerAcquire(traceLock, 0);
					ClerkTrace("Mgr", 0, 0x00, 0, "Calling an PicClerk back from break.\n");
					ServerRelease(traceLock, 0);
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
					ServerAcquire(traceLock, 0);
					ClerkTrace("Mgr", 0, 0x00, 0, "Calling an PassClerk back from break.\n");
					ServerRelease(traceLock, 0);
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
					ServerAcquire(traceLock, 0);
					ClerkTrace("Mgr", 0, 0x00, 0, "Calling an PassClerk back from break.\n");
					ServerRelease(traceLock, 0);
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
					ServerAcquire(traceLock, 0);
					ClerkTrace("Mgr", 0, 0x00, 0, "Calling an CashClerk back from break.\n");
					ServerRelease(traceLock, 0);
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
					ServerAcquire(traceLock, 0);
					ClerkTrace("Mgr", 0, 0x00, 0, "Calling an CashClerk back from break.\n");
					ServerRelease(traceLock, 0);
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
			
			ServerAcquire(traceLock, 0);
			Write("\n============================================================\n", sizeof("\n============================================================\n"), ConsoleOutput);
			ServerAcquire(appMoneyLock, 0);
			/* */
			/* */
			Trace("Total money received from ApplicationClerk = ", GetMV(appMoney, 0));
			Write("\n", sizeof("\n"), ConsoleOutput);
			totalMoney += GetMV(appMoney, 0);
			ServerRelease(appMoneyLock, 0);
			
			ServerAcquire(picMoneyLock, 0);
			/* */
			/* */
			Trace("Total money received from PictureClerk = ", GetMV(picMoney, 0));
			Write("\n", sizeof("\n"), ConsoleOutput);
			totalMoney += GetMV(picMoney, 0);
			ServerRelease(picMoneyLock, 0);
			
			ServerAcquire(passMoneyLock, 0);
			/* */
			/* */
			Trace("Total money received from PassportClerk = ", GetMV(passMoney, 0));
			Write("\n", sizeof("\n"), ConsoleOutput);
			totalMoney += GetMV(passMoney, 0);
			ServerRelease(passMoneyLock, 0);
			
			ServerAcquire(cashMoneyLock, 0);
			/* */
			/* */
			Trace("Total money received from Cashier = ", GetMV(cashMoney, 0));
			Write("\n", sizeof("\n"), ConsoleOutput);
			totalMoney += GetMV(cashMoney, 0);
			ServerRelease(cashMoneyLock, 0);
			
			/* */
			/* */
			Trace("Total money made by office = ", totalMoney);
			Write("\n", sizeof("\n"), ConsoleOutput);
			Write("\n============================================================\n", sizeof("\n============================================================\n"), ConsoleOutput);
			
			Write("No more customers in passport office, ending simulation.\n", sizeof("No more customers in passport office, ending simulation.\n"), ConsoleOutput);
			Write("\n", sizeof("\n"), ConsoleOutput);
			ServerRelease(traceLock, 0);
			
			SetMV(shutdown, 0, 1);
			
			/* */
			ServerAcquire(traceLock, 0);
			ClerkTrace("Mgr", 0, 0x00, 0, "Notifying everyone that Passport Office is closing.\n");
			ServerRelease(traceLock, 0);
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
			ServerAcquire(traceLock, 0);
			ClerkTrace("Mgr", 0, 0x00, 0, "Notified all clerks. Shutting down.\n");
			ServerRelease(traceLock, 0);
			Exit(0);
		}
		ServerRelease(senatorLock, 0);
		ServerRelease(customerLock, 0);
		Yield();
	}
}