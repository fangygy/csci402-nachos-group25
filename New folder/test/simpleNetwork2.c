#include "syscall.h"

int lock1, cv1,i;

int
main(){

        int a[0];

        Print("\nsimple TEST2\n",sizeof("\nsimple TEST2\n"),a,0);
        lock1 = CreateServerLock("pie",0);
        cv1 = CreateServerCondition("strudle",0);
	
	for(i=0;i<100;i++){
	Yield();
	}
        ServerAcquire(lock1,0);
	ServerSignal(cv1,lock1,0);
	ServerRelease(lock1,0);
}


