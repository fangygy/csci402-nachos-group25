/* ClientBad.c
 *	
 * Client that performs lock and CV operations using locks/cvs
 *	it has not created yet
 *	
 *
 */

#include "syscall.h"

char* name;
int length;
int lock;
int lock2;
int cv;
int cv2;

int main()
{
	lock = 0;
	/* Acquire a lock before creating */ 
	Trace("Acquiring a lock I haven't created.\n", 0x9999);
	ServerAcquire(lock, 0);
	
	/* Destroy a lock I don't own */ 
	Trace("Destroy a lock I don't own.\n", 0x9999);
	ServerDestroyLock(lock, 0);
	
	Trace("Creating a CV\n", 0x9999);
	cv = ServerCreateCV("CV", sizeof("CV"), 1);
	
	/* Waiting on a CV with a lock I don't own */
	Trace("Waiting on a CV with a lock I don't own\n", 0x9999);
	ServerWait(cv, 0, lock, 0);
	
	/* Signalling a CV with a lock I don't own */
	Trace("Signalling a CV with a lock I don't own\n", 0x9999);
	ServerSignal(cv, 0, lock, 0);
	
	/* Broadcasting on a CV with a lock I don't own */
	Trace("Broadcasting on a CV with a lock I don't own\n", 0x9999);
	ServerBroadcast(cv, 0, lock, 0);
	
	lock = ServerCreateLock("lock", sizeof("lock"), 1);
	cv2 = 0;
	/* Waiting on a CV I haven't created */
	Trace("Waiting on a CV I haven't created\n", 0x9999);
	ServerWait(cv2, 0, lock, 0);
	
	/* Signalling a CV I haven't created */
	Trace("Signalling a CV I havne't created\n", 0x9999);
	ServerSignal(cv2, 0, lock, 0);
	
	/* Broadcasting on a CV I haven't created */
	Trace("Broadcasting on a CV I haven't created\n", 0x9999);
	ServerBroadcast(cv2, 0, lock, 0);
	
	Trace("Done screwing around.\n", 0x9999);
	
	/* not reached */
	Exit(0);
}
