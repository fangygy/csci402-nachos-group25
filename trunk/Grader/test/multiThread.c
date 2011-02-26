#include "syscall.h"

int threads = 10;
int count = 0;
int lock;
int cond;

void aThread(){
	int A[1];
	Acquire(lock);
	count += 1;
	Signal(cond, lock);
	Wait(cond, lock);
	Signal(cond, lock);
	Release(lock);
	Print("aThread finished\n", sizeof("aThread finished\n"), A, 0);
	
	
	Exit(0);
}

int main(){
	int i;
	lock = CreateLock("lock");
	cond = CreateCondition("cond");
	Acquire(lock);
	for(i = 0; i < threads; i++){
		Fork(aThread);
	}

	while(count < threads){
		Wait(cond, lock);
	}
	
	Signal(cond, lock);
	Release(lock);
}
