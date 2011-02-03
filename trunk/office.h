#ifndef OFFICE_H
#define OFFICE_H

#include "officeMonitor.h"

class Office {
 private:
  OfficeMonitor oMonitor;

  void Customer(int index);
  void AppClerk(int index);
  void PicClerk(int index);
  void PassClerk(int index);
  void Cashier(int index);
  void Manager();
  void Senator(int index);

 public:
  Office();
  void startOffice(int numCust, int numApp, int numPic,
		    int numPass, int numCash);
};

#endif // OFFICE_H
