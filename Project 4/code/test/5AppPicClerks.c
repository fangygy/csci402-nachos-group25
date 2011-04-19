/* 5AppPicClerks.c */

#include "syscall.h"

int main() {

	Exec("../test/AppClerk");
	Exec("../test/AppClerk");
	Exec("../test/AppClerk");
	Exec("../test/AppClerk");
	Exec("../test/AppClerk");
	Exec("../test/PicClerk");
	Exec("../test/PicClerk");
	Exec("../test/PicClerk");
	Exec("../test/PicClerk");
	Exec("../test/PicClerk");
	
	Exit(0);
}