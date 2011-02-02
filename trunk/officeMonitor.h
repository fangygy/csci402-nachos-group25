#ifndef OFFICEMONITOR_H
#define OFFICEMONITOR_H

#define MAX_CLERKS 10


#include "synch.h"

class officeMonitor() {
 private:
 public:
  officeMonitor(int numAC, int numPC, int numPassC, int numCash);
  int numAppClerks, numPicClerks, numPassClerks, numCashiers;

  // Line Lengths
  int regACLineLength, regPCLineLength, regPassLineLength;
  int privACLineLength, privPCLineLength, privPassLineLength;
  int cashLineLength;

  // Line Locks
  Lock *acpcLineLock[MAX_CLERKS];
  Lock *passLineLock[MAX_CLERKS];
  Lock *cashLineLock[MAX_CLERKS];

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

  // Clerk Data
  int appData[MAX_CLERKS];

  // Clerk States
  enum clerkState {BUSY, AVAILABLE, BREAK};
  clerkState appState[MAX_CLERKS];
  clerkState picState[MAX_CLERKS];
  clerkState passState[MAX_CLERKS];
  clerkState cashState[MAX_CLERKS];
}
