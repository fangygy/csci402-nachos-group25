/* 5PassCashClerks.c */

#include "syscall.h"

int main() {

	Exec("../test/PassClerk");
	Exec("../test/PassClerk");
	Exec("../test/PassClerk");
	Exec("../test/PassClerk");
	Exec("../test/PassClerk");
	Exec("../test/CashClerk");
	Exec("../test/CashClerk");
	Exec("../test/CashClerk");
	Exec("../test/CashClerk");
	Exec("../test/CashClerk");
	
	Exit(0);
}