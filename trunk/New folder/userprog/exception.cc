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
#include <stdio.h>
#include <iostream>

using namespace std;

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

int copyinint(unsigned int vaddr, int len, int *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 4, paddr );
      while(!result) // FALL 09 CHANGES
	  {
   			result = machine->ReadMem( vaddr, 4, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
	  }	
      
      buf[n++] = *paddr;
     
      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr+=4;
    }

    delete paddr;
    return len;
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

void Yield_Syscall(){
	DEBUG('u', "\nYielding thread %s.\n",currentThread->getName());
	currentThread->Yield();
	return;
}

void exec_thread(){
        //start a new process
        //acquire exec lock
        execLock->Acquire();
        currentThread->space->InitRegisters(); //init gp reg, and tablereg values, etc
        currentThread->space->RestoreState();
        execLock->Release();
        DEBUG('u',"\nExec_thread running thread %s.\n",currentThread->getName());
        machine->Run();         //now the program is running- shouldn't exit until syscall
        ASSERT(FALSE);//should never happen
        
}

SpaceId Exec_Syscall(int vaddr){
	//syscallLock->Acquire();
	OpenFile* execFile;
	Thread* t;
	char* filename = new char [MAX_FILENAME];
	
	if(copyin(vaddr,MAX_FILENAME,filename)==-1){
		DEBUG('u',"\nCOULDN'T SET THE FILENAME FROM VADDR %d!\n",vaddr);
		delete[] filename;
		//syscallLock->Release();
		return -1;
	}
	
	execFile = fileSystem->Open(filename);
	
	if(execFile){
		syscallLock->Acquire();
		AddrSpace *space = new AddrSpace(execFile);
		DEBUG('u',"Executing file %s.\n",filename);
		Process* newProcess = new Process;
		//zero out owned locks and conditions
		for(int i=0;i<512;i++){
			newProcess->locksOwned[i]=false;
			newProcess->conditionsOwned[i]=false;
			newProcess->mvsOwned[i]=false;
		}
		char* name = new char[64];
		char* id = new char[4];
		strcpy(name,filename);

		newProcess->name=filename;
		newProcess->space = space;
		newProcess->numThreads = 1;
		newProcess->processId = processTable.Put(newProcess);
		t = new Thread(strcat(name,id));
		t->process= newProcess;
		syscallLock->Release();
		t->space = t->process->space;	
		//new network mailbox addition
		t->mailbox=nextMailbox++;
		//printf("nextMailbox = %d\n",nextMailbox);
		
		/* COMMENTING OUT FOR PROJECT 3 PART 1 STEP 3
		// PROJECT 3 PART 1 STEP 2
		//populate ipt
		t->space->PopulateIPT(t->process->processId);
		*/
		machine->WriteRegister(2, t->process->processId);
		t->Fork((VoidFunctionPtr)exec_thread, NULL);
		delete id;
		return t->process->processId;
	}
	else{
		DEBUG('u',"COULDN'T EXECUTE FILE %s!\n",filename);
		return -1;
	}
}



void Exit_Syscall(int status){
	/* Cases:
	 *	Thread is part of a process with other threads running	-> Kill thread only
	 *	Thread is last thread in a process			-> Kill thread, kill process
	 *	Thread is the last thread in the last process		-> Kill thread, kill process, kill everything
	 */
	syscallLock->Acquire();
	currentThread->process->numThreads--;
	//if there are now no threads on the process, kill the process
	if(currentThread->process->numThreads==0){
		//relinquish my stack, give up all the code/data pages of this process
		//and remove myself from the process table
		currentThread->space->DeallocateStack();
		currentThread->space->DeallocateProcess();
		processTable.Remove(currentThread->process->processId);
		if(processTable.GetSize()==0){//this was the last process
			syscallLock->Release();
			DEBUG('u',"\nExit syscall: all processes have terminated, calling halt!\n\n");
			interrupt->Halt();
		}
		else{//this was the last thread in the process, but not the last process in NACHOS
			syscallLock->Release();
			currentThread->Finish();
		}		
	}
	else if(currentThread->process->numThreads>0){//we were one of many processes in our thread
		currentThread->space->DeallocateStack();
		syscallLock->Release();
		currentThread->Finish();
	}
	//shouldn't be reached
	return;	   
}

void kernel_thread(int virtualaddress){
	DEBUG('u',"\nKernel thread forking vaddr %d.\n",virtualaddress);
        //ACQUIRE a lock to prevent other threads from
	//changing addrSpace shared data
        forkLock->Acquire();
	//allocate 8 pages of stack
	currentThread->userStackStart=currentThread->space->AllocateStack(currentThread->process->processId);	
	forkLock->Release();
        //write to registers as per student documentation
        machine->WriteRegister(PCReg, virtualaddress);
        machine->WriteRegister(NextPCReg, virtualaddress+4);
        currentThread->space->RestoreState();//inorder to prevent information loss while context switching.
        machine->WriteRegister(StackReg, currentThread->userStackStart);
        //RELEASE LOCK HERE
       // forkLock->Release();
        machine->Run();
        ASSERT(FALSE); //this should never be reached
}

void Fork_Syscall(int vaddr){
	DEBUG('u', "\nForking thread %s with vaddr %d and header code limit %d.\n",currentThread->getName(),vaddr,currentThread->space->headerCodeLimit);
        //ACQUIRe system lock
        syscallLock->Acquire();
	char* name = new char[64];
	strcpy(name,currentThread->process->name);
	strcat(name,"_Thread");
	char* number=new char[8];
	//syscallLock->Acquire();
	sprintf(number,"%d",currentThread->process->numThreads);
	strcat(name,number);
	//cout<<name<<endl;
        Thread *t = new Thread(name);//new thread for forkin'
	//delete name;
	delete number;
	//cout<<"thread's name is "<<t->getName()<<endl;
	//cout<<"the forkin' vaddr is :"<<vaddr<<endl;
        if(0<vaddr && vaddr<currentThread->space->headerCodeLimit){
		t->process = currentThread->process;
		t->process->numThreads++;
		t->space = t->process->space;
		//new network addition
		t->mailbox=nextMailbox++;
                //RELEASE LOCK
                syscallLock->Release();
                t->Fork((VoidFunctionPtr)kernel_thread,vaddr);
        }
        else{
                //RELEASE LOCK
                syscallLock->Release();
                DEBUG('u',"\nBAD VIRTUAL ADDRESS IN FORK!\n");
        }
	return;
}

void Acquire_Syscall(int lid){
	if(lid >= 0 && lid <= 512){
	        lockLock->Acquire();
	        if(currentThread->process->locksOwned[lid] == true){
	        	DEBUG('u',"\nAcquire syscall called by thread %s acquiring lock %s, using id %d.\n",currentThread->getName(),((Lock*)lockTable.Get(lid))->getName(),lid);
	                ((Lock*)(lockTable.Get(lid)))->Acquire(lockLock);
	        }
	        else{
	        	lockLock->Release();
	        	DEBUG('u',"\nAcquire syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),lid);
	        }
	}
	else{
		DEBUG('u',"\nAcquire syscall: INVALID LOCK ID!\n");
	}
	return;
}

void Release_Syscall(int lid){
	if(lid >= 0 && lid <= 512){
	        lockLock->Acquire();
	        if(currentThread->process->locksOwned[lid] == true){
	        	DEBUG('u',"\nRelease syscall called by thread %s acquiring lock %s, using id %d.\n",currentThread->getName(),((Lock*)lockTable.Get(lid))->getName(),lid);
	                Lock* temp = (Lock*)(lockTable.Get(lid));
	                temp->Release();
	                if(temp->isFree() && temp->isMarked()){
	                        delete (Lock*)lockTable.Remove(lid);
	                        currentThread->process->locksOwned[lid] = false;
	                }
	                
	        }
	        else{
	                DEBUG('u',"\nRelease syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),lid);  
	        }
	        lockLock->Release();
	}
	else{
		DEBUG('u',"\nRelease syscall: INVALID LOCK ID!\n");
	}
        return;
}

void Wait_Syscall(int cid, int lid){
	if(cid >= 0 && cid <= 512 && lid >=0 && lid <= 512){
	        condLock->Acquire();
	        if(currentThread->process->locksOwned[lid] == true && currentThread->process->conditionsOwned[cid] == true){
			DEBUG('u',"\nWait syscall called by thread %s signalling on condition %s with id %d using lock %s with id %d for process %d.\n",currentThread->getName(),((Condition*)conditionTable.Get(cid))->getName(),cid, ((Lock*)lockTable.Get(lid))->getName(),lid,currentThread->process->processId);
	                ((Condition*)conditionTable.Get(cid))->Wait((Lock*)lockTable.Get(lid), condLock);
	        }
	        else{
	        	condLock->Release();
	                DEBUG('u',"\nWait syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF EITHER THE CONDITION WITH ID %d OR THE LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),cid, lid);
	        }
	}
	else{
		DEBUG('u',"\nWait syscall: INVALID LOCK OR CONDITION ID!\n");
	}	
}

void Signal_Syscall(int cid, int lid){
	if(cid >= 0 && cid <= 512 && lid >=0 && lid <= 512){
		condLock->Acquire();
		if(currentThread->process->locksOwned[lid] == true && currentThread->process->conditionsOwned[cid] == true){
			DEBUG('u',"\nSignal syscall called by thread %s signalling on condition %s with id %d using lock %s with id %d for process %d.\n",currentThread->getName(),((Condition*)conditionTable.Get(cid))->getName(),cid, ((Lock*)lockTable.Get(lid))->getName(),lid,currentThread->process->processId);
		        Condition *tempC = (Condition*)conditionTable.Get(cid);
		        Lock *tempL = (Lock*)lockTable.Get(lid);
		        tempC->Signal(tempL);
		        if(tempC->isFree() && tempC->isMarked()){
		                delete (Condition*)conditionTable.Remove(cid);
		                currentThread->process->conditionsOwned[cid] = false;
		        }
		}
		else{
	                DEBUG('u',"\nSignal syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF EITHER THE CONDITION WITH ID %d OR THE LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),cid, lid);	
		}
	        condLock->Release();
	}
	else{
		DEBUG('u',"\nSignal syscall: INVALID LOCK OR CONDITION ID!\n");
	}
}

void Broadcast_Syscall(int cid, int lid){
	if(cid >= 0 && cid <= 512 && lid >=0 && lid <= 512){
		condLock->Acquire();
		if(currentThread->process->locksOwned[lid] == true && currentThread->process->conditionsOwned[cid] == true){
			DEBUG('u',"\nBroadcast syscall called by thread %s signalling on condition %s with id %d using lock %s with id %d for process %d.\n",currentThread->getName(),((Condition*)conditionTable.Get(cid))->getName(),cid, ((Lock*)lockTable.Get(lid))->getName(),lid,currentThread->process->processId);
		        Condition *tempC = (Condition*)conditionTable.Get(cid);
		        Lock *tempL = (Lock*)lockTable.Get(lid);
		        tempC->Broadcast(tempL);
		        if(tempC->isFree() && tempC->isMarked()){
		                delete (Condition*)conditionTable.Remove(cid);
		                currentThread->process->conditionsOwned[cid] = false;
		        }
		}
		else{
			DEBUG('u',"\nBroadcast syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF EITHER THE CONDITION WITH ID %d OR THE LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),cid, lid);	
		}
	        condLock->Release();
	}
	else{
		DEBUG('u',"\nBroadcast syscall: INVALID LOCK OR CONDITION ID!\n");
	}
}

int CreateLock_Syscall(int va){
	if(va != NULL){
		char* debugName;
		debugName = new char[64];
		copyin(va,64,debugName);
		Lock* temp = new Lock(debugName);
		int id = lockTable.Put(temp);
		if(id != -1){
			DEBUG('u',"\nCreateLock syscall called by thread %s creating lock %s, putting it in the lock table with id %d, and making process %d the owner.\n",currentThread->getName(),debugName,id,currentThread->process->processId);
			currentThread->process->locksOwned[id] = true;
		} 
		else {
			DEBUG('u',"\nCreateLock syscall: COULD NOT CREATE LOCK %s BECAUSE THE LOCK TABLE IS FULL!\n",debugName);
			delete temp;
			return -1;
		}
		return id;
	}
	else{//lock name was null
		DEBUG('u',"\nCreateLock syscall: NULL POINTER PASSED FOR LOCK NAME!\n");
		return -1;
	}
	
}

void DestroyLock_Syscall(int lid){
	if(lid >= 0 && lid <= 512){
	        lockLock->Acquire();
	        if(currentThread->process->locksOwned[lid] == true){
	                if(((Lock*)lockTable.Get(lid))->isFree()){
	                		DEBUG('u',"\nDestroyLock syscall called by thread %s destroying lock %s with id %d for process %d.\n",currentThread->getName(),((Lock*)lockTable.Get(lid))->getName(),lid,currentThread->process->processId);
	                        delete (Lock*)lockTable.Remove(lid);
	                        currentThread->process->locksOwned[lid] = false;
	                } else{
	                		DEBUG('u',"\nDestroyLock syscall called by thread %s marking for deletion lock %s with id %d for process %d.\n",currentThread->getName(), ((Lock*)lockTable.Get(lid))->getName(), lid, currentThread->process->processId);
	                        ((Lock*)lockTable.Get(lid))->markForDeletion();
	                }
	                
	        }
	        else{
	                DEBUG('u',"\nDestroyLock syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),lid);
	        }
	        lockLock->Release();
	        return;
	}
	else{
		DEBUG('u',"\nDestroyLock syscall: INVALID LOCK ID!\n");
		return;
	}
}

int CreateCondition_Syscall(int va){
	if(va != NULL){
	        char* debugName;
	        debugName = new char[64];
	        copyin(va,64,debugName);
	        Condition *temp = new Condition(debugName);
	        int id = conditionTable.Put(temp);
	        if(id != -1){
	        		DEBUG('u',"\nCreateCondition syscall called by thread %s creating condition %s, putting it in the condition table with id %d, and making process %d the owner.\n",currentThread->getName(),debugName,id,currentThread->process->processId);
	                currentThread->process->conditionsOwned[id] = true;
	        }
	        else{
	        		DEBUG('u',"\nCreateCondition syscall: COULD NOT CREATE CONDITION %s BECAUSE THE CONDITION TABLE IS FULL!\n",debugName);
	                delete temp;
	        }
	        return id;
	}
	else{//we were passed a bad string
		DEBUG('u',"\nCreatCondition syscall: NULL POINTER PASSED FOR CONDITION NAME!\n");
		return -1;
	}
}

void DestroyCondition_Syscall(int cid){
	if(cid >= 0 && cid <= 512){
		condLock->Acquire();
		if(currentThread->process->conditionsOwned[cid] == true){
			if(((Condition*)conditionTable.Get(cid))->isFree()){
				DEBUG('u',"\nDestroyCondition syscall called by thread %s destroying condition %s with id %d for process %d.\n",currentThread->getName(),((Condition*)conditionTable.Get(cid))->getName(),cid,currentThread->process->processId);
				delete (Condition*)conditionTable.Remove(cid);
				currentThread->process->conditionsOwned[cid] = true;
			} else{
				DEBUG('u',"\nDestroyCondition syscall called by thread %s marking for deletion condition %s with id %d for process %d.\n",currentThread->getName(), ((Condition*)conditionTable.Get(cid))->getName(), cid, currentThread->process->processId);
				((Condition*)conditionTable.Get(cid))->markForDeletion();
			}
		}
		else{
			DEBUG('u',"\nDestroyCondition syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF CONDITION WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),cid);
		}
		condLock->Release();
	}
	else{
		DEBUG('u',"\nDestroyCondition syscall: INVALID CONDITION ID!\n");
	}
}

void Print_Syscall(unsigned int vaddr, int len, unsigned int args, int argsSize) {
    // Print string of length len to synchronized console including 
    // the integer arguments supplied in the args array. 
    printLock->Acquire();
	char* c_buf;
    c_buf=new char[len];
    copyin(vaddr,len,c_buf);
    
    int* i_buf;
    i_buf = new int[argsSize];
    copyinint(args,argsSize,i_buf);
    
    switch(argsSize){
		case 0:
        	printf(c_buf);
        	break;
	case 1:
    		printf(c_buf,*i_buf);
    		break;
    	case 2:
    		printf(c_buf,*i_buf,*(i_buf+1));
    		break;
    	case 3:
    		printf(c_buf,*i_buf,*(i_buf+1),*(i_buf+2));
    		break;
    	case 4:
    		printf(c_buf,*i_buf,*(i_buf+1),*(i_buf+2),*(i_buf+3));
    		break;
    }
    delete c_buf;
    delete i_buf;
    printLock->Release();

}

int Rand_Syscall(){
	return rand();
}

int CreateServerLock_Syscall(int va, int server){
	//first check to see if the name of the about to be created lock is null
	//if it is automatically return a garbage value, if it's not, then send a 
	//message to the server, which will reply with a lock id, if this id is 
	//valid we add it to our list of owned locks
	DEBUG('u',"\nCreateServerLock syscall CALLED by thread %s with va %d\n",currentThread->getName(),va);
	if(va != NULL){
		char* debugName = new char[64];
		copyin(va,64,debugName);
				 
		PacketHeader outPktHdr, inPktHdr;
		MailHeader outMailHdr, inMailHdr;
		char buffer[MaxMailSize] = "";
		bool success;
		char data[MaxMailSize] = "RT = CSL NA = ";
		strcat(data,debugName);
		// construct packet, mail header for original message
		// To: destination machine, mailbox 0
		// From: our machine, reply to: mailbox 1
		if(server==-1){
			outPktHdr.to = rand()%numServers;		
			outPktHdr.to = rand()%numServers;
		}			
		else
			outPktHdr.to = server;
		outMailHdr.to = 0;
		outMailHdr.from = currentThread->mailbox;    
		outMailHdr.length = strlen(data) + 1;
   		DEBUG('u',"CreateServerLock syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
		// create & send a request msg to the server  
		success = postOffice->Send(outPktHdr, outMailHdr, data); 

		if ( !success ) {
			DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		
		// wait for the reply msg
		// return (or not) a value to the user program
		DEBUG('u',"CreateServerLock syscall called by thread %s Waiting...\n",currentThread->getName());
		postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
		outPktHdr.to = inPktHdr.from;
   		outMailHdr.to = inMailHdr.from;
		DEBUG('u',"CreateServerLock syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);
		
		int id;
		sscanf(buffer,"%*s %*c %d", &id);
		
		if(id != -1){
			DEBUG('u',"CreateServerLock syscall called by thread %s either created server lock %s (and put it in the server lock table with id %d) or the server lock already existed, and process %d is made the owner.\n",currentThread->getName(),debugName,id,currentThread->process->processId);
			currentThread->process->locksOwned[id] = true;
		} 
		else {
			DEBUG('u',"CreateServerLock syscall: COULD NOT CREATE SERVER LOCK %s BECAUSE THE SERVER LOCK TABLE IS FULL!\n",debugName);
			return -1;
		}
		
		return id;
	}
	else{//lock name was null
		DEBUG('u',"\nCreateServerLock syscall: NULL POINTER PASSED FOR LOCK NAME!\n");
		return -1;
	}	
}

void DestroyServerLock_Syscall(int lid, int server){
	//first validate that lid is in range and owned by this thread/process
	//if it isn't, automatically return -1, if it is then send a message to the
	//server and parse the message to destroy the lock, once we receive confirmation,
	//we remove the lock from our list of owned locks
	DEBUG('u',"\nDestroyServerLock syscall CALLED by thread %s with lid %d\n",currentThread->getName(),lid);
	if(lid >= 0 && lid <= numServers*1000){
		//lockLock->Acquire();
		if(currentThread->process->locksOwned[lid] == true){				
			PacketHeader outPktHdr, inPktHdr;
			MailHeader outMailHdr, inMailHdr;
			char buffer[MaxMailSize] = "";
			bool success;
			char data[MaxMailSize] = "RT = DSL LID = ";
			char num[32];
			sprintf(num,"%d",lid);
			strcat(data,num);

			// construct packet, mail header for original message
			// To: destination machine, mailbox 0
			// From: our machine, reply to: mailbox 1
			if(server==-1){
				outPktHdr.to = rand()%numServers;		
				outPktHdr.to = rand()%numServers;
			}			
			else
				outPktHdr.to = server;		
			outMailHdr.to = 0;
			outMailHdr.from = currentThread->mailbox;    
			outMailHdr.length = strlen(data) + 1;
			DEBUG('u',"DestroyServerLock syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
			// create & send a request msg to the server  
			success = postOffice->Send(outPktHdr, outMailHdr, data); 

			if ( !success ) {
			DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
			}

			// wait for the reply msg
			// return (or not) a value to the user program
			DEBUG('u',"DestroyServerLock syscall called by thread %s Waiting...\n",currentThread->getName());
			postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			DEBUG('u',"DestroyServerLock syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
			fflush(stdout);

			int id;
			sscanf(buffer,"%*s %*c %d", &id);
			
			if(id == 1){
				DEBUG('u',"DestroyServerLock syscall called by thread %s has destroyed server lock with id %d for process %d.\n",currentThread->getName(),lid,currentThread->process->processId);
				currentThread->process->locksOwned[lid] = false;
			} else if(id == 0){
				DEBUG('u',"DestroyServerLock syscall called by thread %s has marked for deletion server lock with id %d for process %d.\n",currentThread->getName(),lid,currentThread->process->processId);
			} else if(id == -1){
				DEBUG('u',"DestroyServerLock syscall called by thread %s has passed an invalid server lock with id %d for process %d.\n",currentThread->getName(),lid,currentThread->process->processId);
			}
		}
        else{
                DEBUG('u',"DestroyServerLock syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),lid);
        }
        //lockLock->Release();
        return;
	}
	else{
		DEBUG('u',"\nDestroyServerLock syscall: INVALID LOCK ID!\n");
		return;
	}
}

int CreateServerCondition_Syscall(int va, int server){
	//first check to see if the name of the about to be created condition is null
	//if it is automatically return a garbage value, if it's not, then send a 
	//message to the server, which will reply with a condition id, if this id is 
	//valid we add it to our list of owned conditions
	DEBUG('u',"\nCreateServerCondition syscall CALLED by thread %s with va %d\n",currentThread->getName(),va);
	if(va != NULL){
		char* debugName = new char[64];
		copyin(va,64,debugName);
					 
		PacketHeader outPktHdr, inPktHdr;
    	MailHeader outMailHdr, inMailHdr;
    	char buffer[MaxMailSize] = "";
    	bool success;
    	char data[MaxMailSize] = "RT = CSC NA = ";
    	strcat(data,debugName);
    	
		// construct packet, mail header for original message
		// To: destination machine, mailbox 0
		// From: our machine, reply to: mailbox 1
		if(server==-1){
			outPktHdr.to = rand()%numServers;		
			outPktHdr.to = rand()%numServers;
		}			
		else
			outPktHdr.to = server;	
		outMailHdr.to = 0;
		outMailHdr.from = currentThread->mailbox;    
		outMailHdr.length = strlen(data) + 1;
   		DEBUG('u',"CreateServerCondition syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
		// create & send a request msg to the server  
		success = postOffice->Send(outPktHdr, outMailHdr, data); 

		if ( !success ) {
			DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		
		// wait for the reply msg
		// return (or not) a value to the user program
		DEBUG('u',"CreateServerCondition syscall called by thread %s Waiting...\n",currentThread->getName());
		postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
		outPktHdr.to = inPktHdr.from;
   		outMailHdr.to = inMailHdr.from;
		DEBUG('u',"CreateServerCondition syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);
		
		int id;
		sscanf(buffer,"%*s %*c %d", &id);
		
		if(id != -1){
			DEBUG('u',"CreateServerCondition syscall called by thread %s either created server condition %s (and put it in the server condition table with id %d) or the server condition already existed, and process %d is made the owner.\n",currentThread->getName(),debugName,id,currentThread->process->processId);
			currentThread->process->conditionsOwned[id] = true;
		} 
		else {
			DEBUG('u',"CreateServerCondition syscall: COULD NOT CREATE SERVER CONDITION %s BECAUSE THE SERVER CONDITION TABLE IS FULL!\n",debugName);
			return -1;
		}
		
		return id;
	}
	else{//lock name was null
		DEBUG('u',"\nCreateServerCondition syscall: NULL POINTER PASSED FOR CONDITION NAME!\n");
		return -1;
	}
}

void DestroyServerCondition_Syscall(int cid, int server){
	//first validate that cid is in range and owned by this thread/process
	//if it isn't, automatically return -1, if it is then send a message to the
	//server and parse the message to destroy the condition, once we receive confirmation,
	//we remove the condition from our list of owned conditions
	DEBUG('u',"\nDestroyServerCondition syscall CALLED by thread %s with cid %d\n",currentThread->getName(),cid);
	if(cid >= 0 && cid%1000 <= 512){
        //condLock->Acquire();
        if(currentThread->process->conditionsOwned[cid] == true){				
			PacketHeader outPktHdr, inPktHdr;
			MailHeader outMailHdr, inMailHdr;
			char buffer[MaxMailSize] = "";
			bool success;
			char data[MaxMailSize] = "RT = DSC CID = ";
			char num[32];
			sprintf(num,"%d",cid);
			strcat(data,num);

			// construct packet, mail header for original message
			// To: destination machine, mailbox 0
			// From: our machine, reply to: mailbox 1
			if(server==-1){
				outPktHdr.to = rand()%numServers;		
				outPktHdr.to = rand()%numServers;
			}			
			else
				outPktHdr.to = server;		
			outMailHdr.to = 0;
			outMailHdr.from = currentThread->mailbox;    
			outMailHdr.length = strlen(data) + 1;
			DEBUG('u',"DestroyServerCondition syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
			// create & send a request msg to the server  
			success = postOffice->Send(outPktHdr, outMailHdr, data); 

			if ( !success ) {
			DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
			}

			// wait for the reply msg
			// return (or not) a value to the user program
			DEBUG('u',"DestroyServerCondition syscall called by thread %s Waiting...\n",currentThread->getName());
			postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			DEBUG('u',"DestroyServerCondition syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
			fflush(stdout);

			int id;
			sscanf(buffer,"%*s %*c %d", &id);
			if(id == -1){
				DEBUG('u',"DestroyServerCondition syscall called by thread %s has passed an invalid server condition with id %d for process %d.\n",currentThread->getName(),cid,currentThread->process->processId);
			} else if(id){//id is 1				
				DEBUG('u',"DestroyServerCondition syscall called by thread %s has destroyed server condition with id %d for process %d.\n",currentThread->getName(),cid,currentThread->process->processId);
				currentThread->process->conditionsOwned[cid] = false;
			} else{//id is 0
				DEBUG('u',"DestroyServerCondition syscall called by thread %s has marked for deletion server condition with id %d for process %d.\n",currentThread->getName(),cid,currentThread->process->processId);
			}
		}
	    else{
	            DEBUG('u',"DestroyServerCondition syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF CONDITION WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),cid);
	    }
	    //condLock->Release();
	    return;
	}
	else{
		DEBUG('u',"\nDestroyServerCondition syscall: INVALID CONDITION ID!\n");
		return;
	}
}

int CreateMV_Syscall(int va, int server){
	//first check to see if the name of the about to be created MV is null
	//if it is automatically return a garbage value, if it's not, then send a 
	//message to the server, which will reply with a MV id, if this id is 
	//valid we add it to our list of owned MVs
	DEBUG('u',"\nCreateMonitorVar syscall CALLED by thread %s with va %d\n",currentThread->getName(),va);
	if(va != NULL){
		char* debugName = new char[64];
		copyin(va,64,debugName);
				 
		PacketHeader outPktHdr, inPktHdr;
		MailHeader outMailHdr, inMailHdr;
		char buffer[MaxMailSize]="";
		bool success;
		char data[MaxMailSize] = "RT = CMV NA = ";
		strcat(data,debugName);
    	
		// construct packet, mail header for original message
		// To: destination machine, mailbox 0
		// From: our machine, reply to: mailbox 1
		if(server==-1){
			outPktHdr.to = rand()%numServers;		
			outPktHdr.to = rand()%numServers;
		}
		else
			outPktHdr.to = server;		
		outMailHdr.to = 0;
		outMailHdr.from = currentThread->mailbox;    
		outMailHdr.length = strlen(data) + 1;
   		DEBUG('u',"CreateMonitorVar syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
		// create & send a request msg to the server  
		success = postOffice->Send(outPktHdr, outMailHdr, data); 

		if ( !success ) {
			DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		
		// wait for the reply msg
		// return (or not) a value to the user program
		DEBUG('u',"CreateMonitorVar syscall called by thread %s Waiting...\n",currentThread->getName());
		postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
		outPktHdr.to = inPktHdr.from;
   		outMailHdr.to = inMailHdr.from;
		DEBUG('u',"CreateMonitorVar syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);
		
		int id;
		sscanf(buffer,"%*s %*c %d", &id);
		
		if(id != -1){
			DEBUG('u',"CreateMonitorVar syscall called by thread %s created mv %s (and put it in the server mv table with id %d) or the mv already existed, and process %d is made the owner.\n",currentThread->getName(),debugName,id,currentThread->process->processId);
			currentThread->process->mvsOwned[id] = true;
		} 
		else {
			DEBUG('u',"CreateMonitorVar syscall: COULD NOT CREATE MONITOR VAR %s BECAUSE THE SERVER LOCK TABLE IS FULL!\n",debugName);
			return -1;
		}
		
		return id;
	}
	else{//mv name was null
		DEBUG('u',"\nCreateMonitorVar syscall: NULL POINTER PASSED FOR VARIABLE NAME!\n");
		return -1;
	}	
}

void DestroyMV_Syscall(int mvid, int server){
	//first validate that mv is in range and owned by this thread/process
	//if it isn't, automatically return -1, if it is then send a message to the
	//server and parse the message to destroy the mv, once we receive confirmation,
	//we remove the MV from our list of owned MV's
	DEBUG('u',"\nDestroyMonitorVar syscall CALLED by thread %s on mv with id %d\n",currentThread->getName(),mvid);
	if(mvid >= 0 && mvid%1000 <= 512){
        if(currentThread->process->mvsOwned[mvid] == true){
        	PacketHeader outPktHdr, inPktHdr;
			MailHeader outMailHdr, inMailHdr;
			char buffer[MaxMailSize]="";
			bool success;
			char data[MaxMailSize] = "RT = DMV MVID = ";
			char num[32];
			sprintf(num,"%d",mvid);
			strcat(data,num);
			// construct packet, mail header for original message
			// To: destination machine, mailbox 0
			// From: our machine, reply to: mailbox 1
			if(server==-1){
				outPktHdr.to = rand()%numServers;		
				outPktHdr.to = rand()%numServers;
			}		
			else
				outPktHdr.to = server;		
			outMailHdr.to = 0;
			outMailHdr.from = currentThread->mailbox;    
			outMailHdr.length = strlen(data) + 1;
			DEBUG('u',"DestroyMonitorVar syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
			// create & send a request msg to the server  
			success = postOffice->Send(outPktHdr, outMailHdr, data); 

			if ( !success ) {
			DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
			}

			// wait for the reply msg
			// return (or not) a value to the user program
			DEBUG('u',"DestroyMonitorVar syscall called by thread %s Waiting...\n",currentThread->getName());
			postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			DEBUG('u',"DestroyMonitorVar syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
			fflush(stdout);

			int id;
			sscanf(buffer,"%*s %*c %d", &id);
			if(id!=-1){
				DEBUG('u',"DestroyMonitorVar syscall called by thread %s has destroyed server MonitorVar with id %d for process %d.\n",currentThread->getName(),id,currentThread->process->processId);
				currentThread->process->mvsOwned[id]=false;
				return;
			}else{
				DEBUG('u',"DestroyMonitorVar syscall called by thread %s encountered an error destroying MonitorVar%d for process %d.\n",currentThread->getName(),id,currentThread->process->processId);
				return;
			}
		}
        else{
        	DEBUG('u',"DestroyMonitorVar syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF MonitorVar WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),mvid);
        	return;
        }
	}
	else{
		DEBUG('u',"\nDestroyMonitorVar syscall: INVALID MonitorVar ID!\n");
		return;
	}

}

int GetMV_Syscall(int mvid, int server){
	//first validate that mv is in range and owned by this thread/process
	//if it isn't, automatically return -1, if it is then send a message to the
	//server and parse the message to get the mv's current value
	DEBUG('u',"\nGetMonitorVar syscall CALLED by thread %s on mv with id %d\n",currentThread->getName(),mvid);
	if(mvid >= 0 && mvid%1000 <= 512){
		if(currentThread->process->mvsOwned[mvid] == true){
	       	PacketHeader outPktHdr, inPktHdr;
			MailHeader outMailHdr, inMailHdr;
			char buffer[MaxMailSize]="";
			bool success;
			char data[MaxMailSize] = "RT = GMV MVID = ";
			char num[32];
			sprintf(num,"%d",mvid);
			strcat(data,num);
			// construct packet, mail header for original message
			// To: destination machine, mailbox 0
			// From: our machine, reply to: mailbox 1
			if(server==-1){
				outPktHdr.to = rand()%numServers;		
				outPktHdr.to = rand()%numServers;
			}		
			else
				outPktHdr.to = server;		
			outMailHdr.to = 0;
			outMailHdr.from = currentThread->mailbox;    
			outMailHdr.length = strlen(data) + 1;
			DEBUG('u',"GetMonitorVar syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
			// create & send a request msg to the server  
			success = postOffice->Send(outPktHdr, outMailHdr, data); 

			if ( !success ) {
			DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
			}

			// wait for the reply msg
			// return (or not) a value to the user program
			DEBUG('u',"GetMonitorVar syscall called by thread %s Waiting...\n",currentThread->getName());
			postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			DEBUG('u',"GetMonitorVar syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
			fflush(stdout);

			int id,xv;
			sscanf(buffer,"%*s %*c %d %*s %*c %d", &id,&xv);
			if(id!=-1){
				DEBUG('u',"GetMonitorVar syscall called by thread %s has gotten server MonitorVar with id %d and value %d for process %d.\n",currentThread->getName(),id,xv,currentThread->process->processId);
				return xv;
			}else{
				DEBUG('u',"GetMonitorVar syscall called by thread %s encountered an error getting the value of server MonitorVar%d for process %d.\n",currentThread->getName(),id,currentThread->process->processId);
				return -1;
			}
		}
	        else{
	        	DEBUG('u',"GetMonitorVar syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF MonitorVar WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),mvid);
	        	return -1;
	        }
	}
	else{
		DEBUG('u',"\nGetMonitorVar syscall: INVALID MonitorVar ID!\n");
		return -1;
	}
}

void SetMV_Syscall(int mvid,int x, int server){
	//first validate that mv is in range and owned by this thread/process
	//if it isn't, automatically return -1, if it is then send a message with the requested value to the
	//server and parse the message to get fina mv status
	DEBUG('u',"\nSetMonitorVar syscall CALLED by thread %s on mv with id %d and new value %d\n",currentThread->getName(),mvid,x);
	if(mvid >= 0 && mvid%1000 <= 512){
        if(currentThread->process->mvsOwned[mvid] == true){
        	PacketHeader outPktHdr, inPktHdr;
		MailHeader outMailHdr, inMailHdr;
		char buffer[MaxMailSize]="";
		bool success;
		char data[MaxMailSize] = "RT = SMV MVID = ";
		char num[32];
		sprintf(num,"%d",mvid);
		strcat(data,num);
		strcat(data," MVV = ");
		sprintf(num,"%d",x);
		strcat(data,num);

		// construct packet, mail header for original message
		// To: destination machine, mailbox 0
		// From: our machine, reply to: mailbox 1
		if(server==-1){
			outPktHdr.to = rand()%numServers;		
			outPktHdr.to = rand()%numServers;
		}		
		else
			outPktHdr.to = server;		
		outMailHdr.to = 0;
		outMailHdr.from = currentThread->mailbox;    
		outMailHdr.length = strlen(data) + 1;
		DEBUG('u',"SetMonitorVar syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
		// create & send a request msg to the server  
		success = postOffice->Send(outPktHdr, outMailHdr, data); 

		if ( !success ) {
		DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
		}

		// wait for the reply msg
		// return (or not) a value to the user program
		DEBUG('u',"SetMonitorVar syscall called by thread %s Waiting...\n",currentThread->getName());
		postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
		outPktHdr.to = inPktHdr.from;
		outMailHdr.to = inMailHdr.from;
		DEBUG('u',"SetMonitorVar syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);

		int id,xv;
		sscanf(buffer,"%*s %*c %d %*s %*c %d", &id,&xv);
		if(id!=-1)
			DEBUG('u',"SetMonitorVar syscall called by thread %s has set server MonitorVar with id %d to value %d for process %d.\n",currentThread->getName(),id,xv,currentThread->process->processId);
		else
			DEBUG('u',"SetMonitorVar syscall called by thread %s tried to set an invalid server MonitorVar for process %d.\n",currentThread->getName(),currentThread->process->processId);

        }
        else{
        	DEBUG('u',"SetMonitorVar syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF MonitorVar WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),mvid);
        }
	}
	else{
		DEBUG('u',"\nSetMonitorVar syscall: INVALID MonitorVar ID!\n");
	}
	return;
}

void ServerAcquire_Syscall(int lid, int server){
	//first validate that lock id is in range and owned by this thread/process
	//if it isn't, automatically return -1, if it is then send a message to the
	//server and parse the message to see if the server was able to acquire the
	//lock for us
	DEBUG('u',"\nServerAcquire syscall CALLED by thread %s on lock with id %d\n",currentThread->getName(),lid);
	if(lid >= 0 && lid%1000 <= 512){
        if(currentThread->process->locksOwned[lid] == true){
        	PacketHeader outPktHdr, inPktHdr;
		MailHeader outMailHdr, inMailHdr;
		char buffer[MaxMailSize] = "";
		bool success;
		char data[MaxMailSize] = "RT = SA LID = ";
		char num[32];
		sprintf(num,"%d",lid);
		strcat(data,num);

		// construct packet, mail header for original message
		// To: destination machine, mailbox 0
		// From: our machine, reply to: mailbox 1
		if(server==-1){
			outPktHdr.to = rand()%numServers;		
			outPktHdr.to = rand()%numServers;
		}		
		else
			outPktHdr.to = server;		
		outMailHdr.to = 0;
		outMailHdr.from = currentThread->mailbox;    
		outMailHdr.length = strlen(data) + 1;
		DEBUG('u',"ServerAcquire syscall called by thread %s Sending %s to Server %d...\n\n",currentThread->getName(),data, outPktHdr.to);
		// create & send a request msg to the server  
		success = postOffice->Send(outPktHdr, outMailHdr, data); 

		if ( !success ) {
		DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
		}

		// wait for the reply msg
		// return (or not) a value to the user program
		DEBUG('u',"ServerAcquire syscall called by thread %s Waiting...\n\n",currentThread->getName());
		postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
		outPktHdr.to = inPktHdr.from;
		outMailHdr.to = inMailHdr.from;
		DEBUG('u',"ServerAcquire syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);

		int id;
		sscanf(buffer,"%*s %*c %d", &id);
		if(id!=-1)
			DEBUG('u',"ServerAcquire syscall called by thread %s has Acquired server lock with id %d for process %d.\n",currentThread->getName(),id,currentThread->process->processId);
		else
			DEBUG('u',"ServerAcquire syscall called by thread %s tried to acquire an invalid server lock for process %d.\n",currentThread->getName(),currentThread->process->processId);

        }
        else{
        	DEBUG('u',"ServerAcquire syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),lid);
        }
	}
	else{
		DEBUG('u',"\nServerAcquire syscall: INVALID LOCK ID!\n");
	}
	return;
}

void ServerRelease_Syscall(int lid, int server){
	//validate lock id to see if we own it
	//if we own it, then send a message to the server requesting a release
		//when the release returns, 3 outcomes are possible- failure to release, success without change to the lock variable, 
		//success with the lock variable being deleted(due to mark for deletion)
	//if we dont own it, or it is out of range print an error  message and return
	
	DEBUG('u',"\nServerRelease syscall CALLED by thread %s on lock with id %d\n",currentThread->getName(),lid);
	if(lid >= 0 && lid%1000 <= 512){
		if(currentThread->process->locksOwned[lid] == true){
	        PacketHeader outPktHdr, inPktHdr;
			MailHeader outMailHdr, inMailHdr;
			char buffer[MaxMailSize] = "";
			bool success;
			char data[MaxMailSize] = "RT = SR LID = ";
			char num[32];
			sprintf(num,"%d",lid);
			strcat(data,num);

			// construct packet, mail header for original message
			// To: destination machine, mailbox 0
			// From: our machine, reply to: mailbox 1
			if(server==-1){
				outPktHdr.to = rand()%numServers;		
				outPktHdr.to = rand()%numServers;
			}			
			else
				outPktHdr.to = server;		
			outMailHdr.to = 0;
			outMailHdr.from = currentThread->mailbox;    
			outMailHdr.length = strlen(data) + 1;
			DEBUG('u',"ServerRelease syscall called by  thread %s Sending %s to Server %d...\n\n",currentThread->getName(),data, outPktHdr.to);
			// create & send a request msg to the server  
			success = postOffice->Send(outPktHdr, outMailHdr, data); 

			if ( !success ) {
			DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
			}

			// wait for the reply msg
			// return (or not) a value to the user program
			DEBUG('u',"ServerRelease syscall called by  thread %s Waiting...\n\n",currentThread->getName());
			postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			DEBUG('u',"ServerRelease syscall called by  thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
			fflush(stdout);

			int id;
			sscanf(buffer,"%*s %*c %d", &id);
			if(id!=-1){
				if(id == -2){
					DEBUG('u',"ServerRelease syscall called by thread %s has Released server lock with id %d for process %d and has removed it from their list of owned locks, because it was marked for deletion.\n",currentThread->getName(),lid,currentThread->process->processId);
					currentThread->process->locksOwned[lid] = false;
				}
				else{
					DEBUG('u',"ServerRelease syscall called by thread %s has Released server lock with id %d for process %d.\n",currentThread->getName(),id,currentThread->process->processId);
				}
			} 
			else{
				DEBUG('u',"ServerRelease syscall called by thread %s tried to release an invalid server lock for process %d.\n",currentThread->getName(),currentThread->process->processId);
			}
		}
		else{
	       	DEBUG('u',"ServerRelease syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),lid);
		}
	}
	else{
		DEBUG('u',"\nServerRelease syscall: INVALID LOCK ID!\n");
	}
	return;

}

void ServerWait_Syscall(int cid, int lid, int server){
	//validate lock and condition id's then see if we own them
	//if we own them, then send a message to the server requesting a wait
	//if we dont own them, print an error  message and return
	//N.B. the reply message that wakes us up will actually be from serverlock acquire completing 
	if(cid >= 0 && cid%1000 <= 512 && lid >=0 && lid%1000 <= 512){
        if(currentThread->process->locksOwned[lid] == true && currentThread->process->conditionsOwned[cid] == true){
        	//lock and condition are valid and we own both!
        	
		DEBUG('u',"\nServerWait syscall CALLED by thread %s waiting on condition with id %d using lock with id %d for process %d.\n",currentThread->getName(),cid,lid,currentThread->process->processId);
		PacketHeader outPktHdr, inPktHdr;
		MailHeader outMailHdr, inMailHdr;
		char buffer[MaxMailSize]="";
		bool success;
		char data[MaxMailSize] = "RT = SW CID = ";
		char num[32];
		sprintf(num,"%d",cid);
		strcat(data,num);
		strcat(data," LID = ");
		sprintf(num,"%d",lid);
		strcat(data,num);

		// construct packet, mail header for original message
		// To: destination machine, mailbox 0
		// From: our machine, reply to: mailbox 1
		if(server==-1){
			outPktHdr.to = rand()%numServers;		
			outPktHdr.to = rand()%numServers;
		}			
		else
			outPktHdr.to = server;		
		outMailHdr.to = 0;
		outMailHdr.from = currentThread->mailbox;    
		outMailHdr.length = strlen(data) + 1;
		DEBUG('u',"ServerWait syscall called by thread %s Sending %s to Server %d...\n\n",currentThread->getName(),data, outPktHdr.to);
		// create & send a request msg to the server  
		success = postOffice->Send(outPktHdr, outMailHdr, data); 

		if ( !success ) {
		DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
		}

		// wait for the reply msg
		// return (or not) a value to the user program
		DEBUG('u',"ServerWait syscall called by thread %s ASLEEP and Waiting...\n\n",currentThread->getName());
		postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
		outPktHdr.to = inPktHdr.from;
		outMailHdr.to = inMailHdr.from;
		DEBUG('u',"\nServerWait syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);

		int id,xv;
		sscanf(buffer,"%*s %*c %d %*s %*c %d", &id,&xv);
		if(id==-1 || xv==-1)
			DEBUG('u',"ServerWait syscall FAILED for thread %s using Condition with id %d and Lock with id %d for process %d.\n",currentThread->getName(),cid,lid,currentThread->process->processId);
		else
			DEBUG('u',"ServerWait syscall was SUCCESSFUL and thread %s using Condition with id %d and Lock with id %d for process %d is now AWAKE.\n",currentThread->getName(),id,xv,currentThread->process->processId);			

        }
        else{
                DEBUG('u',"ServerWait syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF EITHER THE CONDITION WITH ID %d OR THE LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),cid, lid);
        }
	}
	else{
		DEBUG('u',"\nServerWait syscall: INVALID LOCK OR CONDITION ID!\n");
	}
	return;

}

void ServerSignal_Syscall(int cid, int lid, int server){
	//validate lock and condition id's then see if we own them
	//if we own them, then send a message to the server requesting a signal
		//when the signal returns, 3 outcomes are possible- failure to signal, success without change to the condition variable, 
		//success with the condition variable being deleted(due to mark for deletion)
	//if we dont own them, print an error  message and return
	if(cid >= 0 && cid%1000 <= 512 && lid >=0 && lid%1000 <= 512){
        if(currentThread->process->locksOwned[lid] == true && currentThread->process->conditionsOwned[cid] == true){
        	//lock and condition are valid and we own both!
        	
		DEBUG('u',"\nServerSignal syscall CALLED by thread %s signalling on condition with id %d using lock with id %d for process %d.\n",currentThread->getName(),cid,lid,currentThread->process->processId);
		PacketHeader outPktHdr, inPktHdr;
		MailHeader outMailHdr, inMailHdr;
		char buffer[MaxMailSize]="";
		bool success;
		char data[MaxMailSize] = "RT = SS CID = ";
		char num[32];
		sprintf(num,"%d",cid);
		strcat(data,num);
		strcat(data," LID = ");
		sprintf(num,"%d",lid);
		strcat(data,num);

		// construct packet, mail header for original message
		// To: destination machine, mailbox 0
		// From: our machine, reply to: mailbox 1
		if(server==-1){
			outPktHdr.to = rand()%numServers;		
			outPktHdr.to = rand()%numServers;
		}			
		else
			outPktHdr.to = server;		
		outMailHdr.to = 0;
		outMailHdr.from = currentThread->mailbox;    
		outMailHdr.length = strlen(data) + 1;
		DEBUG('u',"ServerSignal syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
		// create & send a request msg to the server  
		success = postOffice->Send(outPktHdr, outMailHdr, data); 

		if ( !success ) {
		DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
		}

		// wait for the reply msg
		// return (or not) a value to the user program
		DEBUG('u',"ServerSignal syscall called by thread %s Waiting...\n",currentThread->getName());
		postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
		outPktHdr.to = inPktHdr.from;
		outMailHdr.to = inMailHdr.from;
		DEBUG('u',"ServerSignal syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);

		int id,xv;
		sscanf(buffer,"%*s %*c %d %*s %*c %d", &id,&xv);
		if(id==-1 || xv==-1){
			DEBUG('u',"ServerSignal syscall FAILED for thread %s using Condition with id %d and Lock with id %d for process %d.\n",currentThread->getName(),cid,lid,currentThread->process->processId);
		}
		else if(id==-2 && xv ==-2){//condition was deleted
			DEBUG('u',"ServerSignal syscall was SUCCESSFUL and Condition was DELETED for thread %s using Condition with id %d and Lock with id %d for process %d.\n",currentThread->getName(),cid,lid,currentThread->process->processId);
			currentThread->process->conditionsOwned[cid] = false;
		}
		else{
			DEBUG('u',"ServerSignal syscall was SUCCESSFUL for thread %s using Condition with id %d and Lock with id %d for process %d.\n",currentThread->getName(),cid,lid,currentThread->process->processId);			
		}
        }
        else{
                DEBUG('u',"ServerSignal syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF EITHER THE CONDITION WITH ID %d OR THE LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),cid, lid);
        }
	}
	else{
		DEBUG('u',"\nServerSignal syscall: INVALID LOCK OR CONDITION ID!\n");
	}
	return;

}

void ServerBroadcast_Syscall(int cid, int lid, int server){
	//validate lock and condition id's then see if we own them
	//if we own them, then send a message to the server requesting a broadcast
	//if we dont own them, print an error  message and return
	//N.B. implementation guarantees that we will only be woken up once the server sends
	//a message to everyone that was waiting
	if(cid >= 0 && cid%1000 <= 512&& lid >=0 && lid%1000 <= 512){
        if(currentThread->process->locksOwned[lid] == true && currentThread->process->conditionsOwned[cid] == true){
        	//lock and condition are valid and we own both!
        	
		DEBUG('u',"\nServerBroadcast syscall CALLED by thread %s broadcasting on condition with id %d using lock with id %d for process %d.\n",currentThread->getName(),cid,lid,currentThread->process->processId);
		PacketHeader outPktHdr, inPktHdr;
		MailHeader outMailHdr, inMailHdr;
		char buffer[MaxMailSize]="";
		bool success;
		char data[MaxMailSize] = "RT = SB CID = ";
		char num[32];
		sprintf(num,"%d",cid);
		strcat(data,num);
		strcat(data," LID = ");
		sprintf(num,"%d",lid);
		strcat(data,num);

		// construct packet, mail header for original message
		// To: destination machine, mailbox 0
		// From: our machine, reply to: mailbox 1
		if(server==-1){
			outPktHdr.to = rand()%numServers;		
			outPktHdr.to = rand()%numServers;
		}			
		else
			outPktHdr.to = server;		
		outMailHdr.to = 0;
		outMailHdr.from = currentThread->mailbox;    
		outMailHdr.length = strlen(data) + 1;
		DEBUG('u',"ServerBroadcast syscall called by thread %s Sending %s to Server %d...\n",currentThread->getName(),data, outPktHdr.to);
		// create & send a request msg to the server  
		success = postOffice->Send(outPktHdr, outMailHdr, data); 

		if ( !success ) {
		DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
		}

		// wait for the reply msg
		// return (or not) a value to the user program
		DEBUG('u',"ServerBroadcast syscall called by thread %s Waiting...\n",currentThread->getName());
		postOffice->Receive(currentThread->mailbox, &inPktHdr, &inMailHdr, buffer);
		outPktHdr.to = inPktHdr.from;
		outMailHdr.to = inMailHdr.from;
		DEBUG('u',"ServerBroadcast syscall called by thread %s Got \"%s\" from %d, box %d\n",currentThread->getName(),buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);

		int id,xv;
		sscanf(buffer,"%*s %*c %d %*s %*c %d", &id,&xv);
		if(id==-1 || xv==-1)
			DEBUG('u',"ServerBroadcast syscall FAILED for thread %s using Condition with id %d and Lock with id %d for process %d.\n",currentThread->getName(),cid,lid,currentThread->process->processId);
		else
			DEBUG('u',"ServerBroadcast syscall was SUCCESSFUL for called by thread %s using Condition with id %d and Lock with id %d for process %d.\n",currentThread->getName(),id,xv,currentThread->process->processId);			

        }
        else{
                DEBUG('u',"ServerBroadcast syscall: PROCESS %d WHICH THREAD %s BELONGS TO IS NOT THE OWNER OF EITHER THE CONDITION WITH ID %d OR THE LOCK WITH ID %d!\n",currentThread->process->processId,currentThread->getName(),cid, lid);
        }
	}
	else{
		DEBUG('u',"\nServerBroadcast syscall: INVALID LOCK OR CONDITION ID!\n");
	}
	return;

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
			case SC_Yield:
				DEBUG('a', "Yield syscall.\n");  
				Yield_Syscall();   
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
				Wait_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));    
				break;
			case SC_Signal:
				DEBUG('a', "Signal syscall.\n");
				Signal_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));          
				break;
			case SC_Broadcast:
				DEBUG('a', "Broadcast syscall.\n");
				Broadcast_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));       
				break;
			case SC_CreateLock:
				DEBUG('a', "CreateLock syscall.\n");        
				rv = CreateLock_Syscall(machine->ReadRegister(4));
				break;
			case SC_DestroyLock:
				DEBUG('a', "DestroyLock syscall.\n");
				DestroyLock_Syscall(machine->ReadRegister(4));      
				break;
			case SC_CreateCondition:
				DEBUG('a', "CreateCondition syscall.\n");
				rv = CreateCondition_Syscall(machine->ReadRegister(4));    
				break;
			case SC_DestroyCondition:
				DEBUG('a', "DestroyCondition syscall.\n");
				DestroyCondition_Syscall(machine->ReadRegister(4));    
				break;
			case SC_Print:
				DEBUG('a', "Print syscall.\n");
				Print_Syscall(machine->ReadRegister(4),
				machine->ReadRegister(5),
				machine->ReadRegister(6),
				machine->ReadRegister(7));
				break;
			case SC_Rand:
				DEBUG('a', "Rand syscall.\n");
				rv = Rand_Syscall();
				break;
			case SC_CreateServerLock:
				DEBUG('a', "CreateServerLock syscall.\n");        
				rv = CreateServerLock_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));
				break;
			case SC_DestroyServerLock:
				DEBUG('a', "DestroyServerLock syscall.\n");
				DestroyServerLock_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));      
				break;
			case SC_CreateServerCondition:
				DEBUG('a', "CreateServerCondition syscall.\n");
				rv = CreateServerCondition_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));    
				break;
			case SC_DestroyServerCondition:
				DEBUG('a', "DestroyServerCondition syscall.\n");
				DestroyServerCondition_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));    
				break;
			case SC_CreateMV:
				DEBUG('a', "CreateMV syscall.\n");
				rv = CreateMV_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));    
				break;
			case SC_DestroyMV:
				DEBUG('a', "DestroyMV syscall.\n");
				DestroyMV_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));    
				break;
			case SC_GetMV:
				DEBUG('a', "GetMV syscall.\n");
				rv = GetMV_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));    
				break;
			case SC_SetMV:
				DEBUG('a', "SetMV syscall.\n");
				SetMV_Syscall(machine->ReadRegister(4),
				machine->ReadRegister(5),machine->ReadRegister(6));    
				break;
			case SC_ServerAcquire:
				DEBUG('a', "ServerAcquire syscall.\n");
				ServerAcquire_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));          
				break;
			case SC_ServerRelease:
				DEBUG('a', "ServerRelease syscall.\n");
				ServerRelease_Syscall(machine->ReadRegister(4),machine->ReadRegister(5));          
				break;
			case SC_ServerWait:
				DEBUG('a', "ServerWait syscall.\n");  
				ServerWait_Syscall(machine->ReadRegister(4),machine->ReadRegister(5),machine->ReadRegister(6));    
				break;
			case SC_ServerSignal:
				DEBUG('a', "ServerSignal syscall.\n");
				ServerSignal_Syscall(machine->ReadRegister(4),machine->ReadRegister(5),machine->ReadRegister(6));          
				break;
			case SC_ServerBroadcast:
				DEBUG('a', "ServerBroadcast syscall.\n");
				ServerBroadcast_Syscall(machine->ReadRegister(4),machine->ReadRegister(5),machine->ReadRegister(6));       
				break;
		}

		// Put in the return value and increment the PC
		machine->WriteRegister(2,rv);
		machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
		return;
    	} 
    	else if(which == PageFaultException) {
		//IntStatus oldLevel = interrupt->SetLevel(IntOff);
		//printf("badvaddrreg is %d and vpn is %d\n", machine->ReadRegister(BadVAddrReg), machine->ReadRegister(BadVAddrReg)/PageSize);
    		if(!currentThread->space->CopyToTLB(currentThread->process->processId))  
    			currentThread->space->CopyToIPT(currentThread->process->processId); 
    		//(void) interrupt->SetLevel(oldLevel); 			
    	}
    	else {
      		cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      		interrupt->Halt();
    }
}
