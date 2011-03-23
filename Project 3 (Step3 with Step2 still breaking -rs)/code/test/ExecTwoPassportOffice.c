/* ExecTwoPassportOffice.c
 *	
 * Execs the passport office
 *	
 */
 
 #include "syscall.h"
 
 int main() {
	
	Write("Executing Two PassportOffice...\n", sizeof("Executing PassportOffice...\n"), ConsoleOutput);
	Exec("../test/PassportOffice");
	Exec("../test/PassportOffice");
	
	Exit(0);
 
 
 }