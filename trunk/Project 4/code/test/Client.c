/* Client.c
 *	
 * Client that creates and acquires a lock
 *	
 *	
 *
 */

#include "syscall.h"

char* name;
int length;
int lock;
int lock2;

int main()
{
	lock = ServerCreateLock("LockA", sizeof("LockA"));
	lock2 = ServerCreateLock("LockB", sizeof("LockB"));
	Trace("LockA's index: ", lock);
	Trace("\n", 0x9999);
	Trace("LockB's index: ", lock2);
	Trace("\n", 0x9999);
	ServerAcquire(lock);
	ServerDestroyLock(lock);
	
	/* not reached */
	Exit(0);
}
