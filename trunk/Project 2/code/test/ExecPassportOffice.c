/* ExecPassportOffice.c
 *	
 * Execs the passport office
 *	
 */
 
 #include "syscall.h"
 
 int main() {
	
	Write("Executing PassportOffice...\n", sizeof("Executing PassportOffice...\n"), ConsoleOutput);
	Exec("../test/PassportOffice");
	
	Exit(0);
 
 
 }