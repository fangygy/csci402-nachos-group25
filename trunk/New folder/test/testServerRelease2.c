#include "syscall.h"

int lock1,lock2;

int
main(){
	lock1 = CreateServerLock("Lock1",1);
	lock2 = CreateServerLock("Lock2",0);
	ServerAcquire(lock1,1);
	ServerRelease(lock1,1);
	ServerRelease(lock2,0);
	Exit(0);
}
