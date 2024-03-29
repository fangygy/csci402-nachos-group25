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
#include <ctime>		// For seeding random
#include <cstdlib>	// For generating random

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	delete lock;
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
    if ( i != -1)
	table[i] = f;
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
	}
	lock->Release();
    }
    return f;
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
    unsigned int i, size;

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
    numPages = divRoundUp(size, PageSize) + divRoundUp(UserStackSize,PageSize);
                                                // we need to increase the size
						// to leave room for the stack
    size = numPages * PageSize;

	nonStackPageEnd = numPages - 8; // int created to store where exactly the stack starts (non-stack's ending page)
    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

	printf("Initializing address space, num pages %d, size %d\n", 
					numPages, size);
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 

	srand(time(NULL));		//Initializing random number generator for seeding random values
	
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		//pageTable[i].physicalPage = i;
		mainmemLock->Acquire();
		pageTable[i].physicalPage = bitMap.Find();	// Find a free physical page, lock down while doing so
		//printf("New page number: %d\n", pageTable[i].physicalPage);
		mainmemLock->Release();
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    // bzero(machine->mainMemory, size);
	
	// just want to zero out the space that we're using, not all of main memory
	for (i = 0; i < numPages; i++) {
		bzero(&(machine->mainMemory[pageTable[i].physicalPage * PageSize]), PageSize);
	}

// then, copy in the code and data segments into memory
	//Physical address, page size, inFileAddr + virual address
	for (i = 0; i < numPages; i++) {
		executable->ReadAt(&(machine->mainMemory[pageTable[i].physicalPage * PageSize]),
			PageSize, noffH.code.inFileAddr + pageTable[i].virtualPage * PageSize);
	}
    /*if (noffH.code.size > 0) {
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
    }*/

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
	
	currentThread->firstPageTable = nonStackPageEnd;
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

//----------------------------------------------------------------------
// AddrSpace::AllocateStack
// 	Called by Fork_Syscall's kernel_thread function to allocate
//	more pages for a new thread.
//      
//		Then the register writing so it can run that code
//----------------------------------------------------------------------
void AddrSpace::AllocateStack(unsigned int vaddr)
{
	TranslationEntry *newPageTable = new TranslationEntry[numPages+8];
	for(int i = 0; i < numPages; i++) {
		newPageTable[i].virtualPage = pageTable[i].virtualPage;
		newPageTable[i].physicalPage = pageTable[i].physicalPage;
		newPageTable[i].valid = pageTable[i].valid;
		newPageTable[i].use = pageTable[i].use;
		newPageTable[i].dirty = pageTable[i].dirty;
		newPageTable[i].readOnly = pageTable[i].readOnly;
	}
	
	//printf("Adding more pages.\n");
	for(int i = numPages; i < numPages + 8; i++) {
		newPageTable[i].virtualPage = i;
		mainmemLock->Acquire();
		newPageTable[i].physicalPage = bitMap.Find();
		//printf("%d vaddr: New page number: %d\n", vaddr, newPageTable[i].physicalPage);
		mainmemLock->Release();
		newPageTable[i].valid = TRUE;
		newPageTable[i].use = FALSE;
		newPageTable[i].dirty = FALSE;
		newPageTable[i].readOnly = FALSE;
	}
	printf("AddrSpace: Added more pages.\n");
	
	currentThread->firstPageTable = numPages;
	numPages += 8;
	TranslationEntry *deleteTable = pageTable;
	pageTable = newPageTable;
	delete deleteTable;
	
	
	machine->pageTableSize = numPages;
	//printf("Writing new registers\n");
	machine->WriteRegister(PCReg, vaddr);	
	//printf("Writing next register\n");
    machine->WriteRegister(NextPCReg, vaddr+4);
	//printf("Restoring state.\n");
	RestoreState();
	//printf("Writing last register\n");
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
	//printf("After last write.\n");
	
}

//----------------------------------------------------------------------
// AddrSpace::DeallocateStack
// 	Clears the physical pages that the current thread is using
//----------------------------------------------------------------------
void AddrSpace::DeallocateStack() {
	int index = currentThread->firstPageTable;
	int paddr;		// physical address of thread stack
	for(int i = index; i < (index + 8); i++) {
		paddr = pageTable[i].physicalPage;
		mainmemLock->Acquire();
		//printf("Deallocating page number: %d\n", pageTable[i].physicalPage);
		bitMap.Clear(paddr);		// clear physical page
		mainmemLock->Release();
		//pageTable[i].valid = FALSE;		// invalidate the page table entry
	}
	printf("AddrSpace: Deallocated stack.\n");
}

//----------------------------------------------------------------------
// AddrSpace::DeallocateStack
// 	Clears the physical pages that the current thread is using
//	AND the code/init data/uninit data pages
//----------------------------------------------------------------------
void AddrSpace::DeallocateProcess() {
	DeallocateStack();
	int paddr;
	for(int i = 0; i < nonStackPageEnd; i++) {
		if(pageTable[i].valid == TRUE) {
			paddr = pageTable[i].physicalPage;
			//printf("Deallocating page number: %d\n", pageTable[i].physicalPage);
			mainmemLock->Acquire();
			bitMap.Clear(paddr);
			mainmemLock->Release();
			//pageTable[i].valid = FALSE;
		}
	}
	printf("AddrSpace: Deallocated process.\n");
}
