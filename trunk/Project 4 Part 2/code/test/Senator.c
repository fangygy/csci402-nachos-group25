/* Senator.c
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

int myIndex;
int indexInit, indexInitLock;
int fileType;
int traceLock;

int myMoney;
enum BOOLEAN visitedApp, visitedPic, visitedPass, visitedCash;

int customerLock, senatorLock;
int officeCustomer, waitingCustomer, officeSenator;

int senWaitLock, senWaitCV;
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

void CustTrace(char* custType, int myIndex, char*  clerkType, int myClerk, char* msg) {
	/* Example */
	/* CustTrace("Cust", myIndex, "Pass", myClerk, "Here is mah message to the PassClerk.\n") */
	/* CustTrace("Cust", myIndex, 0x00, 0, "Here is mah message to no one in particular.\n") */
	Trace(custType, myIndex);
	if (clerkType != 0x00) {
		Trace(" -> ", NV);
		Trace(clerkType, myClerk);
	}
	Trace(": ", NV);
	Trace(msg, NV);
}

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
	
	ServerAcquire(traceLock, 0);
	if (privLine == true) {
		/* "Willing to pay 500 to move ahead of line.\n" */
		CustTrace("Sen", myIndex, "App", myClerk, "Willing to pay 500 to move ahead in line.\n");
	}
	CustTrace("Sen", myIndex, "App", myClerk, "Gives application to clerk.\n");
	ServerRelease(traceLock, 0);
	/* Gives application to */
	
	ServerSignal(appCV, myClerk, appLock, myClerk);
	ServerWait(appCV, myClerk, appLock, myClerk);
	ServerRelease(appLock, myClerk);
	
	/* informed */
	ServerAcquire(traceLock, 0);
	CustTrace("Sen", myIndex, "App", myClerk, "Informed that application is filed.\n");
	ServerRelease(traceLock, 0);
	
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
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, "Pic", myClerk, "Willing to pay 500 to move ahead in line.\n");
		ServerRelease(traceLock, 0);
	}
	
	ServerSignal(picCV, myClerk, picLock, myClerk);
	ServerWait(picCV, myClerk, picLock, myClerk);
	
	while (GetMV(picDataBool, myClerk) == 0) {
		if (hatePicture == true) {
			SetMV(picDataBool, myClerk, 0);
			/* Doesn't like */
			
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, "Pic", myClerk, "Hates the picture.\n");
			ServerRelease(traceLock, 0);
			
			chanceToHate = Random(10);
			if (chanceToHate < 2) {
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
			
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, "Pic", myClerk, "Likes the picture.\n");
			ServerRelease(traceLock, 0);
			
			ServerSignal(picCV, myClerk, picLock, myClerk);
			ServerWait(picCV, myClerk, picLock, myClerk);
			break;
		}
	}
	
	ServerRelease(picLock, myClerk);
	/* Told by */
	
	ServerAcquire(traceLock, 0);
	CustTrace("Sen", myIndex, "Pic", myClerk, "Told that procedure is done.\n");
	ServerRelease(traceLock, 0);
	
	visitedPic = true;
}

void LineAppPicClerk() {

	ServerAcquire(acpcLineLock, 0);
	if (myMoney > 500) {
		
		if (GetMV(regACLineLength, 0) == 0 && GetMV(privACLineLength, 0) == 0 && visitedApp != true) {
		
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering empty regular application line.\n");
			ServerRelease(traceLock, 0);
			/* "Entering empty regular application line.\n" */
			SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) + 1);
			ServerWait(regACLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkAppClerk(false);
		}
		else if (GetMV(regPCLineLength, 0) == 0 && GetMV(privPCLineLength, 0) == 0 && visitedPic != true) {
		
			/* "Entering empty regular picture line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering empty regular picture line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) + 1);
			ServerWait(regPCLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkPicClerk(false);
		}
		else if (visitedPic == true) {
		
			/* "Entering privileged application line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering privileged application line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(privACLineLength, 0, GetMV(privACLineLength, 0) + 1);
			ServerWait(privACLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkAppClerk(true);
			myMoney -= 500;
		}
		else if (visitedApp == true) {
			
			/* "Entering privileged picture line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering privileged picture line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(privPCLineLength, 0, GetMV(privPCLineLength, 0) + 1);
			ServerWait(privPCLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkPicClerk(true);
			myMoney -= 500;
		}
		else if (GetMV(privACLineLength, 0) <= GetMV(privPCLineLength, 0)) {
		
			/* "Entering privileged application line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering privileged application line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(privACLineLength, 0, GetMV(privACLineLength, 0) + 1);
			ServerWait(privACLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkAppClerk(true);
			myMoney -= 500;
		}
		
		else {
		
			/* "Entering privileged picture line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering privileged picture line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(privPCLineLength, 0, GetMV(privPCLineLength, 0) + 1);
			ServerWait(privPCLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkPicClerk(true);
			myMoney -= 500;
		}
	}
	
	else {
		if (visitedPic == true) {
		
			/* Entering regular application line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering reuglar application line.\n");
			ServerRelease(traceLock, 0);
			
			SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) + 1);
			ServerWait(regACLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkAppClerk(false);
		}
		else if (visitedApp == true) {
		
			/* Entering regular picture line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering regular picture line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) + 1);
			ServerWait(regPCLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkPicClerk(false);
		}
		else if (GetMV(regACLineLength, 0) <= GetMV(regPCLineLength, 0)) {
			
			/* "Entering regular application line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering regular application line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(regACLineLength, 0, GetMV(regACLineLength, 0) + 1);
			ServerWait(regACLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numAppWait, 0, GetMV(numAppWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkAppClerk(false);
		}
		else {
		
			/* "Entering regular picture line.\n" */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering regular picture line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(regPCLineLength, 0, GetMV(regPCLineLength, 0) + 1);
			ServerWait(regPCLineCV, 0, acpcLineLock, 0);
			ServerRelease(acpcLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numPicWait, 0, GetMV(numPicWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkPicClerk(false);
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
	if (privLine == true) {
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, "Pass", myClerk, "Willing to pay 500 to move ahead in line.\n");
		ServerRelease(traceLock, 0);
	}
	
	ServerSignal(passCV, myClerk, passLock, myClerk);
	ServerWait(passCV, myClerk, passLock, myClerk);
	
	if (GetMV(passDataBool, myClerk) == true) {
		ServerRelease(passLock, myClerk);
		visitedPass = true;
		/* */
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, "Pass", myClerk, "Certified by PassportClerk.\n");
		ServerRelease(traceLock, 0);
	}
	else {
		ServerRelease(passLock, myClerk);
		/* */
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, "Pass", myClerk, "Not certified by PassportClerk.\n");	
		ServerRelease(traceLock, 0);
		
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
			
			/* Entering passport line */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering empty regular passport line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(regPassLineLength, 0, GetMV(regPassLineLength, 0) + 1);
			ServerWait(regPassLineCV, 0, passLineLock, 0);
			ServerRelease(passLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numPassWait, 0, GetMV(numPassWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkPassClerk(false);
			
		}
		else {
		
			/* Entering passport line */
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Entering privileged passport line.\n");
			ServerRelease(traceLock, 0);
				
			SetMV(privPassLineLength, 0, GetMV(privPassLineLength, 0) + 1);
			ServerWait(privPassLineCV, 0, passLineLock, 0);
			ServerRelease(passLineLock, 0);
			
			ServerAcquire(senatorLock, 0);
			SetMV(numPassWait, 0, GetMV(numPassWait, 0) - 1);
			ServerRelease(senatorLock, 0);
			TalkPassClerk(true);
			
			myMoney -= 500;
		}
	}
	else {
	
		/* Entering passport line */
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, 0x00, 0, "Entering regular passport line.\n");
		ServerRelease(traceLock, 0);
			
		SetMV(regPassLineLength, 0, GetMV(regPassLineLength, 0) + 1);
		ServerWait(regPassLineCV, 0, passLineLock, 0);
		ServerRelease(passLineLock, 0);
		
		ServerAcquire(senatorLock, 0);
		SetMV(numPassWait, 0, GetMV(numPassWait, 0) - 1);
		ServerRelease(senatorLock, 0);
		TalkPassClerk(false);
	}
}

void LineTalkCashClerk() {
	int myClerk;
	int i;
	int randYield;
	
	ServerAcquire(cashLineLock, 0);
	/* */
	ServerAcquire(traceLock, 0);
	CustTrace("Sen", myIndex, 0x00, 0, "Entering Cashier line.\n");
	ServerRelease(traceLock, 0);
		
	SetMV(cashLineLength, 0, GetMV(cashLineLength, 0) + 1);
	ServerWait(cashLineCV, 0, cashLineLock, 0);
	ServerRelease(cashLineLock, 0);
	
	ServerAcquire(senatorLock, 0);
	SetMV(numCashWait, 0, GetMV(numCashWait, 0) - 1);
	ServerRelease(senatorLock, 0);
	
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
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, "Cash", myClerk, "Got valid certification.\n");
		CustTrace("Sen", myIndex, "Cash", myClerk, "Pays $100 for passport.\n");
		CustTrace("Sen", myIndex, "Cash", myClerk, "Passport is now recorded by Cashier.\n");
		ServerRelease(traceLock, 0);
		
		myMoney -= 100;
	}
	else {
		ServerRelease(cashLock, myClerk);
		/* */
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, "Cash", myClerk, "Not certified by Cashier.\n");
		CustTrace("Sen", myIndex, "Cash", myClerk, "Punished to wait.\n");
		ServerRelease(traceLock, 0);
		
		randYield = Random(900) + 100;
		for (i = 0; i < randYield; i++) {
			Yield();
		}
	}
}

int main() {
	
	/* Getting a unique index */
	indexInitLock = ServerCreateLock("CuInLk", sizeof("CuInLk"), 1);
	indexInit = CreateMV("CuIn", sizeof("CuIn"), 1, 0x9999);
	
	ServerAcquire(indexInitLock, 0);
	myIndex = GetMV(indexInit, 0);
	SetMV(indexInit, 0, myIndex + 1);
	ServerRelease(indexInitLock, 0);
	
	fileType = CreateMV("FiTp", sizeof("FiTp"), NUM_CUSTOMERS + NUM_SENATORS, 0x9999);
	SetMV(fileType, myIndex, 1);
	
	customerLock = ServerCreateLock("CuLk", sizeof("CuLk"), 1);
	senatorLock = ServerCreateLock("SeLk", sizeof("SeLk"), 1);
	officeCustomer = CreateMV("OfCu", sizeof("OfCu"), 1, 0x9999);
	waitingCustomer = CreateMV("WaCu", sizeof("WaCu"), 1, 0x9999);
	officeSenator = CreateMV("OfSe", sizeof("OfSe"), 1, 0x9999);
	
	senWaitLock = ServerCreateLock("SeWaLk", sizeof("SeWaLk"), 1);
	senWaitCV = ServerCreateCV("SeWaCV", sizeof("SeWaCV"), 1);
	numAppWait = CreateMV("NuApWa", sizeof("NuApWa"), 1, 0x9999);
	numPicWait = CreateMV("NuPiWa", sizeof("NuPiWa"), 1, 0x9999);
	numPassWait = CreateMV("NuPaWa", sizeof("NuPaWa"), 1, 0x9999);
	numCashWait = CreateMV("NuCaWa", sizeof("NuCaWa"), 1, 0x9999);
	
	regACLineLength = CreateMV("RgApLn", sizeof("RgApLn"), 1, 0x9999);
	regPCLineLength = CreateMV("RgPiLn", sizeof("RgPiLn"), 1, 0x9999);
	regPassLineLength = CreateMV("RgPaLn", sizeof("RgPaLn"), 1, 0x9999);
	privACLineLength = CreateMV("PrApLn", sizeof("PrApLn"), 1, 0x9999);
	privPCLineLength = CreateMV("PrPiLn", sizeof("PrPiLn"), 1, 0x9999);
	privPassLineLength = CreateMV("PrPaLn", sizeof("PrPaLn"), 1, 0x9999);
	cashLineLength = CreateMV("CaLn", sizeof("CaLn"), 1, 0x9999);
	acpcLineLock = ServerCreateLock("ACPCLk", sizeof("ACPCLk"), 1);
	passLineLock = ServerCreateLock("PaLiLk", sizeof("PaLiLk"), 1);
	cashLineLock = ServerCreateLock("CaLiLk", sizeof("CaLiLk"), 1);
	regACLineCV = ServerCreateCV("RgApCV", sizeof("RgApCV"), 1);
	regPCLineCV = ServerCreateCV("RgPiCV", sizeof("RgPiCV"), 1);
	regPassLineCV = ServerCreateCV("RgPaCV", sizeof("RgPaCV"), 1);
	privACLineCV = ServerCreateCV("PrApCV", sizeof("PrApCV"), 1);
	privPCLineCV = ServerCreateCV("PrPiCV", sizeof("PrPiCV"), 1);
	privPassLineCV = ServerCreateCV("PrPaCV", sizeof("PrPaCV"), 1);
	cashLineCV = ServerCreateCV("CaLiCV", sizeof("CaLiCV"), 1);
	
	appLock = ServerCreateLock("ApLk", sizeof("ApLk"), NUM_CLERKS);
	picLock = ServerCreateLock("PiLk", sizeof("PiLk"), NUM_CLERKS);
	passLock = ServerCreateLock("PaLk", sizeof("PaLk"), NUM_CLERKS);
	cashLock = ServerCreateLock("CaLk", sizeof("CaLk"), NUM_CLERKS);
	appCV = ServerCreateCV("ApCV", sizeof("ApCV"), NUM_CLERKS);
	picCV = ServerCreateCV("PiCV", sizeof("PiCV"), NUM_CLERKS);
	passCV = ServerCreateCV("PaCV", sizeof("PaCV"), NUM_CLERKS);
	cashCV = ServerCreateCV("CaCV", sizeof("CaCV"), NUM_CLERKS);
	appData = CreateMV("ApDa", sizeof("ApDa"), NUM_CLERKS, 0x9999);
	picData = CreateMV("PiDa", sizeof("PiDa"), NUM_CLERKS, 0x9999);
	passData = CreateMV("PaDa", sizeof("PaDa"), NUM_CLERKS, 0x9999);
	cashData = CreateMV("CaDa", sizeof("CaDa"), NUM_CLERKS, 0x9999);
	picDataBool = CreateMV("PiDaBo", sizeof("PiDaBo"), NUM_CLERKS, 0x9999);
	passDataBool = CreateMV("PaDaBo", sizeof("PaDaBo"), NUM_CLERKS, 0x9999);
	cashDataBool = CreateMV("CaDaBo", sizeof("CaDaBo"), NUM_CLERKS, 0x9999);
	appState = CreateMV("ApSt", sizeof("ApSt"), NUM_CLERKS, 0x9999);
	picState = CreateMV("PiSt", sizeof("PiSt"), NUM_CLERKS, 0x9999);
	passState = CreateMV("PaSt", sizeof("PaSt"), NUM_CLERKS, 0x9999);
	cashState = CreateMV("CaSt", sizeof("CaSt"), NUM_CLERKS, 0x9999);
	
	RandomMoney();
	visitedApp = false;
	visitedPic = false;
	visitedPass = false;
	visitedCash = false;
	
	
	ServerAcquire(senatorLock, 0);
	/* Senator entering passport office */
	SetMV(officeSenator, 0, GetMV(officeSenator, 0) + 1);
	ServerRelease(senatorLock, 0);
	
	ServerAcquire(traceLock, 0);
	CustTrace("Sen", myIndex, 0x00, 0, "Entering Passport Office.\n");
	ServerRelease(traceLock, 0);
	
	ServerAcquire(customerLock, 0);
	if (GetMV(officeCustomer, 0) > 0) {
		ServerRelease(customerLock, 0);
		
		ServerAcquire(senWaitLock, 0);
		/* Waiting for customers to leave */
		
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, 0x00, 0, "Waiting for customers to leave.\n");
		ServerRelease(traceLock, 0);
		
		ServerWait(senWaitCV, 0, senWaitLock, 0);
		/* Has stopped waiting */
		ServerAcquire(traceLock, 0);
		CustTrace("Sen", myIndex, 0x00, 0, "Has stopped waiting\n");
		ServerRelease(traceLock, 0);
		
		ServerRelease(senWaitLock, 0);
	}
	else {
		ServerRelease(customerLock, 0);
	}
	/* "Entering Passport Office\n" */
	ServerAcquire(traceLock, 0);
	CustTrace("Sen", myIndex, 0x00, 0, "Has entered.\n");
	ServerRelease(traceLock, 0);
	
	while (visitedApp != true || visitedPic != true || visitedPass != true || visitedCash != true) {
		
		if (Random(5) == 1) {
			ServerAcquire(traceLock, 0);
			CustTrace("Sen", myIndex, 0x00, 0, "Going directly to passport clerk like an idiot.\n");
			ServerRelease(traceLock, 0);
			/* "Going directly to passport clerk like an idiot.\n" */
			LinePassClerk();
		}
		
		while (visitedApp != true || visitedPic != true) {
		
			LineAppPicClerk();
		}
		
		while (visitedPass != true) {
		
			LinePassClerk();
		}
		
		while (visitedCash != true) {
			
			LineTalkCashClerk();
		}	
	}
	
	ServerAcquire(senatorLock, 0);
	SetMV(officeSenator, 0, GetMV(officeSenator, 0) - 1);
	ServerRelease(senatorLock, 0);
	/* "Finished with Passport Office, now leaving.\n" */
	ServerAcquire(traceLock, 0);
	CustTrace("Sen", myIndex, 0x00, 0, "Finished with Passport Office, leaving.\n");
	ServerRelease(traceLock, 0);
	
	/*Halt();*/
	Exit(0);
}