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
	ServerAcquire(lock);
	ServerDestroyLock(lock);
	
	/* not reached */
	Exit(0);
}
