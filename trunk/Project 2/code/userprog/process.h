#ifndef PROCESS_H
#define PROCESS_H
#include "addrspace.h"
#include "syscall.h"

struct process {
	AddrSpace* space;
	char* name;
	SpaceId processId;
	int numThreads;
};

#endif
