#ifndef OFFICE_H
#define OFFICE_H

#include "officeMonitor.h"

class Office {
 private:
  OfficeMonitor* oMonitor;

	// Helper functions for Customer
	void lineAppPicClerk(int& myCash, int& SSN, bool& visitedApp, bool& visitedPic);
	void linePassClerk(int& myCash, int& SSN, bool& visitedPass);
	void lineCashier(int& myCash, int& SSN, bool& visitedCash);

	// Helper functions for Senator
	void senLineAppPicClerk(int& myCash, int& SSN, bool& visitedApp, bool& visitedPic);
	void senLinePassClerk(int& myCash, int& SSN, bool& visitedPass);
	void senLineCashier(int& myCash, int& SSN, bool& visitedCash);

	// Helper functions shared by Customer and Senator
	int doRandomCash();
	void talkAppClerk(int& SSN, bool& visitedApp);
	void talkPicClerk(int& SSN, bool& visitedPic); 
	void talkPassClerk(int& SSN, bool& visitedPass);

	// Helper function for Customers/Senators/WaitRooms implementation
	void checkSenator();

public:
	extern int MAX_CLERKS;
	extern int MAX_CUSTOMERS;
	Office();
	void startOffice(int numCust, int numApp, int numPic,
		int numPass, int numCash);
	void Customer(int index);
	void AppClerk(int index);
	void PicClerk(int index);
	void PassClerk(int index);
	void Cashier(int index);
	void Manager();
	void Senator(int index);

	void addSenator(int numS);
	void addCustomer(int numC);
};

#endif // OFFICE_H
