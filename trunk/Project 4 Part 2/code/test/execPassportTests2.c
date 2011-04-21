/* ExecPassportOffice.c
 *	
 * Execs the passport office
 *	
 */
 
 #include "syscall.h"
 
 int main() {
	
	Write("Executing PassportOffice Test3...\n", sizeof("Executing PassportOffice Test3...\n"), ConsoleOutput);
	Exec("../test/passportTest3");

	Write("\n============================================================\n", sizeof("\n============================================================\n"), ConsoleOutput);

	Exit(0);
 
 
 }