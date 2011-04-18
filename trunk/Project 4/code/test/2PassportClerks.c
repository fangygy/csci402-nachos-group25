/* 2PassportClerks.c */

#include "syscall.h"

int main() {

	Exec("../test/AppClerk");
	Exec("../test/AppClerk");
	Exec("../test/PicClerk");
	Exec("../test/PicClerk");
	Exec("../test/PassClerk");
	Exec("../test/PassClerk");
	Exec("../test/CashClerk");
	Exec("../test/CashClerk");
	Exec("../test/Manager");
}