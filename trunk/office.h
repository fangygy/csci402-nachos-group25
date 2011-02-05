#ifndef OFFICE_H
#define OFFICE_H

#include "officeMonitor.h"

class Office {
 private:
  OfficeMonitor oMonitor;

  // Helper functions for Customer
  int doRandomCash();
  void lineAppPicClerk(int& myCash, int& SSN, bool& visitedApp, bool& visitedPic);
  void talkAppClerk(int& SSN, bool& visitedApp);
  void talkPicClerk(int& SSN, bool& visitedPic);
  void linePassClerk(int& myCash, int& SSN, bool& visitedPass); 
  void talkPassClerk(int& SSN, bool& visitedPass);
  void lineCashier(int& myCash, int& SSN, bool& visitedCash);


 public:
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
};

#endif // OFFICE_H
