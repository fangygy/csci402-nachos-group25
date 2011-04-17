/* Customer.c
*
*
*
*
*
*/

#include syscall.h
#define NUM_CLERKS 5
#define NUM_CUSTOMERS 30
#define NUM_SENATORS 5

enum BOOLEAN {
	false,
	true
};

int myIndex;
int indexInit, indexInitLock;
int fileType;

int myMoney;
enum BOOLEAN visitedApp, visitedPic, visitedPass, visitedCash;

int customerLock, senatorLock;
int officeCustomer, waitingCustomer, officeSenator;

int custWaitLock, custWaitCV;
int numAppWait, numPicWait, numPassWait, numCashWait;

int regACLineLength, regPCLineLength, regPassLineLength;
int privACLineLength, privPCLineLength, privPassLineLength;
int cashLineLength;
int acpcLineLock, passLineLock, cashLineLock;
int regACLineCV, regPCLineCV, regPassLineCV;
int privACLineCV, privPCLineCV, privPassLineCV;
int cashLineCV;

int appLock, picLock, passLock, cashLock;
int appCV, picCV, passCV, cashCV;
int appData, picData, passData, cashData;
int picDataBool, passDataBool, cashDataBool;
int appState, picState, passState, cashState;

void RandomMoney() {
	int random = Random(4);
	if (random == 0) {
		myMoney = 100;
	}
	else if (random == 1) {
		myMoney = 600;
	}
	else if (random == 2) {
		myMoney = 1100;
	}
	else if (random == 3) {
		myMoney = 1600;
	}
}

void DoWaitingRoom() {
	
	/* "A customer has entered the waiting room.\n" */
	ServerAcquire(customerLock, 0);
	SetMV(officeCustomer, 0, GetMV(officeCustomer, 0) - 1);
	SetMV(waitingCustomer, 0, GetMV(waitingCustomer, 0) + 1);
	ServerRelease(customerLock, 0);
	
	ServerAcquire(custWaitLock, 0);
	ServerWait(custWaitCV, 0, custWaitLock, 0);
	ServerRelease(custWaitLock, 0);
	/* "A customer has woken up from the waiting room.\n" */
	
	ServerAcquire(customerLock, 0);
	SetMV(officeCustomer, 0, GetMV(officeCustomer, 0) + 1);
	SetMV(waitingCustomer, 0, GetMV(waitingCustomer, 0) - 1);
	ServerRelease(customerLock, 0);
}

void TalkAppClerk(enum BOOLEAN privLine) {
	int myClerk;
	int i;
	
	for (i = 0; i < NUM_CLERKS; i++) {
		ServerAcquire(appLock, i);
		if (GetMV(appState, i) == 0) {
			myClerk = i;
			SetMV(appState, i, 1);
			ServerRelease(appLock, i);
			break;
		}
		else {
			ServerRelease(appLock, i);
		}
	}
	
	ServerAcquire(appLock, myClerk);
	SetMV(appData, myClerk, myIndex);
	if (privLine == true) {
		/* "Willing to pay 500 to move ahead of line.\n" */
	}
	else {
	
	}
	/* Gives application to */
	
	ServerSignal(appCV, myClerk, appLock, myClerk);
	ServerWait(appCV, myClerk, appLock, myClerk);
	ServerRelease(appLock, myClerk);
	
	/* informed */
	
	visitedApp = true;
}

void TalkPicClerk(enum BOOLEAN privLine) {
	int myClerk;
	int i;
	int chanceToHate;
	enum BOOLEAN hatePicture = false;
	
	chanceToHate = Random(10);
	
	if (chanceToHate < 2) {
		hatePicture = true;
	}
	
	for (i = 0; i < NUM_CLERKS; i++) {
		ServerAcquire(picLock, i);
		if (GetMV(picState, i) == 0) {
			myClerk = i;
			SetMV(picState, i, 1);
			ServerRelease(picLock, i);
			break;
		}
		else {
			ServerRelease(picLock, i);
		}
	}
	
	ServerAcquire(picLock, myClerk);
	
	SetMV(picData, myClerk, myIndex);
	if (privLine == true) {
	
	}
	else {
	
	}
	
	ServerSignal(picCV, myClerk, picLock, myClerk);
	ServerWait(picCV, myClerk, picLock, myClerk);
	
	while (GetMV(picDataBool, myClerk) == 0) {
		if (hatePicture == true) {
			SetMV(picDataBool, myClerk, 0);
			/* Doesn't like */
			
			chanceToHate = Random(10);
			if (chanceTohate < 2) {
				hatePicture = true;
			}
			else {
				hatePicture = false;
			}
			
			ServerSignal(picCV, myClerk, picLock, myClerk);
			ServerWait(picCV, myClerk, picLock, myClerk);
		}
		else {
			SetMV(picDataBool, myClerk, 1);
			/* Likes picture */
			
			ServerSignal(picCV, myClerk, picLock, myClerk);
			ServerWait(picCV, myClerk, picLock, myClerk);
			break;
		}
	}
	
	ServerRelease(picLock, myClerk);
	/* Told by */
	
	visitedPic = true;
}

void LineAppPicClerk() {

	ServerAcquire(acpcLineLock, 0);
	if (myMoney > 500) {
		
		if (GetMV(regACLineLength, 0) == 0 && GetMV(privACLineLength, 0) == 0 && visitedApp != true) {
		
			while (visitedApp != true) {
				/* "Entering empty regular application line.\n" */
				SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) + 1);
				ServerWait(regACLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numAppWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in regular app line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining regular app line.\n" */
					
					ServerAcquire(acpcLineLock, 0);
				}
				else {
					SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkAppClerk(false);
					break;
				}
			}
		}
		else if (GetMV(regPCLineLength, 0) == 0 && GetMV(privPCLineLength, 0) == 0 && visitedPic != true) {
		
			while (visitedPic != true) {
				/* "Entering empty regular picture line.\n" */
				SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) + 1);
				ServerWait(regPCLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numPicWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in regular pic line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining regular pic line.\n" */
					
					ServerAcquire(acpcLineLock, 0);
				}
				else {
					SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkPicClerk(false);
					break;
				}
			}
		}
		else if (visitedPic == true) {
			
			while (visitedApp != true) {
				/* "Entering privileged application line.\n" */
				SetMV(privACLineLength, 0, GetMV(privACLineLength, 0) + 1);
				ServerWait(privACLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numAppWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(privACLineLength, 0, GetMV(privACLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in privileged application line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining privileged application line.\n" */
					
					ServerAcquire(acpcLineLock, 0);	
				}
				else {
					SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkAppClerk(true);
					myMoney -= 500;
					break;
				}
			}
		}
		else if (visitedApp == true) {
			
			while (visitedPic != true) {
				/* "Entering privileged picture line.\n" */
				SetMV(privPCLineLength, 0, GetMV(privPCLineLength, 0) + 1);
				ServerWait(privPCLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numPicWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(privPCLineLength, 0, GetMV(privPCLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in privileged picture line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining privileged picture line.\n" */
					
					ServerAcquire(acpcLineLock, 0);	
				}
				else {
					SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkPicClerk(true);
					myMoney -= 500;
					break;
				}
			}
		}
		else if (GetMV(privACLineLength, 0) <= GetMV(privPCLineLength) {
		
			while (visitedApp != true) {
				/* "Entering privileged application line.\n" */
				SetMV(privACLineLength, 0, GetMV(privACLineLength, 0) + 1);
				ServerWait(privACLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numAppWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(privACLineLength, 0, GetMV(privACLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in privileged application line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining privileged application line.\n" */
					
					ServerAcquire(acpcLineLock, 0);
				}
				else {
					SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkAppClerk(true);
					myMoney -= 500;
					break;
				}
			}
		}
		
		else {
		
			while (visitedPic != true) {
				/* "Entering privileged picture line.\n" */
				SetMV(privPCLineLength, 0, GetMV(privPCLineLength, 0) + 1);
				ServerWait(privPCLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numPicWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(privPCLineLength, 0, GetMV(privPCLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in privileged picture line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining privileged picture line.\n" */
					
					ServerAcquire(acpcLineLock, 0);
				}
				else {
					SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkPicClerk(true);
					myMoney -= 500;
					break;
				}
			}
		}
	}
	
	else {
		if (visitedPic == true) {
		
			while (visitedApp != true) {
				/* Entering regular application line.\n" */
				SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) + 1);
				ServerWait(regACLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numAppWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in regular application line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining regular application line.\n" */
					
					ServerAcquire(acpcLineLock, 0);
				}
				else {
					SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkAppClerk(false);
					break;
				}
			}
		}
		else if (visitedApp == true) {
		
			while (visitedPic != true) {
				/* Entering regular picture line.\n" */
				SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) + 1);
				ServerWait(regPCLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numPicWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in regular picture line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining regular picture line.\n" */
					
					ServerAcquire(acpcLineLock, 0);
				}
				else {
					SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkPicClerk(false);
					break;
				}
			}
		}
		else if (GetMV(regACLineLength, 0) <= GetMV(regPCLineLength, 0)) {
			
			while (visitedApp != true) {
				/* "Entering regular application line.\n" */
				SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) + 1);
				ServerWait(regACLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numAppWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in regular application line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining regular application line.\n" */
					
					ServerAcquire(acpcLineLock, 0);
				}
				else {
					SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkAppClerk(false);
					break;
				}
			}
		}
		else {
		
			while (visitedPic != true) {
				/* "Entering regular picture line.\n" */
				SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) + 1);
				ServerWait(regPCLineCV, 0, acpcLineLock, 0);
				ServerRelease(acpcLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numPicWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(acpcLineLock, 0);
					SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) - 1);
					ServerRelease(acpcLineLock, 0);
					
					/* "Was in regular picture line, entering waiting room.\n" */
					DoWaitingRoom();
					/* "Rejoining regular picture line.\n" */
					
					ServerAcquire(acpcLineLock, 0);
				}
				else {
					SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkPicClerk(false);
					break;
				}
			}
		}
	}
}

void TalkPassClerk(enum BOOLEAN privLine) {
	int myClerk;
	int i;
	int randYield;
	
	for (i = 0; i < NUM_CLERKS; i++) {
		ServerAcquire(passLock, i);
		if (GetMV(passState, i) == 0) {
			myClerk = i;
			SetMV(passState, i, 1);
			ServerRelease(passLock, i);
			break;
		}
		else {
			ServerRelease(passLock, i);
		}
	}
	
	ServerAcquire(passLock, myClerk);
	SetMV(passData, myClerk, myIndex);

	/* */
	
	ServerSignal(passCV, myClerk, passLock, myClerk);
	ServerWait(passCV, myClerk, passLock, myClerk);
	
	if (GetMV(passDataBool, myClerk) == true) {
		ServerRelease(passLock, myClerk);
		visitedPass = true;
		/* */
	}
	else {
		ServerRelease(passLock, myClerk);
		/* */
		randYield = Random(900) + 100;
		for (i = 0; i < randYield; i++) {
			Yield();
		}
	}
}

void LinePassClerk() {
	ServerAcquire(passLineLock, 0);
	
	if (myMoney > 500) {
	
		if (GetMV(regPassLineLength, 0) == 0 && GetMV(privPassLineLength, 0) == 0) {
		
			while (visitedPass != true) {
				/* Entering passport line */
				SetMV(regPassLineLength, 0, GetMV(regPassLineLength, 0) + 1);
				ServerWait(regPassLineCV, 0, passLineLock, 0);
				ServerRelease(passLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numPassWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(passLineLock, 0);
					SetMV(regPassLineLength, 0, GetMV(regPassLineLength, 0) - 1);
					ServerRelease(passLineLock, 0);
					
					/* */
					DoWaitingRoom();
					/* */
					
					ServerAcquire(passLineLock, 0);
				}
				else {
					SetMV(numPassWait, 0, GetMV(numPassWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkPassClerk(false);
					break;
				}
			}
		}
		else {
		
			while (visitedPass != true) {
				/* Entering passport line */
				SetMV(privPassLineLength, 0, GetMV(privPassLineLength, 0) + 1);
				ServerWait(privPassLineCV, 0, passLineLock, 0);
				ServerRelease(passLineLock, 0);
				
				ServerAcquire(senatorLock, 0);
				if (GetMV(officeSenator, 0) > 0 && GetMV(numPassWait, 0) == 0) {
					ServerRelease(senatorLock, 0);
					
					ServerAcquire(passLineLock, 0);
					SetMV(privPassLineLength, 0, GetMV(privPassLineLength, 0) - 1);
					ServerRelease(passLineLock, 0);
					
					/* */
					DoWaitingRoom();
					/* */
					
					ServerAcquire(passLineLock, 0);
				}
				else {
					SetMV(numPassWait, 0, GetMV(numPassWait, 0) - 1);
					ServerRelease(senatorLock, 0);
					TalkPassClerk(true);
					
					myMoney -= 500;
					break;
				}
			}
		}
	}
	else {
	
		while (visitedPass != true) {
			/* Entering passport line */
			SetMV(regPassLineLength, 0, GetMV(regPassLineLength, 0) + 1);
			ServerWait(regPassLineCV, 0, passLineLock, 0);
			ServerRelease(passLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			if (GetMV(officeSenator, 0) > 0 && GetMV(numPassWait, 0) == 0) {
				ServerRelease(senatorLock, 0);
				
				ServerAcquire(passLineLock, 0);
				SetMV(regPassLineLength, 0, GetMV(regPassLineLength, 0) - 1);
				ServerRelease(passLineLock, 0);
				
				/* */
				DoWaitingRoom();
				/* */
				
				ServerAcquire(passLineLock, 0);
			}
			else {
				SetMV(numPassWait, 0, GetMV(numPassWait, 0) - 1);
				ServerRelease(senatorLock, 0);
				TalkPassClerk(false);
				break;
			}
		}
	}
}

void LineTalkCashClerk() {
	int myClerk;
	int i;
	int randYield;
	
	while (visitedCash != true) {
	
		ServerAcquire(cashLineLock, 0);
		/* */
		SetMV(cashLineLength, 0, GetMV(cashLineLength, 0) + 1);
		ServerWait(cashLineCV, 0, cashLineLock, 0);
		ServerRelease(cashLineLock, 0);
		
		Acquire(senatorLock, 0);
		if (GetMV(officeSenator, 0) > 0 && GetMV(numCashWait, 0) == 0) {
			ServerRelease(senatorLock, 0);
			
			ServerAcquire(cashLineLock, 0);
			SetMV(cashLineLength, 0, GetMV(cashLineLength, 0) - 1);
			ServerRelease(cashLineLock, 0);
			
			/* */
			DoWaitingRoom();
			/* */
			
			ServerAcquire(cashLineLock, 0);
		}
		else {
			SetMV(numCashWait, 0, GetMV(numCashWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			break;
		}
	}
	
	for (i = 0; i < NUM_CLERKS; i++) {
		ServerAcquire(cashLock, i);
		if (GetMV(cashState, i) == 0) {
			myClerk = i;
			SetMV(cashState, i, 1);
			ServerRelease(cashLock, i);
			break;
		}
		else {
			ServerRelease(cashLock, i);
		}
	}
	
	ServerAcquire(cashLock, myClerk);
	
	SetMV(cashData, myClerk, myIndex);
	ServerSignal(cashCV, myClerk, cashLock, myClerk);
	ServerWait(cashCV, myClerk, cashLock, myClerk);
	
	if (GetMV(cashDataBool, myClerk) == 1) {
		ServerRelease(cashLock, myClerk);
		visitedCash = true;
		
		/* */
		myMoney -= 100;
	}
	else {
		ServerRelease(cashLock, myClerk);
		/* */
		
		randYield = Random(900) + 100;
		for (i = 0; i < randYield; i++) {
			Yield();
		}
	}
}

int main() {
	
	/* Getting a unique index */
	indexInitLock = ServerCreateLock("CustomerIndexLock", sizeof("CustomerIndexLock"), 1);
	indexInit = CreateMV("CustomerIndex", sizeof("CustomerIndex"), 1, 0x9999);
	
	ServerAcquire(indexInitLock, 0);
	myIndex = GetMV(indexInit, 0);
	SetMV(indexInit, 0, myIndex);
	ServerRelease(indexInitLock, 0);
	
	fileType = CreateMV("fileType", sizeof("fileType"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	SetMV(fileType, myIndex, 0);
	
	customerLock = ServerCreateLock("customerLock", sizeof("customerLock"), 1);
	senatorLock = ServerCreateLock("senatorLock", sizeof("senatorLock"), 1);
	officeCustomer = CreateMV("officeCustomer", sizeof("officeCustomer"), 1, 0x9999);
	waitingCustomer = CreateMV("waitingCustomer", sizeof("waitingCustomer"), 1, 0x9999);
	officeSenator = CreateMV("officeSenator", sizeof("officeSenator"), 1, 0x9999);
	
	custWaitLock = ServerCreateLock("custWaitLock", sizeof("custWaitLock"), 1);
	custWaitCV = ServerCreateCV("custWaitCV", sizeof("custWaitCV"), 1);
	numAppWait = CreateMV("numAppWait", sizeof("numAppWait"), 1, 0x9999);
	numPicWait = CreateMV("numPicWait", sizeof("numPicWait"), 1, 0x9999);
	numPassWait = CreateMV("numPassWait", sizeof("numPassWait"), 1, 0x9999);
	numCashWait = CreateMV("numCashWait", sizeof("numCashWait"), 1, 0x9999);
	
	regACLineLength = CreateMV("regACLineLength", sizeof("regACLineLength"), 1, 0x9999);
	regPCLineLength = CreateMV("regPCLineLength", sizeof("regPCLineLength"), 1, 0x9999);
	regPassLineLength = CreateMV("regPassLineLength", sizeof("regPassLineLength"), 1, 0x9999);
	privACLineLength = CreateMV("privACLineLength", sizeof("privACLineLength"), 1, 0x9999);
	privPCLineLength = CreateMV("privPCLineLength", sizeof("privPCLineLength"), 1, 0x9999);
	privPassLineLength = CreateMV("privPassLineLength", sizeof("privPassLineLength"), 1, 0x9999);
	cashLineLength = CreateMV("cashLineLength", sizeof("cashLineLength"), 1, 0x9999);
	acpcLineLock = ServerCreateLock("acpcLineLock", sizeof("acpcLineLock"), 1);
	passLineLock = ServerCreateLock("passLineLock", sizeof("passLineLock"), 1);
	cashLineLock = ServerCreateLock("cashLineLock", sizeof("cashLineLock"), 1);
	regACLineCV = ServerCreateCV("regACLineCV", sizeof("regACLineCV"), 1);
	regPCLineCV = ServerCreateCV("regPCLineCV", sizeof("regPCLineCV"), 1);
	regPassLineCV = ServerCreateCV("regPassLineCV", sizeof("regPassLineCV"), 1);
	privACLineCV = ServerCreateCV("privACLineCV", sizeof("privACLineCV"), 1);
	privPCLineCV = ServerCreateCV("privPCLineCV", sizeof("privPCLineCV"), 1);
	privPassLineCV = ServerCreateCV("privPassLineCV", sizeof("privPassLineCV"), 1);
	cashLineCV = ServerCreateCV("cashLineCV", sizeof("cashLineCV"), 1);
	
	appLock = ServerCreateLock("appLock", sizeof("appLock"), NUM_CLERKS);
	picLock = ServerCreateLock("picLock", sizeof("picLock"), NUM_CLERKS);
	passLock = ServerCreateLock("passLock", sizeof("passLock"), NUM_CLERKS);
	cashLock = ServerCreateLock("cashLock", sizeof("cashLock"), NUM_CLERKS);
	appCV = ServerCreateCV("appCV", sizeof("appCV"), NUM_CLERKS);
	picCV = ServerCreateCV("picCV", sizeof("picCV"), NUM_CLERKS);
	passCV = ServerCreateCV("passCV", sizeof("passCV"), NUM_CLERKS);
	cashCV = ServerCreateCV("cashCV", sizeof("cashCV"), NUM_CLERKS);
	appData = CreateMV("appData", sizeof("appData"), NUM_CLERKS, 0x9999);
	picData = CreateMV("picData", sizeof("picData"), NUM_CLERKS, 0x9999);
	passData = CreateMV("passData", sizeof("passData"), NUM_CLERKS, 0x9999);
	cashData = CreateMV("cashData", sizeof("cashData"), NUM_CLERKS, 0x9999);
	picDataBool = CreateMV("picDataBool", sizeof("picDataBool"), NUM_CLERKS, 0x9999);
	passDataBool = CreateMV("passDataBool", sizeof("passDataBool"), NUM_CLERKS, 0x9999);
	cashDataBool = CreateMV("cashDataBool", sizeof("cashDataBool"), NUM_CLERKS, 0x9999);
	appState = CreateMV("appState", sizeof("appState"), NUM_CLERKS, 0x9999);
	picState = CreateMV("picState", sizeof("picState"), NUM_CLERKS, 0x9999);
	passState = CreateMV("passState", sizeof("passState"), NUM_CLERKS, 0x9999);
	cashState = CreateMV("cashState", sizeof("cashState"), NUM_CLERKS, 0x9999);
	
	RandomMoney();
	visitedApp = false;
	visitedPic = false;
	visitedPass = false;
	visitedCash = false;
	
	
	ServerAcquire(senatorLock, 0);
	if (GetMV(officeSenator, 0) > 0) {
		/* "Entered office, but senator is here. Going to waiting room.\n" */
		ServerRelease(senatorLock, 0);
		
		ServerAcquire(customerLock, 0);
		SetMV(waitingCustomer, 0, GetMV(waitingCustomer, 0) + 1);
		ServerRelease(customerLock, 0);
		
		ServerAcquire(custWaitLock, 0);
		ServerWait(custWaitCV, 0, custWaitLock, 0);
		ServerRelease(custWaitLock, 0);
		/* "Leaving waiting room, entering passport office\n" */
		
		ServerAcquire(customerLock, 0);
		SetMV(waitingCustomer, 0, GetMV(waitingCustomer, 0) - 1);
		ServerRelease(customerLock, 0);
	}
	else {
		ServerRelease(senatorLock, 0);
	}
	
	ServerAcquire(customerLock, 0);
	SetMV(officeCustomer, 0, GetMV(officeCustomer, 0) + 1);
	ServerRelease(customerLock, 0);
	/* "Entering Passport Office\n" */
	
	while (visitedApp != true || visitedPic != true || visitedPass != true || visitedPic != true) {
		
		if (Random(5) == 1) {
			ServerAcquire(senatorLock, 0);
			if (GetMV(officeSenator, 0) > 0) {
				ServerRelease(senatorLock, 0);
				DoWaitingRoom();
			}
			else {
				ServerRelease(senatorLock, 0);
			}
			/* "Going directly to passport clerk like an idiot.\n" */
			LinePassClerk();
		}
		
		while (visitedApp != true || visitedPic != true) {
		
			ServerAcquire(senatorLock, 0);
			if (GetMV(officeSenator, 0) > 0) {
				ServerRelease(senatorLock, 0);
				DoWaitingRoom();
			}
			else {
				ServerRelease(senatorLock, 0);
			}
			LineAppPicClerk();
		}
		
		while (visitedPass != true) {
		
			ServerAcquire(senatorLock, 0);
			if (GetMV(officeSenator, 0) > 0) {
				ServerRelease(senatorLock, 0);
				DoWaitingRoom();
			}
			else {
				ServerRelease(senatorLock, 0);
			}
			LinePassClerk();
		}
		
		while (visitedCash != true) {
			
			ServerAcquire(senatorLock, 0);
			if (GetMV(officeSenator, 0) > 0) {
				ServerRelease(senatorLock, 0);
				DoWaitingRoom();
			}
			else {
				ServerRelease(senatorLock, 0);
			}
			LineTalkCashClerk();
			
		}	
	}
	
	ServerAcquire(customerLock, 0);
	SetMV(officeCustomer, 0, GetMV(officeCustomer, 0) - 1);
	ServerRelease(customerLock, 0);
	/* "Finished with Passport Office, now leaving.\n" */
	
	Exit(0);
}