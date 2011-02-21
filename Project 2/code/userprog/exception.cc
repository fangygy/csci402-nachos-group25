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
#include <stdio.h>
#include <iostream>

using namespace std;

#define MAX_LOCKS 100
#define MAX_CONDITIONS 100
int numLocks = 0;
int numConditions = 0;

Lock* kernelLock = new Lock("kernelLock");

struct KernelLock {
	Lock* lock;
	AddrSpace* space;
	bool beingAcquired;
	bool isToBeDeleted;
	bool hasBeenDeleted;
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
		lock = NULL;
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

int CreateLock(char* name, int length) {
	// axe crowley
	kernelLock-> Acquire();
	if (numLocks >= MAX_LOCKS) {
		// print error msg?
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
	
	locks[index].lock = new Lock(name);
	locks[index].space = currentThread->space;		// double-check
	locks[index].beingAcquired = false;
	locks[index].isToBeDeleted = false;
	locks[index].deleted = false;
	numLocks ++;
	
	kernelLock-> Release();
	return (index);
}

int CreateCondition(char* name, int length) {
	kernelLock-> Acquire();
	if (numConditions >= MAX_CONDITIONS) {
		// print error msg?
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
	
	conditions[index].condition = new Condition(name);
	conditions[index].space = currentThread->space;		// double-check
	conditions[index].isToBeDeleted = false;
	conditions[index].deleted = false;
	numConditions ++;
	
	kernelLock-> Release();
	return (index);
}

int DestroyLock(int index) {
	kernelLock-> Acquire();
	// check on return value
	if (index < 0) {
		kernelLock->Release();
		// print error msg
		return 0;
	}
	if (locks[index].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		kernelLock->Release();
		return 0;
	}
	if (locks[index].isToDeleted || locks[index].deleted) {
		// Delete has already been called for this lock. don't do anything
		kernelLock->Release();
		return 0;
	}
	if (locks[index].lock->getFree() && !locks[index].beingAcquired) {
		// Lock isn't in use; delete it
		delete locks[index].lock;
		locks[index].lock = NULL;			// nullify lock pointer; this is now a free space
		locks[index].space = NULL;			// make the address space null
		locks[index].isToBeDeleted = false;
		locks[index].deleted = true;
		numLocks --;
	} else {
		// Lock is still in use; will delete it later
		locks[index].isToBeDeleted = true;
	}
	
	kernelLock->Release();
	return 1;
}

int DestroyCondition(int index) {
	kernelLock-> Acquire();
	
	if (conditions[index].deleted || conditions[index].deleted) {
		// Delete has already been called for this condition. don't do anything
		kernelLock->Release();
		return 0;
	}
	if (conditions[index].condition->getFree() ) {
		// Condition isn't in use; delete it
		conditions[index].condition-> ~Condition();
		conditions[index].condition = NULL;		// nullify condition pointer; this is now a free space
		conditions[index].space = NULL;			// make the address space null
		conditions[index].isToBeDeleted = false;
		conditions[index].deleted = true;
		numConditions --;
	} else {
		// Condition is still in use; will delete it later
		conditions[index].isToBeDeleted = true;
	}
	
	kernelLock->Release();
	return 1;
}

void Acquire(int index){
	kernelLock->Acquire();
	if (index < 0) {
		// print error msg
		kernelLock->Release();
		return 0;
	}
	if (locks[index].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		kernelLock->Release();
		return 0;
	}
	if(locks[index].deleted){
		// Lock does not exist
		kernelLock->Release();
		return;
	}
	//
	if(locks[index].isToBeDeleted){
		// Lock is going to be deleted, no further action permitted
		kernelLock->Release();
		return;
	}

	locks[index].beingAcquired = true;
	kernelLock->Release();
	locks[index].lock->Acquire();
	locks[index].beingAcquired = false;
}

void Release(int index){
	kernelLock->Acquire();
	if (index < 0) {
		// print error msg
		kernelLock->Release();
		return 0;
	}
	if (locks[index].space != currentThread->space) {
		// wrong address space, foo
		// print error msg
		kernelLock->Release();
		return 0;
	}
	if(locks[index].deleted){
		// Lock does not exist
		kernelLock->Release();
		return;
	}

	locks[index].lock->Release();
	if(locks[index].lock->getFree() && locks[index].isToBeDeleted && !locks[index].beingAcquired){
		locks[index].deleted = true;
		locks[index].isToBeDeleted = false;
		delete locks[index].lock;
		numLocks--;
	}

	kernelLock->Release();
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
		printf("ExceptionHandler: Halting...\n");
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
		printf("ExceptionHandler: Yielding...\n");
		currentThread->Yield();
		break;
		case SC_Exit:
		DEBUG('a', "Exit syscall.\n");
		
		break;
		case SC_Exec:
		DEBUG('a', "Exec syscall.\n");
		
		break;
		case SC_Fork:
		DEBUG('a', "Fork syscall.\n");
		
		break;
		case SC_Acquire:
		DEBUG('a', "Acquire syscall.\n");
		printf("ExceptionHandler: Acquiring...\n");	
		break;
		case SC_Release:
		DEBUG('a', "Release syscall.\n");
		printf("ExceptionHandler: Releasing...\n");	
		break;
		case SC_Wait:
		DEBUG('a', "Wait syscall.\n");

		break;
		case SC_Signal:
		DEBUG('a', "Signal syscall.\n");
		
		break;
		case SC_Broadcast:
		DEBUG('a', "Broadcast syscall.\n");
		
		break;
		case SC_CreateLock:
		DEBUG('a', "Create lock syscall.\n");
		printf("ExceptionHandler: Creating lock...\n");	
		break;
		case SC_DestroyLock:
		DEBUG('a', "Destroy lock syscall.\n");
		printf("ExceptionCallHandler: Destroying lock...\n");		
		break;
		case SC_CreateCondition:
		DEBUG('a', "Create condition syscall.\n");
		
		break;
		case SC_DestroyCondition:
		DEBUG('a', "Destroy condition syscall.\n");
		
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
