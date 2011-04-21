/* 1ManagerSenator.c
*
*
*
*/

#include "syscall.h"

int main() {

	Exec("../test/Manager");
	Exec("../test/Senator");
	Exec("../test/Senator");
	Exec("../test/Senator");
	Exec("../test/Senator");
	Exec("../test/Senator");
	
	Exit(0);
}