// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!

//----------------------------------------------------------------------
// Lock::Lock
// 	Constructor that takes in a char for name debugging purposes,
//	boolean "free" for whether the lock is available, and Thread 
//	"holder" for keeping the current holder of this lock. 
//	Semaphore used for actual locking purposes.
//----------------------------------------------------------------------
Lock::Lock(char* debugName) 
{
	name = debugName;
	free = true;
	holder = NULL;
	queue = new List;
}

Lock::~Lock() 
{
	delete queue;
	delete holder;
}

//----------------------------------------------------------------------
// Lock::Acquire
//	Lock is held by the first thread to call it, and subsequent
//	threads are placed into the semaphore's wait queue. If the lock
//	holder attempts to recall acquire, nothing will occur. This is an
//	atomic function.
//----------------------------------------------------------------------
void Lock::Acquire() 
{
	IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interrupts

	if (currentThread == holder) {
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	if (free) {
		free = false;
		holder = currentThread;
	}
	else {
		queue->Append((void *)currentThread);
		currentThread->Sleep();
	}

	(void) interrupt->SetLevel(oldLevel); //restore interrupts
}

//----------------------------------------------------------------------
// Lock::Release
//	Should only be called by lockowner, otherwise will print an error 
//	message and return. If there are any waiting threads, will pass
//	ownership to them, otherwise will free avaliability of lock.
//	This is an atomic function.
//----------------------------------------------------------------------
void Lock::Release() 
{
	IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interrupts

	if (currentThread != holder) {	//If non-lockowner is calling release, exit function
		printf("%s: Non-lockowner attempting to release lock. Preventing.", currentThread->getName());
		printf("\n");
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	if (!(queue->IsEmpty())) { //If there are threads waiting
		holder = (Thread *) queue->Remove(); //Pass ownership
		scheduler->ReadyToRun(holder);
	}
	else { //No threads waiting
		free = true; //This lock is available
		holder = NULL; //Ownership is free
	}

	(void) interrupt->SetLevel(oldLevel); //restore interrupts
}

//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
//	Returns a true if the currentThread running is the owner of this
//	lock. False if otherwise.
//----------------------------------------------------------------------
bool Lock::isHeldByCurrentThread()
{
	if (currentThread == holder) 
		return true;
	else
		return false;
}

Condition::Condition(char* debugName) 
{ 
	name = debugName;
	waitingLock = NULL;
	queue = new List;
}

Condition::~Condition() 
{ 
	delete queue;
	delete waitingLock;
}

void Condition::Wait(Lock* conditionLock) 
{ 
	IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interrupts

	if (conditionLock == NULL) {
		printf("Lock does not exist. Exiting CV.");
		printf("\n");
		(void) interrupt->SetLevel(oldLevel); //restore interrupts
		return;
	}

	if (waitingLock == NULL) {
		waitingLock = conditionLock;
	}
	
	if (waitingLock != conditionLock) {
		printf("CV's lock is not the one being called. Exiting.");
		printf("\n");
		(void) interrupt->SetLevel(oldLevel); //restore interrupts
		return;
	}

	//OK to start doing actual waiting
	printf("About to append %s to queue\n",currentThread->getName());
	queue->Append((void*)currentThread);
	conditionLock->Release();
	printf("Released lock %s in CV and sleeping\n", name);
	currentThread->Sleep();
	printf("Woken and Acquiring lock %s...\n", name);
	conditionLock->Acquire();

	(void) interrupt->SetLevel(oldLevel); //restore interrupts
}

void Condition::Signal(Lock* conditionLock) 
{ 
	IntStatus oldLevel = interrupt->SetLevel(IntOff); //disable interrupts

	if (queue->IsEmpty()) {
		(void) interrupt->SetLevel(oldLevel); //restore interrupts
		return;
	}

	if (waitingLock != conditionLock) {
		printf("CV's lock is not the one being called. Exiting.");
		printf("\n");
		(void) interrupt->SetLevel(oldLevel); //restore interrupts
		return;
	}
	Thread* t = (Thread *)queue->Remove();
	t->setStatus(READY);
	scheduler->ReadyToRun(t);
	//conditionLock->Release();

	if (queue->IsEmpty()) {
		waitingLock = NULL;
	}

	(void) interrupt->SetLevel(oldLevel); //restore interrupts
}
void Condition::Broadcast(Lock* conditionLock) 
{ 
	while (!(queue->IsEmpty())) {
		Signal(conditionLock);
	}
}
