/* ExecPassportOffice.c
 *	
 * Execs the passport office
 *	
 */
 
 #include "syscall.h"
 
 int main() {
	
	Write("Executing PassportOffice Test1...\n", sizeof("Executing PassportOffice Test1...\n"), ConsoleOutput);
	Exec("../test/passportTest1");

	Write("\n============================================================\n", sizeof("\n============================================================\n"), ConsoleOutput);

	Write("Executing PassportOffice Test2...\n", sizeof("Executing PassportOffice Test2...\n"), ConsoleOutput);
	Exec("../test/passportTest2");
	
	Exit(0);
 
 
 }