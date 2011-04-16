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
	lock = ServerCreateLock("LockA", sizeof("LockA"), 1);
	lock2 = ServerCreateLock("LockB", sizeof("LockB"), 1);
	Trace("LockA's index: ", lock);
	Trace("\n", 0x9999);
	Trace("LockB's index: ", lock2);
	Trace("\n", 0x9999);
	ServerAcquire(lock, 0);
	ServerDestroyLock(lock, 0);
	
	/* not reached */
	Exit(0);
}
