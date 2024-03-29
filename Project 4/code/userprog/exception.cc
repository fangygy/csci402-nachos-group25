// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "synch.h"
#include "process.h"
#include "../machine/network.h"
#include "../network/post.h"
#include <stdio.h>
#include <iostream>
#include <ctime>		// For seeding random
#include <cstdlib>	// For generating random

using namespace std;

#define MAX_LOCKS 512
#define MAX_CONDITIONS 512
#define MAX_MVS 512
#define MAX_CLIENTS 512

int numLocks = 0;
int numConditions = 0;

Lock* lock_condLock = new Lock("lock_condLock");
Lock* memoryLock = new Lock("memoryLock");
Lock* traceLock = new Lock("traceLock");

struct KernelLock {
	Lock* lock;
	AddrSpace* space;
	bool beingAcquired;
	bool isToBeDeleted;
	bool deleted;
	
	KernelLock() {
		lock = NULL;
		beingAcquired = false;
		space = NULL;
		isToBeDeleted = false;
		deleted = true;
	}
};

struct KernelCondition {
	Condition* condition;
	AddrSpace* space;
	bool beingAcquired;
	bool isToBeDeleted;
	bool deleted;
	
	KernelCondition() {
		condition = NULL;
		beingAcquired = false;
		space = NULL;
		isToBeDeleted = false;
		deleted = true;
	}
};

KernelLock locks[MAX_LOCKS];
KernelCondition conditions[MAX_CONDITIONS];

int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
	  {
   			result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
	  }	
      
      buf[n++] = *paddr;
     
      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
      result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
		}
    }

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
	printf("%c",buf[ii]);
      }

    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    f->Write(buf, len);
	} else {
	    printf("%s","Bad OpenFileId passed to Write\n");
	    len = -1;
	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    len = f->Read(buf, len);
	    if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
  	        if ( copyout(vaddr, len, buf) == -1 ) {
		    printf("%s","Bad pointer passed to Read: data not copied\n");
		}
	    }
	} else {
	    printf("%s","Bad OpenFileId passed to Read\n");
	    len = -1;
	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}

int CreateLock_Syscall(unsigned int vaddr, int length) {
	// axe crowley
	lock_condLock-> Acquire();
	if (numLocks >= MAX_LOCKS) {
		// print error msg?
		printf("CreateLock_Syscall: Error: Number of locks exceeded maximum lock limit. Returning.\n");
		return -1;
	}
	
	int index = -1;
	for (int i = 0; i < MAX_LOCKS; i++) {
		// find 1st vacancy in the list
		if (locks[i].lock == NULL) {
			index = i;
			break;
		}
	}
	
	char* name;
	
	if ( !(name = new char[length]) ) {
		printf("%s","Error allocating kernel buffer for lock creation!\n");
		lock_condLock->Release();
		return -1;
    } else {
        if ( copyin(vaddr,length,name) == -1 ) {
			printf("%s","Bad pointer passed to lock creation\n");
			delete[] name;
			lock_condLock->Release();
			return -1;
		}
    }
	
	locks[index].lock = new Lock(name);
	locks[index].space = currentThread->space;		// double-check
	locks[index].beingAcquired = false;
	locks[index].isToBeDeleted = false;
	locks[index].deleted = false;
	numLocks ++;
	
	lock_condLock-> Release();
	return (index);
}

int CreateCondition_Syscall(unsigned int vaddr, int length) {
	lock_condLock-> Acquire();
	if (numConditions >= MAX_CONDITIONS) {
		// print error msg?
		printf("CreateCondition_Syscall: Error: Number of CVs exceeded maximum CV limit.\n");
		return -1;
	}
	
	int index = -1;
	for (int i = 0; i < MAX_CONDITIONS; i++) {
	// find 1st vacancy in the list
		if (conditions[i].condition == NULL) {
			index = i;
			break;
		}
	}
	
	char* name;
	
	if ( !(name = new char[length]) ) {
		printf("%s","Error allocating kernel buffer for condition creation!\n");
		lock_condLock->Release();
		return -1;
    } else {
        if ( copyin(vaddr,length,name) == -1 ) {
			printf("%s","Bad pointer passed to condition creation\n");
			delete[] name;
			lock_condLock->Release();
			return -1; 
		}
    }
	
	conditions[index].condition = new Condition(name);
	conditions[index].space = currentThread->space;		// double-check
	conditions[index].beingAcquired = false;
	conditions[index].isToBeDeleted = false;
	conditions[index].deleted = false;
	numConditions ++;
	
	lock_condLock-> Release();
	return (index);
}

int DestroyLock_Syscall(int index) {
	lock_condLock-> Acquire();
	// check on return value
	if (index < 0) {
		lock_condLock->Release();
		printf("DestroyLock_Syscall: Lock index less than zero. Invalid.\n");
		// print error msg
		return 0;
	}
	if (index >= MAX_LOCKS) {
		lock_condLock->Release();
		printf("DestroyLock_Syscall: Lock index >= MAX_LOCKS. Invalid.\n");
		// print error msg
		return 0;
	}
	if (locks[index].isToBeDeleted || locks[index].deleted) {
		// Delete has already been called for this lock. don't do anything
		printf("DestroyLock_Syscall: Delete has already been called for this lock.\n");
		lock_condLock->Release();
		return 0;
	}
	if (locks[index].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("DestroyLock_Syscall: Lock's address space is not equal to this thread's address space.\n");
		lock_condLock->Release();
		return 0;
	}
	if (locks[index].lock->getFree() && !locks[index].beingAcquired) {
		// Lock isn't in use; delete it
		printf("DestroyLock_Syscall: Lock isn't in use and is now being deleted.\n");
		delete locks[index].lock;
		locks[index].lock = NULL;			// nullify lock pointer; this is now a free space
		locks[index].space = NULL;			// make the address space null
		locks[index].isToBeDeleted = false;
		locks[index].deleted = true;
		numLocks --;
	} else {
		// Lock is still in use; will delete it later
		printf("DestroyLock_Syscall: Lock still in use. Delete later.\n");
		locks[index].isToBeDeleted = true;
	}
	
	lock_condLock->Release();
	return 1;
}

int DestroyCondition_Syscall(int index) {
	lock_condLock-> Acquire();
	
	if (index < 0) {
		lock_condLock->Release();
		// print error msg
		printf("DestroyCondition_Syscall: CV index less than zero. Invalid.\n");
		return 0;
	}
	if (index >= MAX_CONDITIONS) {
		lock_condLock->Release();
		printf("DestroyCondition_Syscall: CV index >= MAX_CONDITIONS. Invalid.\n");
		// print error msg
		return 0;
	}
	if (conditions[index].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("DestroyCondition_Syscall: Condition's address space is not equal to this thread's address space. Invalid.\n");
		lock_condLock->Release();
		return 0;
	}
	if (conditions[index].isToBeDeleted || conditions[index].deleted) {
		// Delete has already been called for this condition. don't do anything
		lock_condLock->Release();
		printf("DestroyCondition_Syscall: CV is going to be deleted. Ignored. \n");
		return 0;
	}
	if (conditions[index].condition->getFree() && !conditions[index].beingAcquired) {
		// Condition isn't in use; delete it
		delete conditions[index].condition;
		printf("DestroyCondition_Syscall: CV not in use and is now deleted.\n");
		conditions[index].condition = NULL;		// nullify condition pointer; this is now a free space
		conditions[index].space = NULL;			// make the address space null
		conditions[index].isToBeDeleted = false;
		conditions[index].deleted = true;
		numConditions --;
	} else {
		// Condition is still in use; will delete it later
		printf("DestroyCondition_Syscall: CV still in use. Will delete later.\n");
		conditions[index].isToBeDeleted = true;
	}
	
	lock_condLock->Release();
	return 1;
}

void Acquire_Syscall(int index){
	lock_condLock->Acquire();
	if (index < 0) {
		printf("Acquire_Syscall: Lock index less than zero. Invalid.\n");
		// print error msg
		lock_condLock->Release();
		return;
	}
	if (index >= MAX_LOCKS) {
		lock_condLock->Release();
		printf("Acquire_Syscall: Lock index >= MAX_LOCKS. Invalid.\n");
		// print error msg
		return;
	}
	if(locks[index].deleted){
		// Lock does not exist
		printf("Acquire_Syscall: Lock does not exist.\n");
		lock_condLock->Release();
		return;
	}
	//
	if(locks[index].isToBeDeleted){
		// Lock is going to be deleted, no further action permitted
		printf("Acquire_Syscall: Lock is going to be deleted. Ignored.\n");
		lock_condLock->Release();
		return;
	}
	if (locks[index].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("Acquire_Syscall: Lock's address space is not equal to this thread's address space.\n");
		lock_condLock->Release();
		return;
	}

	locks[index].beingAcquired = true;
	lock_condLock->Release();
	locks[index].lock->Acquire();
	locks[index].beingAcquired = false;
}

void Release_Syscall(int index){
	lock_condLock->Acquire();
	if (index < 0) {
		// print error msg
		printf("Release_Syscall: Lock index less than zero. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if (index >= MAX_LOCKS) {
		lock_condLock->Release();
		printf("Release_Syscall: Lock index >= MAX_LOCKS. Invalid.\n");
		// print error msg
		return;
	}
	if(locks[index].deleted){
		// Lock does not exist
		printf("Release_Syscall: Lock does not exist.\n");
		lock_condLock->Release();
		return;
	}
	if (locks[index].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("Release_Syscall: Lock's address space does not equal this thread's address space.\n");
		lock_condLock->Release();
		return;
	}

	locks[index].lock->Release();
	if(locks[index].lock->getFree() && locks[index].isToBeDeleted && !locks[index].beingAcquired){
		locks[index].deleted = true;
		locks[index].isToBeDeleted = false;
		delete locks[index].lock;
		numLocks--;
	}

	lock_condLock->Release();
}

void Signal_Syscall(int cIndex, int lIndex) {
	lock_condLock->Acquire();
	if (cIndex < 0) {
		// print error msg
		printf("Signal_Syscall: Signal index less than zero. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if (cIndex >= MAX_CONDITIONS) {
		lock_condLock->Release();
		printf("Signal_Syscall: CV index >= MAX_CONDITIONS. Invalid.\n");
		// print error msg
		return;
	}
	if (lIndex < 0) {
		// print error msg
		printf("Signal_Syscall: CV's Lock index less than zero. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if (lIndex >= MAX_LOCKS) {
		lock_condLock->Release();
		printf("Signal_Syscall: Lock index >= MAX_LOCKS. Invalid.\n");
		// print error msg
		return;
	}
	if(conditions[cIndex].deleted){
		// Condition does not exist
		printf("Signal_Syscall: CV does not exist. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if(locks[lIndex].deleted){
		// Lock does not exist
		printf("Signal_Syscall: CV's Lock does not exist. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if(conditions[cIndex].isToBeDeleted){
		// Condition is going to be deleted, no further action permitted
		printf("Signal_Syscall: CV is going to be deleted. Ignored. \n");
		lock_condLock->Release();
		return;
	}
	if(locks[lIndex].isToBeDeleted){
		// Lock is going to be deleted, no further action permitted
		printf("Signal_Syscall: CV's Lock is going to be deleted. Ignored. \n");
		lock_condLock->Release();
		return;
	}
	if (conditions[cIndex].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("Signal_Syscall: CV address space is wrong. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if (locks[lIndex].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("Signal_Syscall: CV's Lock index less than zero. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	
	conditions[cIndex].condition->Signal(locks[lIndex].lock);
	
	lock_condLock->Release();
	
}

void Broadcast_Syscall(int cIndex, int lIndex) {
	lock_condLock->Acquire();
	if (cIndex < 0) {
		// print error msg
		printf("Broadcast_Syscall: Condition index less than zero. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if (cIndex >= MAX_CONDITIONS) {
		lock_condLock->Release();
		printf("Broadcast_Syscall: CV index >= MAX_CONDITION. Invalid.\n");
		// print error msg
		return;
	}
	if (lIndex < 0) {
		// print error msg
		printf("Broadcast_Syscall: Lock index less than zero. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if (lIndex >= MAX_LOCKS) {
		lock_condLock->Release();
		printf("Broadcast_Syscall: Lock index >= MAX_LOCKS. Invalid.\n");
		// print error msg
		return;
	}
	if(conditions[cIndex].deleted){
		// Condition does not exist
		printf("Broadcast_Syscall: Condition does not exist.\n");
		lock_condLock->Release();
		return;
	}
	if(locks[lIndex].deleted){
		// Lock does not exist
		printf("Broadcast_Syscall: Lock does not exist.\n");
		lock_condLock->Release();
		return;
	}
	if(conditions[cIndex].isToBeDeleted){
		// Condition is going to be deleted, no further action permitted
		printf("Broadcast_Syscall: Condition is going to be deleted. Ignored.\n");
		lock_condLock->Release();
		return;
	}
	if(locks[lIndex].isToBeDeleted){
		// Lock is going to be deleted, no further action permitted
		printf("Broadcast_Syscall: Lock is going to be deleted. Ignored.\n");
		lock_condLock->Release();
		return;
	}
	if (conditions[cIndex].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("Broadcast_Syscall: Condition's address space is not equal to this thread's address space.\n");
		lock_condLock->Release();
		return;
	}
	if (locks[lIndex].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("Broadcast_Syscall: Lock's address space is not equal to this thread's address space.\n");
		lock_condLock->Release();
		return;
	}
	
	conditions[cIndex].condition->Broadcast(locks[lIndex].lock);
	
	lock_condLock->Release();
	
}

void Wait_Syscall(int cIndex, int lIndex) {
	lock_condLock->Acquire();
	if (cIndex < 0) {
		// print error msg
		printf("Wait_Syscall: CV index less than zero. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if (lIndex < 0) {
		// print error msg
		printf("Wait_Syscall: Lock index less than zero. Invalid.\n");
		lock_condLock->Release();
		return;
	}
	if (cIndex >= MAX_LOCKS) {
		lock_condLock->Release();
		printf("Wait_Syscall: CV index >= MAX_CONDITIONS. Invalid.\n");
		// print error msg
		return;
	}
	if (lIndex >= MAX_LOCKS) {
		lock_condLock->Release();
		printf("Wait_Syscall: Lock index >= MAX_LOCKS. Invalid.\n");
		// print error msg
		return;
	}
	if(conditions[cIndex].deleted){
		// Condition does not exist
		printf("Wait_Syscall: Condition does not exist.\n");
		lock_condLock->Release();
		return;
	}
	if(locks[lIndex].deleted){
		// Lock does not exist
		printf("Wait_Syscall: Lock does not exist.\n");
		lock_condLock->Release();
		return;
	}
	if(conditions[cIndex].isToBeDeleted){
		// Condition is going to be deleted, no further action permitted
		printf("Wait_Syscall: Condition is going to be deleted. Ignored.\n");
		lock_condLock->Release();
		return;
	}
	if(locks[lIndex].isToBeDeleted){
		// Lock is going to be deleted, no further action permitted
		printf("Wait_Syscall: Lock is going to be deleted. Ignored.\n");
		lock_condLock->Release();
		return;
	}
	if (conditions[cIndex].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("Wait_Syscall: Condition's address space not equal to this thread's address space.\n");
		lock_condLock->Release();
		return;
	}
	if (locks[lIndex].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		printf("Wait_Syscall: Lock's address space not equal to this thread's address space.\n");
		lock_condLock->Release();
		return;
	}
	
	
	locks[lIndex].beingAcquired = true;
	conditions[cIndex].beingAcquired = true;
	lock_condLock->Release();
	
	conditions[cIndex].condition->Wait(locks[lIndex].lock);
	
	lock_condLock->Acquire();
	locks[lIndex].beingAcquired = false;
	conditions[cIndex].beingAcquired = false;
	
	if(conditions[cIndex].condition->getFree() && conditions[cIndex].isToBeDeleted) {
		delete conditions[cIndex].condition;
		conditions[cIndex].condition = NULL;		// nullify condition pointer; this is now a free space
		conditions[cIndex].space = NULL;			// make the address space null
		conditions[cIndex].isToBeDeleted = false;
		conditions[cIndex].deleted = true;
		numConditions --;
	}
	lock_condLock->Release();
	return;
}

#ifdef NETWORK
int ClientReceive() {
	PacketHeader inPktHdr;
    MailHeader inMailHdr;
	
	char buffer[MaxMailSize];
	postOffice->Receive(currentThread->mailboxID, &inPktHdr, &inMailHdr, buffer);
	//printf("Client: Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from,inMailHdr.from);
	fflush(stdout);
	
	//Parse buffer into a return value
	int rv = 0;
	
	rv = atoi(buffer);
	//printf("Client: rv = %d\n", rv);
	
	return rv;
}
#endif

int CreateMV_Syscall(unsigned int vaddr, int length, int arraySize, int value) {
	char* name;
	
	//Read char* from the vaddr
	if ( !(name = new char[length]) ) {
		printf("%s","Error allocating kernel buffer for server lock creation!\n");
		return -1;
    } else {
        if ( copyin(vaddr,length,name) == -1 ) {
			printf("%s","Bad pointer passed to server lock creation\n");
			delete[] name;
			return -1;
		}
    }
	
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int mvIndex;
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "mon cre %s %d %d", name, arraySize, value);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	//postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in CreateMV: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in CreateMV: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in CreateMV: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in CreateMV: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in CreateMV: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in CreateMV: Not Lock owner\n", NOT_OWNER);
	}

    
	//Do data parsing here with mvIndex and buffer
	mvIndex = rv;
	
    fflush(stdout);
	#endif
	return mvIndex;
}

int GetMV_Syscall(int outerIndex, int innerIndex) {
	
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	
	int mvValue;
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "mon get %d %d", outerIndex, innerIndex);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	//postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in GetMV: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in GetMV: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in GetMV: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in GetMV: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in GetMV: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in GetMV: Not Lock owner\n", NOT_OWNER);
	}
	
	//Parse buffer to get mvValue
	mvValue = rv;
	
    fflush(stdout);
	#endif
	return mvValue;
}

void SetMV_Syscall(int outerIndex, int innerIndex, int val) {
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "mon set %d %d %d", outerIndex, innerIndex, val);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	//postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in SetMV: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in SetMV: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in SetMV: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in SetMV: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in SetMV: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in SetMV: Not Lock owner\n", NOT_OWNER);
	}
	//printf("Successfully set MV at Index: %d to Value: %d\n", outerIndex, val);
	
    fflush(stdout);
	#endif
}

int ServerCreateLock_Syscall(unsigned int vaddr, int length, int arraySize) {
	char* name;
	
	//Read char* from the vaddr
	if ( !(name = new char[length]) ) {
		printf("%s","Error allocating kernel buffer for server lock creation!\n");
		return -1;
    } else {
        if ( copyin(vaddr,length,name) == -1 ) {
			printf("%s","Bad pointer passed to server lock creation\n");
			delete[] name;
			return -1;
		}
    }
	
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	int lockIndex;
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "loc cre %s %d", name, arraySize);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	//postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in CreateLock: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in CreateLock: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in CreateLock: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in CreateLock: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in CreateLock: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in CreateLock: Not Lock owner\n", NOT_OWNER);
	}
	
	lockIndex = rv;
    
	//Do data parsing here with lockIndex and buffer
	//lockIndex = buffer?
	
    //fflush(stdout);		// clientReceive takes care of this
	
	#endif
	return lockIndex;
}

void ServerDestroyLock_Syscall(int outerLockIndex, int innerLockIndex){
	
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data [MaxMailSize];
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "loc des %d %d", outerLockIndex, innerLockIndex);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
	//sprintf(data, "loc des %d", outerLockIndex);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
//	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in DestroyLock: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in DestroyLock: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in DestroyLock: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in DestroyLock: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in DestroyLock: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in DestroyLock: Not Lock owner\n", NOT_OWNER);
	}

    //printf("Successfully sent a Destroy Request on Lock: %d\n", outerLockIndex);
	
    fflush(stdout);
	#endif
}

void ServerAcquire_Syscall(int outerLockIndex, int innerLockIndex) {
	
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "loc acq %d %d", outerLockIndex, innerLockIndex);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;
	
	#ifdef NETWORK
    // Send the first message
	//sprintf(data, "loc acq %d", outerLockIndex);
	//char* tdata = "lock acq 0";
	//sprintf(data, "%s", tdata);
	//printf("%s\n", data);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data);
	
    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
//	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in Acquire: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in Acquire: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in Acquire: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in Acquire: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in Acquire: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in Acquire: Not Lock owner\n", NOT_OWNER);
	}


    //printf("Successfully acquired Lock: %d\n", outerLockIndex);
	
    fflush(stdout);
	#endif
}

void ServerRelease_Syscall(int outerLockIndex, int innerLockIndex) {
	
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "loc rel %d %d", outerLockIndex, innerLockIndex);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
	//sprintf(data, "loc rel %d", outerLockIndex);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
//	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in Release: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in Release: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in Release: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in Release: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in Release: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in Release: Not Lock owner\n", NOT_OWNER);
	}


   // printf("Successfully released Lock: %d\n", outerLockIndex);
	
    fflush(stdout);
	#endif
}

int ServerCreateCV_Syscall(unsigned int vaddr, int length, int arraySize){
	
	char* name;
	
	//Read char* from the vaddr
	if ( !(name = new char[length]) ) {
		printf("%s","Error allocating kernel buffer for server cv creation!\n");
		return -1;
    } else {
        if ( copyin(vaddr,length,name) == -1 ) {
			printf("%s","Bad pointer passed to server cv creation\n");
			delete[] name;
			return -1;
		}
    }
	
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char buffer[MaxMailSize];
	
	int condIndex;
	
	//Create the correct message to send here? Ask Antonio later
	char request[MaxMailSize];
	sprintf(request, "con cre %s %d", name, arraySize);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(request) + 1;
	
	#ifdef NETWORK
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, request); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
//	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in CreateCV: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in CreateCV: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in CreateCV: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in CreateCV: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in CreateCV: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in CreateCV: Not Lock owner\n", NOT_OWNER);
	}

	condIndex = rv;
	
	//Parse buffer into a condIndex
	
    fflush(stdout);
	#endif
	return condIndex;
}

void ServerDestroyCV_Syscall(int outerConditionIndex, int innerConditionIndex){
	
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "con del %d %d", outerConditionIndex, innerConditionIndex);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
	//sprintf(data, "con del %d", outerConditionIndex);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
//	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);

	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in DestroyCV: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in DestroyCV: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in DestroyCV: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in DestroyCV: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in DestroyCV: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in DestroyCV: Not Lock owner\n", NOT_OWNER);
	}

	//printf("Successfully called Destroy on Condition: %d\n", outerConditionIndex);
	
    fflush(stdout);
	#endif
}

void ServerWait_Syscall(int outerConditionIndex, int innerConditionIndex, int outerLockIndex, int innerLockIndex){

	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "con wai %d %d %d %d", outerConditionIndex, innerConditionIndex, outerLockIndex, innerLockIndex);
	//printf("Segmentation?\n");
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;
	//printf("Segmentation?\n");

	#ifdef NETWORK
    // Send the first message
	//sprintf(data, "con wai %d %d", outerConditionIndex, outerLockIndex);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

	//printf("Segmentation?\n");
    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	//printf("Waiting on Condition: %d with Lock: %d\n", outerConditionIndex, outerLockIndex);
//	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in Wait: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in Wait: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in Wait: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in Wait: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in Wait: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in Wait: Not Lock owner\n", NOT_OWNER);
	}

	//printf("Woken on Condition: %d with Lock: %d\n", outerConditionIndex, outerLockIndex);
	
    fflush(stdout);
	#endif
}

void ServerSignal_Syscall(int outerConditionIndex, int innerConditionIndex, int outerLockIndex, int innerLockIndex){

	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "con sig %d %d %d %d", outerConditionIndex, innerConditionIndex, outerLockIndex, innerLockIndex);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
	//sprintf(data, "con sig %d %d", outerConditionIndex, outerLockIndex);
	//printf("Signalling Condition: %d with Lock: %d\n", outerConditionIndex, outerLockIndex);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
//	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in Signal: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in Signal: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in Signal: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in Signal: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in Signal: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in Signal: Not Lock owner\n", NOT_OWNER);
	}
	
    fflush(stdout);
	#endif
}

void ServerBroadcast_Syscall(int outerConditionIndex, int innerConditionIndex, int outerLockIndex, int innerLockIndex){
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char data[MaxMailSize];
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	sprintf(data, "con bro %d %d %d %d", outerConditionIndex, innerConditionIndex, outerLockIndex, innerLockIndex);
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = currentThread->mailboxID;
    outMailHdr.length = strlen(data) + 1;

	#ifdef NETWORK
    // Send the first message
	//sprintf(data, "con bro %d", outerConditionIndex, outerLockIndex);
	//printf("Broadcasting Condition: %d with Lock: %d\n", outerConditionIndex, outerLockIndex);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
//	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	int rv = ClientReceive();
	
	if (rv == BAD_FORMAT) {
		printf("Client: Error Code %d in Broadcast: Bad Format\n", BAD_FORMAT);
	} else if (rv == BAD_INDEX) {
		printf("Client: Error Code %d in Broadcast: Index out of range\n", BAD_INDEX);
	} else if (rv == NO_SPACE) {
		printf("Client: Error Code %d in Broadcast: Not enough space\n", NO_SPACE);
	} else if (rv == NOT_CREATED) {
		printf("Client: Error Code %d in Broadcast: Lock has not been created\n", NOT_CREATED);
	} else if (rv == DELETED) {
		printf("Client: Error Code %d in Broadcast: Lock has been deleted\n", DELETED);
	} else if (rv == NOT_OWNER) {
		printf("Client: Error Code %d in Broadcast: Not Lock owner\n", NOT_OWNER);
	}
	
    fflush(stdout);
	#endif
}

void Exit_Syscall(int status) {
	Process* process = currentThread->myProcess;
	// find the current process
	/*for(int i = 0; i < numProcesses; i++) {
		process = (Process*)processTable.Get(i);
		if(process->space == currentThread->space) {
			break;
		}
		else
			process = NULL;
	}*/
	if(process == NULL) {
		printf("Exit_Syscall: Main/initial thread.\n");
		currentThread->space->DeallocateProcess();
		currentThread->Finish();
	}
	if(process->numThreads == 1) {
	//If last thread in process
		if(numProcesses == 1) {
		//If last process
			process->space->DeallocateProcess();
			printf("Exit_Syscall: Last process\n");
			interrupt->Halt();
		}
		else {
			printf("Exit_Syscall: Last thread in process\n");
			numProcesses--;
			process->space->DeallocateProcess();
			processTable.Remove(process->processId);
			currentThread->Finish();
			//What do we do here?
			//How to deallocate memory from process?
		}
	}
	else {
		//Deallocate stack? How?
		process->numThreads--;
		process->space->DeallocateStack();
		printf("Exit_Syscall: Just a thread finishing.\n");
		currentThread->Finish();
	}
}

void exec_thread() {
	//printf("exec_thread running.\n");
	memoryLock->Acquire();
	currentThread->mailboxID = mailboxCounter;
	mailboxCounter++;
	//printf("Calling InitRegisters.\n");
	currentThread->space->InitRegisters();
	//printf("Calling RestoreState.\n");
	currentThread->space->RestoreState();
	memoryLock->Release();
	machine->Run();
}

SpaceId Exec_Syscall (unsigned int vaddr) {

	char *fileName;
	if ( !(fileName = new char[256]) ) {
		printf("%s","Error allocating kernel buffer for executing process!\n");
		return -1;
    } else {
        if ( copyin(vaddr,256, fileName) == -1 ) {
			printf("%s","Bad pointer passed to executing process\n");
			delete[] fileName;
			return -1; 
		}
    }
	
    OpenFile *f;
	
	//Now Open that file using filesystem->Open.
	//Store its openfile pointer.
    f = fileSystem->Open(fileName);
	
	if(f != NULL) {
		memoryLock->Acquire();
		//Create new addrespace for this executable file.
		//Create a new thread.
		Thread* t = new Thread(fileName);
		//Allocate the space created to this thread's space.
		//printf("New address space.\n");
		AddrSpace* space = new AddrSpace(f);
		//printf("New address space created.\n");
		Process* process = new Process;
		process->space = space;
		t->space = space;
		process->name = fileName;
		process->numThreads = 1;
		//Update the process table and related data structures.
		process->processId = processTable.Put(process);
		numProcesses++;
		t->myProcess = process;
		//Write the space ID to the register 2.
		machine->WriteRegister(2, process->processId);
		//Fork the new thread. I call it exec_thread.
		
		// need vaddr
		//printf("Allocating stack.\n");
		//t->space->AllocateStack();
		
		memoryLock->Release();
		printf("Exec_Syscall: Forking exec thread.\n");
		t->Fork((VoidFunctionPtr)exec_thread, NULL);		// exec_thread NOT RUNNING!
		//printf("Exec_Syscall: Forked thread.\n");
		return process->processId;
	}
	else {
		printf("Exec_Syscall: Couldn't open file.\n");
		return -1;
	}
}

void kernel_thread(unsigned int vaddr) {
	if (vaddr > 0) {
		//printf("kernel_thread: Starting allocation.\n");
		memoryLock->Acquire();
		
		currentThread->space->AllocateStack(vaddr);

		memoryLock->Release();
		//printf("About to run\n");
		machine->Run();
	} else {
		printf("kernel_thread: Bad virtual address.\n");
		return;
	}
}

void Fork_Syscall(unsigned int vaddr) {
	printf("\nFork_Syscall\n");
	if (vaddr > 0) {
		memoryLock->Acquire();
		if (numProcesses == 0) {
			printf("Fork_Syscall: There is no process for this thread to Fork to.\n");
			memoryLock->Release();
			return;
		}
		
		Process *tempProcess = currentThread->myProcess;
		//printf("NumProcesses:%d", numProcesses);
		//printf("\nFork_Syscall: Finding correct process...\n");
		/*for(int i = 0; i < numProcesses; i++) {
			tempProcess = (Process*)processTable.Get(i);
			if(tempProcess->space == currentThread->space) {
				//printf("Found correct process.\n");
				break;
			}
		}*/
		//printf("Fork_Syscall: Making new thread\n");
		Thread *t = new Thread("name");
		//printf("Fork_Syscall: New thread created successfully!\n");
		t->space = tempProcess->space;
		//printf("1\n");
		t->myProcess = tempProcess;
		tempProcess->numThreads++;
		//printf("2\n");
		//printf("Number of threads: %d\n", tempProcess->numThreads);
		memoryLock->Release();
		//printf("Calling t->Fork()\n");
		t->Fork((VoidFunctionPtr)kernel_thread, vaddr);
		return;
	}
	else {
		printf("Fork_Syscall: Bad virtual address.\n");
		return;
	}
}

int Random_Syscall(int max) {
	int value = rand() % max;
	return value;
}

void Trace_Syscall(unsigned int vaddr, int val) {
    
    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output
	int len = 128;	// buffer size
    
    if ( !(buf = new char[len]) ) {
		printf("%s","Error allocating kernel buffer for write!\n");
		return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed to to write: data not written\n");
			delete[] buf;
			return;
		}
    }
	
	for (int ii = 0; ii < len; ii++) {
		if (buf[ii] == 0x00) {	// at end of input chars
			if (val == 0x9999) {	// val = "nothing"
				break;
			}
			printf("%d", val);
			break;
		}
		printf("%c", buf[ii]);
	}

    delete[] buf;
}

void Yield_Syscall() {
	currentThread->Yield();
}

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Which syscall?
    int rv=0; 	// the return value from a syscall

    if ( which == SyscallException ) {
	switch (type) {
	    default:
		DEBUG('a', "Unknown syscall - shutting down.\n");
	    case SC_Halt:
		DEBUG('a', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
		break;
	    case SC_Create:
		DEBUG('a', "Create syscall.\n");
		Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_Open:
		DEBUG('a', "Open syscall.\n");
		rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_Write:
		DEBUG('a', "Write syscall.\n");
		Write_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
	    case SC_Read:
		DEBUG('a', "Read syscall.\n");
		rv = Read_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
	    case SC_Close:
		DEBUG('a', "Close syscall.\n");
		Close_Syscall(machine->ReadRegister(4));
		break;
		case SC_Yield:
		DEBUG('a', "Yield syscall.\n");
		Yield_Syscall();
		break;
		case SC_Exit:
		DEBUG('a', "Exit syscall.\n");
		Exit_Syscall(machine->ReadRegister(4));
		break;
		case SC_Exec:
		DEBUG('a', "Exec syscall.\n");
		rv = Exec_Syscall(machine->ReadRegister(4));
		break;
		case SC_Fork:
		DEBUG('a', "Fork syscall.\n");
		Fork_Syscall(machine->ReadRegister(4));
		break;
		case SC_Acquire:
		DEBUG('a', "Acquire syscall.\n");
		Acquire_Syscall(machine->ReadRegister(4));
		break;
		case SC_Release:
		DEBUG('a', "Release syscall.\n");
		Release_Syscall(machine->ReadRegister(4));
		break;
		case SC_Wait:
		DEBUG('a', "Wait syscall.\n");
		Wait_Syscall(machine->ReadRegister(4), 
					machine->ReadRegister(5));
		break;
		case SC_Signal:
		DEBUG('a', "Signal syscall.\n");
		Signal_Syscall(machine->ReadRegister(4),
					machine->ReadRegister(5));
		break;
		case SC_Broadcast:
		DEBUG('a', "Broadcast syscall.\n");
		Broadcast_Syscall(machine->ReadRegister(4),
						machine->ReadRegister(5));
		break;
		case SC_CreateLock:
		DEBUG('a', "Create lock syscall.\n");
		rv = CreateLock_Syscall(machine->ReadRegister(4),
							machine->ReadRegister(5));
		break;
		case SC_DestroyLock:
		DEBUG('a', "Destroy lock syscall.\n");
		rv = DestroyLock_Syscall(machine->ReadRegister(4));
		break;
		case SC_CreateCondition:
		DEBUG('a', "Create condition syscall.\n");
		rv = CreateCondition_Syscall(machine->ReadRegister(4),
									machine->ReadRegister(5));
		break;
		case SC_DestroyCondition:
		DEBUG('a', "Destroy condition syscall.\n");
		rv = DestroyCondition_Syscall(machine->ReadRegister(4));
		break;
		case SC_Random:
		DEBUG('a', "Random number syscall.\n");
			rv = Random_Syscall(machine->ReadRegister(4));
		break;
		
		case SC_Trace:
		DEBUG('a', "Trace syscall.\n");
		Trace_Syscall(machine->ReadRegister(4),
					machine->ReadRegister(5));
		break;
		
		case SC_CreateMV:
		DEBUG('a', "Create MV syscall.\n");
			rv = CreateMV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6), machine->ReadRegister(7));
		break;
		case SC_GetMV:
		DEBUG('a', "Get MV syscall.\n");
			rv = GetMV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
		case SC_SetMV:
		DEBUG('a', "Set MV syscall.\n");
			SetMV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
		break;
		case SC_ServerCreateLock:
		DEBUG('a', "Server Create Lock syscall.\n");
			rv = ServerCreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
		break;
		case SC_ServerDestroyLock:
		DEBUG('a', "Server Destroy Lock syscall.\n");
			ServerDestroyLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
		case SC_ServerAcquire:
		DEBUG('a', "Server Acquire syscall.\n");
			ServerAcquire_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
		case SC_ServerRelease:
		DEBUG('a', "Server Release syscall.\n");
			ServerRelease_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
		case SC_ServerCreateCV:
		DEBUG('a', "Server Create CV syscall.\n");
			rv = ServerCreateCV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
		break;
		case SC_ServerDestroyCV:
		DEBUG('a', "Server Destroy CV syscall.\n");
			ServerDestroyCV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
		case SC_ServerWait:
		DEBUG('a', "Server Wait syscall.\n");
			ServerWait_Syscall(machine->ReadRegister(4),  machine->ReadRegister(5), machine->ReadRegister(6), machine->ReadRegister(7));
		break;
		case SC_ServerSignal:
		DEBUG('a', "Server Signal syscall.\n");
			ServerSignal_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6), machine->ReadRegister(7));
		break;
		case SC_ServerBroadcast:
		DEBUG('a', "Server Broadcast syscall.\n");
		ServerBroadcast_Syscall(machine->ReadRegister(4),  machine->ReadRegister(5), machine->ReadRegister(6), machine->ReadRegister(7));
		break;
	}

	// Put in the return value and increment the PC
	machine->WriteRegister(2,rv);
	machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
	machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
	machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
	return;
    } else {
      cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      interrupt->Halt();
    }
}
