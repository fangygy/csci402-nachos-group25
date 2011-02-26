#include "syscall.h"

int lock1, cv1;

int
main(){

        int a[0];

        Print("\nsimple TEST\n",sizeof("\nsimple TEST\n"),a,0);
        lock1 = CreateServerLock("pie",0);
        cv1 = CreateServerCondition("strudle",0);

        ServerAcquire(lock1,0);
        Exec("../test/simpleNetwork2");
	ServerWait(cv1,lock1,0);
	ServerRelease(lock1,0);
}


