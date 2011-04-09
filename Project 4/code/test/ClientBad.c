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
	ServerAcquire(lock);
	
	/* Destroy a lock I don't own */ 
	Trace("Destroy a lock I don't own.\n", 0x9999);
	ServerDestroyLock(lock);
	
	Trace("Creating a CV\n", 0x9999);
	cv = ServerCreateCV("CV", sizeof("CV"));
	
	/* Waiting on a CV with a lock I don't own */
	Trace("Waiting on a CV with a lock I don't own\n", 0x9999);
	ServerWait(cv, lock);
	
	/* Signalling a CV with a lock I don't own */
	Trace("Signalling a CV with a lock I don't own\n", 0x9999);
	ServerSignal(cv, lock);
	
	/* Broadcasting on a CV with a lock I don't own */
	Trace("Broadcasting on a CV with a lock I don't own\n", 0x9999);
	ServerBroadcast(cv, lock);
	
	lock = ServerCreateLock("lock", sizeof("lock"));
	cv2 = 0;
	/* Waiting on a CV I haven't created */
	Trace("Waiting on a CV I haven't created\n", 0x9999);
	ServerWait(cv2, lock);
	
	/* Signalling a CV I haven't created */
	Trace("Signalling a CV I havne't created\n", 0x9999);
	ServerSignal(cv2, lock);
	
	/* Broadcasting on a CV I haven't created */
	Trace("Broadcasting on a CV I haven't created\n", 0x9999);
	ServerBroadcast(cv2, lock);
	
	Trace("Done screwing around.\n", 0x9999);
	
	/* not reached */
	Exit(0);
}
