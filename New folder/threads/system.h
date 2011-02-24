// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#define MAX_PROCESSES 64
#define MAX_LOCKS 512
#define MAX_CONDITIONS 512
#define MAX_MVS 512
#define MAX_FILENAME 256

#define NUM_SWAP_PAGES 8192

#define MAILBOXES 1000

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock

#ifdef USER_PROGRAM
#include "machine.h"
#include "synch.h"
extern Machine* machine;	// user program memory and registers
extern Lock* syscallLock;
extern Lock* forkLock;
extern Condition* finalThread;
extern Table processTable;
extern int numProcesses;
extern Lock* bitmapLock;
extern Lock* execLock;
extern Lock* lockLock;
extern Lock* condLock;
extern BitMap bitmap;
extern Table lockTable;
extern Table conditionTable;
extern Lock* printLock;

#include "synch.h"
#include "ipttranslate.h"
enum Policy{
	RAND,
	FIFO,
};
extern BitMap swapBitmap;
extern Lock* swapBitmapLock;
extern int nextTLBSlot;
extern int nextPageToEvict;
extern Lock* tlbLock;
extern OpenFile *swapFile;
extern IPTTranslationEntry* IPT[NumPhysPages];
extern Policy PageEvictionPolicy;
extern Lock* swapFileLock;
extern Lock* exeFileLock;
extern Lock* nextPageToEvictLock;
extern Lock* vpnLock;
extern Lock* iptLock;
extern Lock* IPTLock;
extern Lock* evictLock;
extern Lock* swapLock;
extern int nextMailbox;
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK

#include "post.h"
#include "ipttranslate.h"
extern PostOffice* postOffice;
extern int numServers;
extern int machineID;
extern BitMap mailboxBitmap;
#endif

#endif // SYSTEM_H
