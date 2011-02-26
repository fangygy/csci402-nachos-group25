#include "syscall.h"

int lock;

int
main(){

        int a[0];

        Print("\nNetworkLockKIll TEST\n",sizeof("\nNetworkLockKILL TEST\n"),a,0);
        ServerAcquire(0,0);
        ServerRelease(0,0);
        lock = CreateServerLock("pie",0);
        ServerAcquire(lock,0);
        ServerAcquire(lock,0);
	ServerRelease(lock,0);
	ServerRelease(lock,0);
}
