/* Client2A.c
 *	
 * Client that creates a lock
 *	Creates a CV
 *	Creates a MV
 *	Sets MV's value to 10
 *	Acquires lock
 *	Increase MV by 1
 *	Waits on that CV
 *	Deletes lock and CV at the end
 */

#include "syscall.h"

int lock;
int cv;
int mv;

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
	
	Trace("Creating MV\n", 0x9999);
	mv = CreateMV("MV", sizeof("MV"), 10);
	Trace("MV's index: ", mv);
	Trace("\n", 0x9999);
	Trace("MV's value: ", GetMV(mv));
	Trace("\n", 0x9999);
	
	Trace("Acquiring lock\n", 0x9999);
	ServerAcquire(lock);
	
	Trace("Incrementing MV\n", 0x9999);
	SetMV(mv, GetMV(mv)+1);
	
	Trace("Waiting on CV with Lock\n", 0x9999);
	ServerWait(cv, lock);
	Trace("Woken up from wait\n", 0x9999);
	Trace("Now deleteing lock and cv\n", 0x9999);
	
	ServerDestroyLock(lock);
	ServerDestroyCV(cv);
	
	Trace("Final MV value should be 4\n", 0x9999);
	Trace("Final MV value: ", GetMV(mv));
	Trace("\n", 0x9999);
	/* not reached */
	Exit(0);
}
