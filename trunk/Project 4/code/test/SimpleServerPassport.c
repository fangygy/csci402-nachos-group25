/* 1 of all test */

#include "syscall.h"

int main() {
	Exec("../test/Customer");
	/*Exec("../test/Customer");*/
	/*Exec("../test/Customer");*/
	Exec("../test/Customer");
	Exec("../test/AppClerk");
	Exec("../test/AppClerk");
	Exec("../test/PicClerk");
	Exec("../test/PassClerk");
	Exec("../test/CashClerk");
	Exec("../test/Senator");
	Exec("../test/Manager");

	Exit(0);
}