// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"
#include "synch.h"

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
    numProcesses=0;
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	//delete lock;
	lock = 0;
    } 
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;	// to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if ( i != -1){
	table[i] = f;
	numProcesses++;
    }
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
	lock->Acquire();
	if ( map.Test(i) ) {
	    map.Clear(i);
	    f = table[i];
	    table[i] = 0;
	    numProcesses--;
	}
	lock->Release();
    }
    return f;
}

int Table::GetSize(){
	return numProcesses;
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable) : fileTable(MaxOpenFiles) {
    NoffHeader noffH;
    unsigned int i, size, numCodeInitPages;

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
	//store the limit of our code section, so we can find if someone is forking to a bad location
	headerCodeLimit=noffH.code.size;
	exe = executable;

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
    numPages = divRoundUp(size, PageSize) + divRoundUp(UserStackSize,PageSize);
                                                // we need to increase the size
						// to leave room for the stack
	numCodeInitPages = divRoundUp(noffH.code.size + noffH.initData.size, PageSize);					

    size = numPages * PageSize;

	//Commenting this out, since we now have virtual memory
    //ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new PageTableTranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		/* COMMENTING OUT FOR PROJECT 3 PART 1 STEP 3
		bitmapLock->Acquire();
		pageTable[i].physicalPage = bitmap.Find();
		bzero(&(machine->mainMemory[pageTable[i].physicalPage*PageSize]), PageSize);
		bitmapLock->Release();
		*/
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
		pageTable[i].swapFileLocation = -1;
		if(i<numCodeInitPages ){
			pageTable[i].pageLocation = IN_EXECUTABLE;
			pageTable[i].diskLocation = noffH.code.inFileAddr + (i*PageSize);
		}
		else{
			pageTable[i].pageLocation = NEITHER;
			pageTable[i].diskLocation = -1;
		}
    }
    

/* COMMENTING OUT FOR PROJECT 3 PART 1 STEP 3
//read code and data into physical memory
	for(i=0;i<numPages;i++){
		executable->ReadAt(&(machine->mainMemory[ pageTable[i].physicalPage*PageSize] ), PageSize, noffH.code.inFileAddr + (i*PageSize));
	}
 */  
    /*	THIS IS THE OLD WAY OF DOING THINGS.  SIMPLY HERE AS A REFERENCE WHILE WE WORK
				// zero out the entire address space, to zero the unitialized data segment 
				// and the stack segment
				    bzero(machine->mainMemory, size);

				// then, copy in the code and data segments into memory
				    if (noffH.code.size > 0) {
				        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
							noffH.code.virtualAddr, noffH.code.size);
				        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
							noffH.code.size, noffH.code.inFileAddr);
				    }
				    if (noffH.initData.size > 0) {
				        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
							noffH.initData.virtualAddr, noffH.initData.size);
				        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
							noffH.initData.size, noffH.initData.inFileAddr);
				    }
    */

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete pageTable;
    delete exe;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
    //set the user stack start so that it can be deallocated later
    currentThread->userStackStart=(numPages*PageSize-16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{	
	for(int i = 0; i < TLBSize; i++){
		if(machine->tlb[i].valid)
			IPT[machine->tlb[i].physicalPage]->dirty = machine->tlb[i].dirty;
		machine->tlb[i].valid = false;
	}
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
	//machine->pageTable = pageTable;
	machine->pageTableSize = numPages;
}

int AddrSpace::AllocateStack(SpaceId pid){
        //setup a new stack by copying our old pageTable and adding 8 new stack pages
        int startStack;
        PageTableTranslationEntry *newPageTable=new PageTableTranslationEntry[numPages+UserStackSize/PageSize];
		PageTableTranslationEntry *temp = new PageTableTranslationEntry[numPages];
        for(unsigned int i=0;i<numPages+UserStackSize/PageSize;i++){
                if(i<numPages){
                  newPageTable[i].virtualPage=pageTable[i].virtualPage;
                  newPageTable[i].physicalPage=pageTable[i].physicalPage;
                  newPageTable[i].valid = pageTable[i].valid;
                  newPageTable[i].use = pageTable[i].use;
                  newPageTable[i].dirty = pageTable[i].dirty;
                  newPageTable[i].readOnly = pageTable[i].readOnly;
                  newPageTable[i].diskLocation = pageTable[i].diskLocation;
                  newPageTable[i].pageLocation = pageTable[i].pageLocation;
                  newPageTable[i].swapFileLocation = pageTable[i].swapFileLocation;
                }
				//make new stack pages
                else{
                        newPageTable[i].virtualPage = i;
                        /*
                        bitmapLock->Acquire();
                        newPageTable[i].physicalPage = bitmap.Find();
                        //bzero(&(machine->mainMemory[pageTable[i].physicalPage*PageSize]), PageSize);
                        bitmapLock->Release();
                        */
                        newPageTable[i].valid = TRUE;
                        newPageTable[i].use = FALSE;
                        newPageTable[i].dirty = FALSE;
                        newPageTable[i].readOnly = FALSE;
                        newPageTable[i].diskLocation = -1; 
			newPageTable[i].pageLocation = NEITHER;
			newPageTable[i].swapFileLocation = -1;
            		/* COMMENT OUT FOR PROJECT 3 PART 1 STEP 3           
            			// PROJECT 3 PART 1 STEP 2           
						DEBUG('v',"Setting IPT page %d with stack's virtual page %d, requested by thread %s\n", newPageTable[i].physicalPage, i, currentThread->getName());
						IPT[newPageTable[i].physicalPage]->virtualPage = newPageTable[i].virtualPage;
		    			IPT[newPageTable[i].physicalPage]->physicalPage = newPageTable[i].physicalPage;
		    			IPT[newPageTable[i].physicalPage]->valid = newPageTable[i].valid;
		    			IPT[newPageTable[i].physicalPage]->use = newPageTable[i].use;
		    			IPT[newPageTable[i].physicalPage]->dirty = newPageTable[i].dirty;
		    			IPT[newPageTable[i].physicalPage]->readOnly = newPageTable[i].readOnly;
		    			IPT[newPageTable[i].physicalPage]->processOwner = pid;
		    			IPT[newPageTable[i].physicalPage]->diskLocation = newPageTable[i].diskLocation; 
						IPT[newPageTable[i].physicalPage]->pageLocation = newPageTable[i].pageLocation;
					*/
                }
        }
        numPages+=UserStackSize/PageSize;
        startStack=numPages*PageSize -16;
		temp=pageTable;
        pageTable=newPageTable;
	//delete our old page table
	delete temp; 
        //machine->pageTable=newPageTable;
        machine->pageTableSize=numPages;
        DEBUG('u',"\nAllocating stack for thread %s at %d.\n", currentThread->getName(),startStack);
        return startStack;
	//maybe I could just do this, since I'm already changing the machine's page table?
	//machine->WriteRegister(StackReg, numPages * PageSize - 16);
}

void AddrSpace::DeallocateStack(){
	DEBUG('u',"\nDeallocating stack for thread %s at %d.\n", currentThread->getName(),currentThread->userStackStart);

        //clear the bitmap for the pages associated with stack
	//depending on how StackReg changes based on tests- also stackReg should be an argument to this func?
        int va = currentThread->userStackStart;
        int vpn = va/PageSize;
        for(int i=0;i<UserStackSize/PageSize;i++){
                bitmap.Clear(pageTable[vpn-i].physicalPage);
                //pageTable[vpn+i].valid = FALSE;
                // PROJECT 3 PART 1 STEP 2
                DEBUG('v',"Invalidating virtual page %d, located in IPT index %d, requested by thread %s\n", vpn-i, pageTable[vpn-i].physicalPage, currentThread->getName());
                IPT[pageTable[vpn-i].physicalPage]->valid = false;
        }
}

void AddrSpace::DeallocateProcess(){
	DEBUG('u',"\nDeallocating process %d for thread %s.\n", currentThread->process->processId,currentThread->getName());
        //clear the bitmap for the process pages(-8 because the first stack was added to numpages :D
        for(unsigned int i=0;i<numPages-UserStackSize/PageSize;i++){
                bitmap.Clear(pageTable[i].physicalPage);
                // PROJECT 3 PART 1 STEP 2
                IPT[pageTable[i].physicalPage]->valid = false;
        }
	//call the destructor here??
}

bool AddrSpace::CopyToTLB(SpaceId pid){
	//Get the VPN for our BadVAddr
	int vpn = machine->ReadRegister(BadVAddrReg) / PageSize;
	
	/*
	machine->tlb[nextTLBSlot].virtualPage = pageTable[vpn].virtualPage;
        machine->tlb[nextTLBSlot].physicalPage = pageTable[vpn].physicalPage;
        machine->tlb[nextTLBSlot].valid = true;
        machine->tlb[nextTLBSlot].use = pageTable[vpn].use;
        machine->tlb[nextTLBSlot].dirty = pageTable[vpn].dirty;
        machine->tlb[nextTLBSlot].readOnly = pageTable[vpn].readOnly;
        nextTLBSlot = (nextTLBSlot + 1) % TLBSize;
        */
	
	// PROJECT 3 PART 1 STEP 2
	//IntStatus oldLevel = interrupt->SetLevel(IntOff);
	//have to acquire a lock here, because this function can be reached by two different code paths
	//only one of which is protected with a lock- e.g. only half the entry points to this routine are
	//mutually exclusive D:
	IPTLock->Acquire();
	for(unsigned int i = 0; i < NumPhysPages; i++){
		if(IPT[i]->valid == true){
			if(IPT[i]->virtualPage == vpn){
				if(IPT[i]->processOwner == pid){
					//IntStatus oldLevel = interrupt->SetLevel(IntOff);
					if(machine->tlb[nextTLBSlot].valid == true){
						IPT[machine->tlb[nextTLBSlot].physicalPage]->dirty = machine->tlb[nextTLBSlot].dirty;
					}
					IPTLock->Release();
					//turn off interrupts to access tlb, as per lecture notes
					IntStatus oldLevel = interrupt->SetLevel(IntOff);
					DEBUG('v',"Copying Process %d's virtual page %d (physical page %d) to TLB slot %d\n", pid, IPT[i]->virtualPage, IPT[i]->physicalPage, nextTLBSlot);
					machine->tlb[nextTLBSlot].virtualPage = IPT[i]->virtualPage;
					machine->tlb[nextTLBSlot].physicalPage = IPT[i]->physicalPage;
					machine->tlb[nextTLBSlot].valid = true;
					machine->tlb[nextTLBSlot].use = IPT[i]->use;
					machine->tlb[nextTLBSlot].dirty = IPT[i]->dirty;
					machine->tlb[nextTLBSlot].readOnly = IPT[i]->readOnly;
					nextTLBSlot = (nextTLBSlot + 1) % TLBSize;
					(void) interrupt->SetLevel(oldLevel); 
					return true;
				}
				else{
					DEBUG('v',"Requesting Process %d is not the owner of requested virtual page %d\n", pid, vpn);
				}
			}
		}
	}
	DEBUG('v',"IPT doesn't hold the requested virtual address %d, in virtual page %d, requested by thread %s\n", machine->ReadRegister(BadVAddrReg), vpn, currentThread->getName());
	//(void) interrupt->SetLevel(oldLevel);
	IPTLock->Release(); 
	return false;
}

void AddrSpace::PopulateIPT(SpaceId pid){
	DEBUG('v',"Populating IPT using Process %d\n", pid);
	IPTLock->Acquire();
	for(unsigned int i = 0; i < numPages; i++){
		DEBUG('v',"Setting IPT page %d with virtual page %d\n", pageTable[i].physicalPage, i);
		IPT[pageTable[i].physicalPage]->virtualPage = pageTable[i].virtualPage;
		IPT[pageTable[i].physicalPage]->physicalPage = pageTable[i].physicalPage;
		IPT[pageTable[i].physicalPage]->valid = pageTable[i].valid;
		IPT[pageTable[i].physicalPage]->use = pageTable[i].use;
		IPT[pageTable[i].physicalPage]->dirty = pageTable[i].dirty;
		IPT[pageTable[i].physicalPage]->readOnly = pageTable[i].readOnly;
		IPT[pageTable[i].physicalPage]->processOwner = pid;
	}
	IPTLock->Release();
}

void AddrSpace::CopyToIPT(SpaceId pid){
	int vpn = machine->ReadRegister(BadVAddrReg) / PageSize;
	//DEBUG('y', "Thread %s accessing physical page %d and virtual page %d from main memory\n",currentThread->getName(),pageTable[vpn].physicalPage,pageTable[vpn].virtualPage);
	//printf("badvaddrreg is %d and vpn is %d\n", machine->ReadRegister(BadVAddrReg), machine->ReadRegister(BadVAddrReg)/PageSize);
	bitmapLock->Acquire();
	//printf("badvaddrreg is %d and vpn is %d\n", machine->ReadRegister(BadVAddrReg), machine->ReadRegister(BadVAddrReg)/PageSize);
	pageTable[vpn].physicalPage = bitmap.Find();
	//have to lock around eviction as it is not a thread safe routine- furthermore, eviction being nonconcurrent shouldn't be a throughput issue, because if
	//many threads paritially evicting many pages will not actually freeup those pages for use; it seems better to just give more cpu time to freeing one page at a time
	//so that it can be reused
	evictLock->Acquire();
	bitmapLock->Release();
	if(pageTable[vpn].physicalPage == -1){
		pageTable[vpn].physicalPage = EvictPage(pid);
		//DEBUG('y', "Thread %s Evicting physical page %d and virtual page %d from main memory\n",currentThread->getName(),pageTable[vpn].physicalPage,pageTable[vpn].virtualPage);
	}
	IPTLock->Acquire();
	evictLock->Release();
	bzero(&(machine->mainMemory[pageTable[vpn].physicalPage*PageSize]), PageSize);
	//swapLock->Acquire();
	//evictLock->Release();
	
	//swap in the new page
	//this page may have been invalid from the evict routine
	pageTable[vpn].valid=true;
	IPT[pageTable[vpn].physicalPage]->virtualPage = pageTable[vpn].virtualPage;
	IPT[pageTable[vpn].physicalPage]->physicalPage = pageTable[vpn].physicalPage;
	IPT[pageTable[vpn].physicalPage]->valid = pageTable[vpn].valid;
	IPT[pageTable[vpn].physicalPage]->use = pageTable[vpn].use;
	IPT[pageTable[vpn].physicalPage]->dirty = pageTable[vpn].dirty;
	IPT[pageTable[vpn].physicalPage]->readOnly = pageTable[vpn].readOnly;
	IPT[pageTable[vpn].physicalPage]->processOwner = pid;
	if(pageTable[vpn].pageLocation == IN_EXECUTABLE){
		exe->ReadAt(&(machine->mainMemory[ pageTable[vpn].physicalPage*PageSize] ), PageSize, pageTable[vpn].diskLocation);
	}
	if(pageTable[vpn].pageLocation == IN_SWAP_FILE){
		swapFile->ReadAt(&(machine->mainMemory[ pageTable[vpn].physicalPage*PageSize] ), PageSize, pageTable[vpn].swapFileLocation*PageSize);
		//DEBUG('s', "Reading virtual page %d of process %d from swapFileLocation %d\n",pageTable[vpn].virtualPage, IPT[pageTable[vpn].physicalPage]->processOwner,pageTable[vpn].swapFileLocation);
	}
	
	IPTLock->Release();

	CopyToTLB(pid);
	//bitmapLock->Release();

	
	


	//PrintTLB();
}

int AddrSpace::EvictPage(SpaceId pid){
	
	if(PageEvictionPolicy == RAND)
		nextPageToEvict = rand() % NumPhysPages;
	else if(PageEvictionPolicy == FIFO)
		nextPageToEvict = (nextPageToEvict + 1) % NumPhysPages;
	DEBUG('e', "Evicting nextPageToEvict %d and virtual page %d from main memory\n",nextPageToEvict,IPT[nextPageToEvict]->virtualPage);
	
	//we set the valid bit of this page's addrspace to false
	((Process*)(processTable.Get(IPT[nextPageToEvict]->processOwner)))->space->pageTable[IPT[nextPageToEvict]->virtualPage].valid=false;
	//First check if page is in the TLB
	for(int i = 0; i < TLBSize; i++){
		if(machine->tlb[i].valid == true)
			if(machine->tlb[i].virtualPage == IPT[nextPageToEvict]->virtualPage){
				//if(IPT[nextPageToEvict]->processOwner == pid){ //not necessary- if multiple pages hade the same vpn only one would be valid
					//Page about to be evicted is in the tlb
					machine->tlb[i].valid = false;
					IPT[nextPageToEvict]->dirty = machine->tlb[i].dirty;
					DEBUG('e',"nextPageToEvict %d with virtual page %d currently exists in the TLB\n",nextPageToEvict,IPT[nextPageToEvict]->virtualPage);
					break;
				}
	}
	//If page is dirty, we need to write it back to the disk
	//We are assuming that the executable will never get changed 
	//(nachos will never try to write to the executable, since this 
	//is our code and we shouldn't alter it on the fly)
	if(IPT[nextPageToEvict]->dirty){
		swapBitmapLock->Acquire();
		if(((Process*)(processTable.Get(IPT[nextPageToEvict]->processOwner)))->space->pageTable[IPT[nextPageToEvict]->virtualPage].swapFileLocation == -1){
			//printf("the swap location was -1 and next page to evict was %d \n",nextPageToEvict);
			((Process*)(processTable.Get(IPT[nextPageToEvict]->processOwner)))->space->pageTable[IPT[nextPageToEvict]->virtualPage].swapFileLocation = swapBitmap.Find();
			if(((Process*)(processTable.Get(IPT[nextPageToEvict]->processOwner)))->space->pageTable[IPT[nextPageToEvict]->virtualPage].swapFileLocation == -1){
				DEBUG('s',"No space in swap file halting user program!\n");
				swapBitmapLock->Release();
				interrupt->Halt();
			}
		}
		swapBitmapLock->Release();
		//Found a place in the swap file for the page to live
		swapFile->WriteAt(&(machine->mainMemory[ ((Process*)(processTable.Get(IPT[nextPageToEvict]->processOwner)))->space->pageTable[IPT[nextPageToEvict]->virtualPage].physicalPage*PageSize]) ,PageSize,((Process*)(processTable.Get(IPT[nextPageToEvict]->processOwner)))->space->pageTable[IPT[nextPageToEvict]->virtualPage].swapFileLocation*PageSize);
		// add enum of in swapfile
		((Process*)(processTable.Get(IPT[nextPageToEvict]->processOwner)))->space->pageTable[IPT[nextPageToEvict]->virtualPage].pageLocation = IN_SWAP_FILE;
		DEBUG('s', "Writing virtual page %d of process %d to swap file to swapFileLocation %d\n",IPT[nextPageToEvict]->virtualPage, IPT[nextPageToEvict]->processOwner,((Process*)(processTable.Get(IPT[nextPageToEvict]->processOwner)))->space->pageTable[IPT[nextPageToEvict]->virtualPage].swapFileLocation);
	}
	
	return nextPageToEvict;
}

void AddrSpace::PrintTLB(){
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	for(int i = 0; i < TLBSize; i++){
		DEBUG('t', "TLB slot %d holds physical page %d (virtual page %d).\n",i,machine->tlb[i].physicalPage,machine->tlb[i].virtualPage);
	}
	(void) interrupt->SetLevel(oldLevel);
}
