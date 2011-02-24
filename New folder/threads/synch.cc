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
Lock::Lock(char* debugName) 
{
	name = debugName;
	owner = NULL;
	state = FREE;
	waitQueue = new List;
	markedForDeletion = false;
}

Lock::~Lock() {
	//delete waitQueue; 
}

void Lock::Acquire()
{
	//disable interrupts
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	//if currentThread is the owner, restore interrupts and return
	if(owner == currentThread)
	{
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	//if the lock is available, make it busy and make currentThread the owner
	if(state == FREE)
	{
		state = BUSY;
		owner = currentThread;
	}

	//else the lock is not available, so add it to the end of the waiting
	//Queue and set it to sleep
	else
	{
		waitQueue->Append(currentThread);
		currentThread->Sleep();
	}

	//restore interrupts
	(void) interrupt->SetLevel(oldLevel);
}

void Lock::Acquire(Lock* sysLock)
{
	//disable interrupts
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	
	/*THIS IS A DISGUSTING HACK TO ELIMINATE AN 
	//EVEN MORE DISGUSTING RACE CONDITION THAT OCCURS
	//IN OUR USER PROGRAM SYSCALL*/
	sysLock->Release();

	//if currentThread is the owner, restore interrupts and return
	if(owner == currentThread)
	{
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	//if the lock is available, make it busy and make currentThread the owner
	if(state == FREE)
	{
		state = BUSY;
		owner = currentThread;
	}

	//else the lock is not available, so add it to the end of the waiting
	//Queue and set it to sleep
	else
	{
		waitQueue->Append(currentThread);
		currentThread->Sleep();
	}

	//restore interrupts
	(void) interrupt->SetLevel(oldLevel);
}

void Lock::Release()
{
	//disable interrupts
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	//if currentThread != owner, print a message, restore interrupts  and return
	if(currentThread != owner)
	{
	        DEBUG('t', "currentThread \"%s\" is not the owner\n", currentThread->getName());
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	//if we reach this point currentThread is the owner

	//if there is a thread in the waiting queue, take the first thread in the
	//waiting queue and set its status to ready, add it to the readyQueue, remove
	//it from the waitQueue, and make it the owner
	if(!waitQueue->IsEmpty())
	{
		Thread* t = (Thread*)waitQueue->Remove();
		t->setStatus(READY);
		scheduler->ReadyToRun(t);
		owner = t;
	}

	//if there is no thread in the waiting queue, make the lock available and
	//clear the ownership
	else
	{
		state = FREE;
		owner = NULL;
	}

	//restore interrupts
	(void) interrupt->SetLevel(oldLevel);
}

bool Lock::isFree(){
	if(state == FREE){
		return true;
	}
	return false;
}

void Lock::markForDeletion(){
	markedForDeletion = true;
}

bool Lock::isMarked(){
	return markedForDeletion;
}


Condition::Condition(char* debugName)
{ 
	name = debugName;
	myLock = NULL;
	waitQueue = new List;
	markedForDeletion = false;
}

Condition::~Condition() { 
	delete waitQueue; 
}

void Condition::Wait(Lock* conditionLock)
{ 
	//disable interrupts
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	// Keep track of the lock being used with this CV
	
	if(conditionLock == NULL)
	{
		DEBUG('t', "Condition Variable \"%s\" is not using a lock.\n", getName());
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	
	// Save the lock pointer in the condition class - 
	// for the FIRST waiter (ALL waiters who wait have given up the same lock)
	if(myLock == NULL)
		myLock = conditionLock; // save lock pointer

	conditionLock->Release(); //Exit monitor
	waitQueue->Append((void*)currentThread);  // add myself to the wait queue
	currentThread->Sleep();
	conditionLock->Acquire(); //Re-enter monitor

	(void) interrupt->SetLevel(oldLevel);
}

void Condition::Wait(Lock* conditionLock, Lock* sysLock)
{ 
	//disable interrupts
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	
	/*THIS IS A DISGUSTING HACK TO ELIMINATE AN 
	//EVEN MORE DISGUSTING RACE CONDITION THAT OCCURS
	//IN OUR USER PROGRAM SYSCALL*/
	sysLock->Release();

	// Keep track of the lock being used with this CV
	
	if(conditionLock == NULL)
	{
		DEBUG('t', "Condition Variable \"%s\" is not using a lock.\n", getName());
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	
	// Save the lock pointer in the condition class - 
	// for the FIRST waiter (ALL waiters who wait have given up the same lock)
	if(myLock == NULL)
		myLock = conditionLock; // save lock pointer

	conditionLock->Release(); //Exit monitor
	waitQueue->Append((void*)currentThread);  // add myself to the wait queue
	currentThread->Sleep();
	conditionLock->Acquire(); //Re-enter monitor

	(void) interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock* conditionLock)
{ 
	// disable interrupts
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	// If no waiters, restore interrupts and return
	if(waitQueue->IsEmpty())
	{
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	// Verify the Signaler's lock matches the lock 
	// given up by Waiters, if it doesn't, print message
	// restore interrupts, and return
	if(myLock != conditionLock)
	{
                DEBUG('t', "Signaler \"%s\"\'s lock does not match the lock associated with this Condition Variable!\n", currentThread->getName());
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
  
	// Wake up 1 waiter
	Thread* newThread;
	newThread = (Thread*) waitQueue->Remove();	// remove one thread from CV waitQueue
	newThread->setStatus(READY);	// set thread to ready state
	scheduler->ReadyToRun(newThread);	// put thread on ready queue

	// If no more waiting threads, clear lock pointer
	if(waitQueue->IsEmpty())
	{
		myLock = NULL;
	}

	// restore interrupts
	(void) interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock* conditionLock)
{ 
	// while there are waiters signal them
	while(!waitQueue->IsEmpty())
	{
		Signal(conditionLock);
	}
}

void Condition::markForDeletion(){
	markedForDeletion = true;
}

bool Condition::isFree(){
	return waitQueue->IsEmpty();
}

bool Condition::isMarked(){
	return markedForDeletion;
}
