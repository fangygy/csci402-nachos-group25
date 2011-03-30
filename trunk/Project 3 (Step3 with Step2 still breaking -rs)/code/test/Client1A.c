/* Client1A.c
 *	
 * Client that creates and acquires a lock
 *	Creates a CV
 *	Waits on that CV
 *	Deletes lock and CV at the end
 */

#include "syscall.h"

int lock;
int cv;

int main()
{
	Trace("Creating Lock", 0x9999);
	Trace("\n", 0x9999);
	lock = ServerCreateLock("Lock", sizeof("Lock"));
	Trace("Lock's index: ", lock);
	Trace("\n", 0x9999);
	Trace("Creating CV", 0x9999);
	Trace("\n", 0x9999);
	cv = ServerCreateCV("CV", sizeof("CV"));
	Trace("CV's index: ", cv);
	Trace("\n", 0x9999);
	Trace("Acquiring lock\n", 0x9999);
	ServerAcquire(lock);
	Trace("Waiting on CV with Lock\n", 0x9999);
	ServerWait(cv, lock);
	Trace("Woken up from wait\n", 0x9999);
	Trace("Now deleteing lock and cv\n", 0x9999);
	
	ServerDestroyLock(lock);
	ServerDestroyCV(cv);
	
	/* not reached */
	Exit(0);
}
