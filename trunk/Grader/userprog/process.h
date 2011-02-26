#pragma once

#include "addrspace.h"
#include "syscall.h"

typedef struct{
	AddrSpace* space;
	char* name;
	SpaceId processId;
	int numThreads;
	bool locksOwned[512];
	bool conditionsOwned[512];
	bool mvsOwned[512];
} Process;
