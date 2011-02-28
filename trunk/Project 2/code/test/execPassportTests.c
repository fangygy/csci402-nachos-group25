/* ExecPassportOffice.c
 *	
 * Execs the passport office
 *	
 */
 
 #include "syscall.h"
 
 int main() {
	
	Write("Executing PassportOffice Test1...\n", sizeof("Executing PassportOffice Test1...\n"), ConsoleOutput);
	Exec("../test/passportTest1");
	
	Exit(0);
 
 
 }