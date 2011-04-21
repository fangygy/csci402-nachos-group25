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
	lock = ServerCreateLock("Lock", sizeof("Lock"), 1);
	Trace("Lock's index: ", lock);
	Trace("\n", 0x9999);
	Trace("Creating CV", 0x9999);
	Trace("\n", 0x9999);
	cv = ServerCreateCV("CV", sizeof("CV"), 1);
	Trace("CV's index: ", cv);
	Trace("\n", 0x9999);
	Trace("Acquiring lock\n", 0x9999);
	ServerAcquire(lock, 0);
	Trace("Waiting on CV with Lock\n", 0x9999);
	ServerWait(cv, 0, lock, 0);
	Trace("Woken up from wait\n", 0x9999);
	Trace("Now deleteing lock and cv\n", 0x9999);
	ServerRelease(lock, 0);
	
	ServerDestroyLock(lock, 0);
	ServerDestroyCV(cv, 0);
	
	/* not reached */
	Exit(0);
}
