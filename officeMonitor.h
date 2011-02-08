#ifndef OFFICEMONITOR_H
#define OFFICEMONITOR_H

#define MAX_CLERKS 10
#define MAX_CUSTOMERS 100


#include "synch.h"

class OfficeMonitor {
private:
	//No private variables

public:
	static const int MAX_CLERK = MAX_CLERKS;
	static const int MAX_CUSTOMER = MAX_CUSTOMERS;

	OfficeMonitor();
	~OfficeMonitor();
	OfficeMonitor(int numAC, int numPC, int numPassC, int numCash);

	// Amount of each kind of Clerk
	int numAppClerks, numPicClerks, numPassClerks, numCashiers;

	// Amount of each customer/senator in total (used for instantiation)
	int totalCustSen, numAppWait, numPicWait, numPassWait, numCashWait;

	// Amount of each customer/senator currently in office or waiting room
	int officeCust, waitCust, officeSenator;

	// Locks for customer/senator checking
	Lock *customerLock;
	Lock *senatorLock;

	// Waiting room locks/CVs
	Lock *custWaitLock;
	Lock *senWaitLock;
	Lock *clerkWaitLock;
	Condition *clerkWaitCV;
	Condition *custWaitCV;
	Condition *senWaitCV;

	// Line Lengths (Cashier has no privileged line)
	int regACLineLength, regPCLineLength, regPassLineLength;
	int privACLineLength, privPCLineLength, privPassLineLength;
	int cashLineLength;

	// Line Locks
	Lock *acpcLineLock;
	Lock *passLineLock;
	Lock *cashLineLock;

	// Line Condition Variables
	Condition *regACLineCV, *regPCLineCV, *regPassLineCV;
	Condition *privACLineCV, *privPCLineCV, *privPassLineCV;
	Condition *cashLineCV;

	// Clerk Locks
	Lock *appLock[MAX_CLERKS];
	Lock *picLock[MAX_CLERKS];
	Lock *passLock[MAX_CLERKS];
	Lock *cashLock[MAX_CLERKS];

	// Clerk Condition Variables
	Condition *appCV[MAX_CLERKS];
	Condition *picCV[MAX_CLERKS];
	Condition *passCV[MAX_CLERKS];
	Condition *cashCV[MAX_CLERKS];

	// Clerk Data used for passing customer index
	int appData[MAX_CLERKS];
	int picData[MAX_CLERKS];
	int passData[MAX_CLERKS];
	int cashData[MAX_CLERKS];

	// Clerk Data used for passing true/false statements
	bool picDataBool[MAX_CLERKS];
	bool passDataBool[MAX_CLERKS];
	bool cashDataBool[MAX_CLERKS];

	// Clerk States
	enum clerkState {BUSY, AVAILABLE, BREAK};
	clerkState appState[MAX_CLERKS];
	clerkState picState[MAX_CLERKS];
	clerkState passState[MAX_CLERKS];
	clerkState cashState[MAX_CLERKS];

	// Clerk Money
	int appMoney;
	Lock *appMoneyLock;
	int picMoney;
	Lock *picMoneyLock;
	int passMoney;
	Lock *passMoneyLock;
	int cashMoney;
	Lock *cashMoneyLock;

	// Customer States, Types and Lock
	Lock *fileLock[MAX_CUSTOMERS];
	enum custState { NONE, PICDONE, APPDONE, APPPICDONE, PASSDONE, ALLDONE };
	custState fileState[MAX_CUSTOMERS];

	enum custType {CUSTOMER, SENATOR};
	custType fileType[MAX_CUSTOMERS];

	// Wait States
	enum waitState{ WAITING, NOTWAITING };
	waitState custWaitState[MAX_CLERKS];
	waitState appWaitState[MAX_CLERKS];
	waitState picWaitState[MAX_CLERKS];
	waitState passWaitState[MAX_CLERKS];
	waitState cashWaitState[MAX_CLERKS];
};

#endif // OFFICEMONITOR_H
