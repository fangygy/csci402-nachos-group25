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

int main()
{
	lock = ServerCreateLock("LockA", sizeof("LockA"));
	ServerAcquire(lock);
	ServerDestroyLock(lock);
	
	/* not reached */
	Exit(0);
}
