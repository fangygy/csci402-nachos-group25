/* syscalls.h 
 * 	Nachos system call interface.  These are Nachos kernel operations
 * 	that can be invoked from user programs, by trapping to the kernel
 *	via the "syscall" instruction.
 *
 *	This file is included by user programs and by the Nachos kernel. 
 *
 * Copyright (c) 1992-1993 The Regents of the University of California.
 * All rights reserved.  See copyright.h for copyright notice and limitation 
 * of liability and disclaimer of warranty provisions.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "copyright.h"

/* system call codes -- used by the stubs to tell the kernel which system call
 * is being asked for
 */
#define SC_Halt					0
#define SC_Exit					1
#define SC_Exec					2
#define SC_Join					3
#define SC_Create				4
#define SC_Open					5
#define SC_Read					6
#define SC_Write				7
#define SC_Close				8
#define SC_Fork					9
#define SC_Yield				10

#define SC_Acquire				11
#define SC_Release				12
#define SC_Wait					13
#define SC_Signal				14
#define SC_Broadcast			15
#define SC_CreateLock			16
#define SC_DestroyLock			17
#define SC_CreateCondition		18
#define SC_DestroyCondition		19
#define SC_Random				20
#define SC_Trace				21

#define SC_CreateMV				22
#define SC_GetMV				23
#define SC_SetMV				24

#define SC_ServerCreateLock		25
#define SC_ServerAcquire		26
#define SC_ServerRelease		27
#define SC_ServerCreateCV		28
#define SC_ServerWait			29
#define SC_ServerSignal			30
#define SC_ServerBroadcast		31
#define SC_ServerDestroyLock	32
#define SC_ServerDestroyCV		33



#define MAXFILENAME 256

#ifndef IN_ASM

/* The system call interface.  These are the operations the Nachos
 * kernel needs to support, to be able to run user programs.
 *
 * Each of these is invoked by a user program by simply calling the 
 * procedure; an assembly language stub stuffs the system call code
 * into a register, and traps to the kernel.  The kernel procedures
 * are then invoked in the Nachos kernel, after appropriate error checking, 
 * from the system call entry point in exception.cc.
 */

/* Stop Nachos, and print out performance stats */
void Halt();		
 

/* Address space control operations: Exit, Exec, and Join */

/* This user program is done (status = 0 means exited normally). */
void Exit(int status);	

/* A unique identifier for an executing user program (address space) */
typedef int SpaceId;	
 
/* Run the executable, stored in the Nachos file "name", and return the 
 * address space identifier
 */
SpaceId Exec(char *name);
 
/* Only return once the the user program "id" has finished.  
 * Return the exit status.
 */
int Join(SpaceId id); 	
 

/* File system operations: Create, Open, Read, Write, Close
 * These functions are patterned after UNIX -- files represent
 * both files *and* hardware I/O devices.
 *
 * If this assignment is done before doing the file system assignment,
 * note that the Nachos file system has a stub implementation, which
 * will work for the purposes of testing out these routines.
 */
 
/* A unique identifier for an open Nachos file. */
typedef int OpenFileId;	

/* when an address space starts up, it has two open files, representing 
 * keyboard input and display output (in UNIX terms, stdin and stdout).
 * Read and Write can be used directly on these, without first opening
 * the console device.
 */

#define ConsoleInput	0  
#define ConsoleOutput	1  
 
/* Create a Nachos file, with "name" */
void Create(char *name, int size);

/* Open the Nachos file "name", and return an "OpenFileId" that can 
 * be used to read and write to the file.
 */
OpenFileId Open(char *name, int size);

/* Write "size" bytes from "buffer" to the open file. */
void Write(char *buffer, int size, OpenFileId id);

/* Read "size" bytes from the open file into "buffer".  
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough 
 * characters to read, return whatever is available (for I/O devices, 
 * you should always wait until you can return at least one character).
 */
int Read(char *buffer, int size, OpenFileId id);

/* Close the file, we're done reading and writing to it. */
void Close(OpenFileId id);



/* User-level thread operations: Fork and Yield.  To allow multiple
 * threads to run within a user program. 
 */

/* Fork a thread to run a procedure ("func") in the *same* address space 
 * as the current thread.
 */
void Fork(void (*func)());

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void Yield();		

/* 
 * Lock functions: Acquire and Release.
 */

/* Acquires the lock
 */
void Acquire(int index);

/* Releases the lock
 */
void Release(int index);

/* 
 * Condition Variable functions: Wait, Signal and Broadcast
 */
 
/* Waits on the condition variable to signal it awake.
 */
void Wait(int cIndex, int lIndex);

 
/* Signals the next thread waiting on the condition variable awake.
 */
void Signal(int cIndex, int lIndex);

/* Wakes up all sleeping threads waiting on the condition variable.
 */
void Broadcast(int cIndex, int lIndex);

/* 
 * Constructor and destructor functions for locks and condition variables.
 */

/* Creates a Lock.
 */
int CreateLock(char* name, int length);

/* Destroys a Lock.
 */
int DestroyLock(int index);

/* Creates a condition variable.
*/
int CreateCondition(char* name, int length);

/* Destroys a condition variable.
*/
int DestroyCondition(int index);

/* Returns a random number from 0 to max.
*/
int Random(int max);

/* Print a sentence and a number to the console window.
*/
void Trace(int vaddr, int val);

/* ******************** Networking stuff *********************/
/* Creates a monitor variable for networking 
*/
int CreateMV(char* name, int length, int value);

/* Gets a monitor variable for networking 
*/
int GetMV(int index);

/* Sets a monitor variable for networking 
*/
void SetMV(int index, int val);

/* Creates a lock for networking 
*/
int ServerCreateLock(char* name, int length);

/* Acquires a lock for networking 
*/
void ServerAcquire(int lockIndex);

/* Releases a lock for networking 
*/
void ServerRelease(int lockIndex);

/* Creates a Condition Variable for networking 
*/
int ServerCreateCV(unsigned int vaddr, int length);

/* Waits on a condition variable for networking 
*/
void ServerWait(int conditionIndex, int lockIndex);

/* Signals a condition variable for networking 
*/
void ServerSignal(int conditionIndex, int lockIndex);

/* Broadcasts all waiting condition variables for networking 
*/
void ServerBroadcast(int conditionIndex, int lockIndex);

/* Destroys a lock for networking 
*/
void ServerDestroyLock(int lockIndex);

/* Destroys a Condition Variable for networking 
*/
void ServerDestroyCV(int conditionIndex);



#endif /* IN_ASM */

#endif /* SYSCALL_H */
