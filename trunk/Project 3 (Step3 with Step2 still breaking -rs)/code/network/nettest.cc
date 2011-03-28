// nettest.cc 
//	Test out message delivery between two "Nachos" machines,
//	using the Post Office to coordinate delivery.
//
//	Two caveats:
//	  1. Two copies of Nachos must be running, with machine ID's 0 and 1:
//		./nachos -m 0 -o 1 &
//		./nachos -m 1 -o 0 &
//
//	  2. You need an implementation of condition variables,
//	     which is *not* provided as part of the baseline threads 
//	     implementation.  The Post Office won't work without
//	     a correct implementation of condition variables.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"
#include "syscall.h"
#include "list.h"

// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our 
//	    original message

#define MAX_LOCKS 512
#define MAX_CONDITIONS 512
#define MAX_MVS 512
#define MAX_CLIENTS 512

int numServerLocks = 0;
int numServerCVs = 0;
int numMVs = 0;


struct ServerLock {
	bool exists;
	char* name;
	int holder;		// machineID of the lock holder
	List *queue;
	int numClients;
	int clientID[MAX_CLIENTS];

	ServerLock(){
		exists = false;
		name = "";
		holder = -1;
		queue = new List;
		numClients = 0;
	}

	ServerLock(char* n){
		exists = true;
		name = n;
		holder = -1;
		queue = new List;
		numClients = 0;
	}
};

struct ServerCV {
	char* name;
	bool exists;
	int waitingLock;
	List* queue;
	int numClients;
	int clientID[MAX_CLIENTS];

	ServerCV() {
		exists = false;
		name = "";
		waitingLock = -1;
		queue = new List;
		numClients = 0;
	}
	
	ServerCV(char* n){
		exists = false;
		name = n;
		waitingLock = -1;
		queue = new List;
		numClients = 0;
	}
};

struct ServerMV {
	char* name;
	int value;
}

ServerLock serverLocks[MAX_LOCKS];
ServerCV serverCVs[MAX_CONDITIONS];
ServerMV serverMVs[MAX_MVS];

void CreateLock_RPC(char* name, int machineID) {
	//If reached max lock capacity, return -1
	if (numServerLocks >= MAX_LOCKS) {
		printf("Server - CreateLock_RPC: Max server lock limit reached, cannot create.\n");
		
		//SEND ERROR MESSAGE BACK
		
		return;
	}
	
	for (int i = 0; i < MAX_LOCKS; i++) {
		//Check if lock already exists
		if ( (strcmp(name, serverLocks[i].name)) == 0 &&
			 serverLocks[i].exists) {
			
			//If it does, check to see if this machine has already created it
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverLocks[i].clientID[j] == machineID) {
					printf("Server - CreateLock_RPC: Machine%d has already created this lock.\n", machineID);
					
					//SEND INDEX i IN MESSAGE BACK
					
					return;
				}
			}
			
			//If this machine hasn't created it, add the ID to the lock's list
			// and increment number of clients, then return the lock index
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverLocks[i].clientID[j] == 0) {
					serverLocks[i].clientID[j] = machineID;
					serverLocks[i].numClients++;
					
					//SEND INDEX i IN MESSAGE BACK
					
					return;
				}
			}
		}
	}
	
	//If lock doesn't exist, create it and set the name
	// Find an open space in the lock's client list, add this machine
	// and return the lock index
	for (int i = 0; i < MAX_LOCKS; i++) {
		if (!serverLocks[i].exists) {
			numServerLocks++;
			serverLocks[i].exists = true;
			serverLocks[i].name = name;
			
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverLocks[i].clientID[j] == 0) {
					serverLocks[i].clientID[j] = machineID;
					serverLocks[i].numClients++;
					
					//SEND INDEX i IN MESSAGE
					
					return;
				}
			}
		}	 
	}
	
	//Should never reach here
	return;
}

void Acquire_RPC(int lockIndex, int machineID) {
	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("Server - Acquire_RPC: Machine%d trying to acquire non-existant ServerLock%d\n", machineID, lockIndex);
		
		// SEND ERROR MESSAGE BACK
		
		return;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[lockIndex].clientID[i] == machineID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Acquire_RPC: Machine%d trying to acquire ServerLock%d that has not been 'created'.\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		
		return;
	}
	
	//If already owner, return 0
	if (serverLocks[lockIndex].holder == machineID) {
		printf("Server - Acquire_RPC: Machine%d is already the owner of ServerLock%d\n", machineID, lockIndex);
		
		//SEND MESSAGE BACK
		
		return;
	}
	
	if (serverLocks[lockIndex].holder == -1) {
		serverLocks[lockIndex].holder = machineID;
		
		//SEND MESSAGE BACK
		
		return;
	}
	else {
		serverLocks[lockIndex].queue->Append((void*)machineID);
		
		//DONT SEND MESSAGE
		
		return;
	}
}

void Release_RPC(int lockIndex, int machineID) {
	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("Server - Release_RPC: Machine%d trying to release non-existant ServerLock%d\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		
		return;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[lockIndex].clientID[i] == machineID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Release_RPC: Machine%d trying to release ServerLock%d that has not been 'created'.\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		
		return;
	}
	
	//If not owner, return -1
	if (serverLocks[lockIndex].holder != machineID) {
		printf("Server - Release_RPC: Machine%d is not the owner of ServerLock%d\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		
		return;
	}
	
	//SEND ACTUAL MESSAGE BACK TO machineID
	
	if (serverLocks[lockIndex].queue->IsEmpty()) {
		serverLocks[lockIndex].holder = -1;
		return;
	}
	
	int nextToAcquire = (int)serverLocks[lockIndex].queue->Remove();
	
	serverLocks[lockIndex].holder = nextToAcquire;
	
	//SEND MESSAGE TO nextToAcquire
	return;
}

void DestroyLock_RPC(int lockIndex, int machineID) {
	//If this lock doesn't exist, return
	if (!serverLocks[lockIndex].exists) {
		printf("Server - DestroyLock_RPC: Machine%d trying to destroy non-existant ServerLock%d\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK
		
		return;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[lockIndex].clientID[i] == machineID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - DestroyLock_RPC: Machine%d trying to destroy ServerLock%d that has not been 'created'.\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK
		
		return;
	}
	
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[lockIndex].clientID[i] == machineID) {
			serverLocks[lockIndex].clientID[i] = 0;
			break;
		}
	}
	serverLocks[lockIndex].numClients--;
	
	//SEND MESSAGE BACK TO machineID
	
	//If no more clients, delete lock
	if (serverLocks[lockIndex].numClients == 0) {
		serverLocks[lockIndex].exists = false;
		serverLocks[lockIndex].name = "";
		numServerLocks--;
		return;
	}
	
	//If this client is current owner, release it
	if (serverLocks[lockIndex].holder == machineID) {
		if (serverLocks[lockIndex].queue->IsEmpty()) {
			serverLocks[lockIndex].holder = -1;
			return;
		}
		
		int nextToAcquire = (int)serverLocks[lockIndex].queue->Remove();
	
		serverLocks[lockIndex].holder = nextToAcquire;
		
		//SEND MESSAGE TO nextToAcquire
		return;
	}
	
	return;
}

int CreateCV_RPC(char* name) {
	//If reached max cv capacity, return -1
	if (numServerCVs >= MAX_CONDITIONS) {
		printf("ServerCreateCV_Syscall: Max server cv limit reached, cannot create.\n");
		return -1;
	}
	
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
	
	for (int i = 0; i < MAX_CONDITIONS; i++) {
		//Check if condition already exists
		if ( (strcmp(name, serverCVs[i].name)) == 0 &&
			 serverCVs[i].exists) {
			
			//If it does, check to see if this machine has already created it
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverCVs[i].clientID[j] == machineID) {
					printf("ServerCreateCV_Syscall: Machine%d has already created this cv.\n", machineID);
					return i;
				}
			}
			
			//If this machine hasn't created it, add the ID to the lock's list
			// and increment number of clients, then return the lock index
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverCVs[i].clientID[j] == 0) {
					serverCVs[i].clientID[j] = machineID;
					serverCVs[i].numClients++;
					return i;
				}
			}
		}
	}
	
	//If lock doesn't exist, create it and set the name
	// Find an open space in the lock's client list, add this machine
	// and return the lock index
	for (int i = 0; i < MAX_CONDITIONS; i++) {
		if (!serverCVs[i].exists) {
			numServerCVs++;
			serverCVs[i].exists = true;
			serverCVs[i].name = name;
			
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverCVs[i].clientID[j] == 0) {
					serverCVs[i].clientID[j] = machineID;
					serverCVs[i].numClients++;
					return i;
				}
			}
		}	 
	}

	/*
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char* data;
	char* ack;
	
	char buffer[MaxMailSize];
	
	int condIndex;
	
	//Create the correct message to send here? Ask Antonio later
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	
	//Parse buffer into a condIndex
	
    fflush(stdout);
	
	return condIndex;
	*/
}

void Wait_RPC(int cIndex, int lIndex) {
	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("ServerWaitSyscall: Machine%d trying to wait on non-existant ServerLock%d\n", machineID, lockIndex);
	//	return -1;
	}
	
	//If condition doesn't exist, return -1
	if (!serverCVs[conditionIndex].exists) {
		printf("ServerWaitSyscall: Machine%d trying to wait on non-existant ServerCV%d\n", machineID, conditionIndex);
	//	return -1;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[lockIndex].clientID[i] == machineID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("ServerWaitSyscall: Machine%d trying to wait on ServerLock%d that has not been 'created'.\n", machineID, lockIndex);
	//	return -1;
	}
	
	//Same for condition, make sure is client
	isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[conditionIndex].clientID[i] == machineID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("ServerWaitSyscall: Machine%d trying to wait on ServerCV%d that has not been 'created'\n", machineID, conditionIndex);
	//	return -1;
	}	
	
	//If waitingLock is -1, set it to the passed in lock
	// Else if lock is wrong, return -1
	if (serverCVs[conditionIndex].waitingLock == -1) {
		serverCVs[conditionIndex].waitingLock = lockIndex;
	}
	else if (serverCVs[conditionIndex].waitingLock != lockIndex) {
		printf("ServerWaitSyscall: Machine%d trying to wait on wrong ServerLock%d in ServerCV%d\n", machineID, lockIndex, conditionIndex);
	//	return -1;
	}
	
	//Needs to be the holder for the lock
	if (serverLocks[lockIndex].holder != machineID) {
		printf("ServerWaitSyscall: Machine%d is not the holder of ServerLock%d\n", machineID, lockIndex);
	//	return -1;
	}
	
	//Actually wait now
	serverCVs[conditionIndex].queue->Append((void*)machineID);
	
	//Return 0 if no locks are going to acquire the lock
	// after this one releases it
	if (serverLocks[lockIndex].queue->IsEmpty()) {
		serverLocks[lockIndex].holder = -1;
	//	return 0;
	}
	
	int nextToAcquire = (int)serverLocks[lockIndex].queue->Remove();
	
	serverLocks[lockIndex].holder = nextToAcquire;
	
	//Otherwise return the new lock holder
	//return serverLocks[lockIndex].holder;

	/*
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char* data;
	char* ack;
	
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	printf("Waiting on Condition: %d with Lock: %d\n", cIndex, lIndex);
	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	printf("Woken on Condition: %d with Lock: %d\n", cIndex, lIndex);
	
    fflush(stdout);
	*/
}

void Signal_RPC(int cIndex, int lIndex) {
	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("ServerSignalSyscall: Machine%d trying to signal on non-existant ServerLock%d\n", machineID, lockIndex);
	//	return -1;
	}
	
	//If condition doesn't exist, return -1
	if (!serverCVs[conditionIndex].exists) {
		printf("ServerSignalSyscall: Machine%d trying to signal on non-existant ServerCV%d\n", machineID, conditionIndex);
	//	return -1;
	}

	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[lockIndex].clientID[i] == machineID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("ServerSignalSyscall: Machine%d trying to signal on ServerLock%d that has not been 'created'.\n", machineID, lockIndex);
	//	return -1;
	}

	//Same for condition, make sure is client
	isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[conditionIndex].clientID[i] == machineID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("ServerSignalSyscall: Machine%d trying to signal on ServerCV%d that has not been 'created'\n", machineID, conditionIndex);
	//	return -1;
	}	
	
	// If waitingLock is -1, set it to the passed in lock
	// Else if lock is wrong, return -1
	if (serverCVs[conditionIndex].waitingLock == -1) {
		serverCVs[conditionIndex].waitingLock = lockIndex;
	}
	else if (serverCVs[conditionIndex].waitingLock != lockIndex) {
		printf("ServerSignalSyscall: Machine%d trying to signal on wrong ServerLock%d in ServerCV%d\n", machineID, lockIndex, conditionIndex);
	//	return -1;
	}
	
	//Needs to be the holder for the lock
	if (serverLocks[lockIndex].holder != machineID) {
		printf("ServerSignalSyscall: Machine%d is not the owner of ServerLock%d\n", machineID, lockIndex);
	//	return -1;
	}

	// Queue must not be empty
	if (serverCVs[conditionIndex].queue->IsEmpty()) {
		printf("ServerSignalSyscall: Condition queue is empty. Nothing waiting.\n");
	//	return 0;
	}

	int nextWaiting = (int)serverCVs[conditionIndex].queue->Remove();

	if (serverCVs[conditionIndex].queue->IsEmpty()) {
		printf("ServerSignalSyscall: Condition queue is now empty. Nothing waiting.\n");
		serverCVs[conditionIndex].waitingLock = -1;	
	}
	
	if (serverLocks[lockIndex].holder == -1) {
		serverLocks[lockIndex].holder = nextWaiting;
	//	return nextWaiting;
	}
	else {
		serverLocks[lockIndex].queue->Append((void*)nextWaiting);
	//	return 0;
	}		

	/*
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char* data;
	char* ack;
	
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
	printf("Signalling Condition: %d with Lock: %d\n", cIndex, lIndex);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	
    fflush(stdout);
	*/
}

void Broadcast_RPC(int cIndex, int lIndex) {
	/*
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char* data;
	char* ack;
	
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
	printf("Broadcasting Condition: %d with Lock: %d\n", cIndex, lIndex);
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	
    fflush(stdout);
	*/
}

void DestroyCV_RPC(int cIndex) {
	//If this CV doesn't exist, return
	if (!serverCVs[conditionIndex].exists) {
		printf("ServerDestroyCVSyscall: Machine%d trying to destroy non-existant ServerCV%d\n", machineID, conditionIndex);
		return -1;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[conditionIndex].clientID[i] == machineID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("ServerDestroyCVSyscall: Machine%d trying to destroy ServerLock%d that has not been 'created'.\n", machineID, conditionIndex);
		return -1;
	}
	
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[conditionIndex].clientID[i] == machineID) {
			serverCVs[conditionIndex].clientID[i] = 0;
			break;
		}
	}
	serverCVs[conditionIndex].numClients--;
	
	//If no more clients, delete lock
	if (serverCVs[conditionIndex].numClients == 0) {
		serverCVs[conditionIndex].exists = false;
		serverCVs[conditionIndex].name = "";
		numServerCVs--;
		return 0;
	}
	/*
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char* data;
	char* ack;
	
	char buffer[MaxMailSize];
	
	//Create the correct message to send here? Ask Antonio later
	
	// Check following if this will actually work?
	outPktHdr.to = 0;		
    outMailHdr.to = 0; 
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	printf("Successfully called Destroy on Condition: %d\n", cIndex);
	
    fflush(stdout);
	*/
}

int CreateMV_RPC(char* name, int value) {
	if (numMVs >= MAX_MVS) {
		// print error msg?
		printf("CreateMV_Syscall: Error: Number of Monitor Vars exceeded maximum Monitor Vars limit.\n");
		mvLock->Release();
		return -1;
	}
	if (val == 0x9999) {
		// print error msg
		printf("GetMV_Syscall: Cannot set MV to reserved \"uninitialized\" value.\n");
		mvLock->Release();
		return -1;
	}
	
	int index = -1;
	for (int i = 0; i < MAX_MVS; i++) {
	// find 1st vacancy in the list
		if (monitorVars[i] == 0x9999) {
			index = i;
			break;
		}
	}
	
	monitorVars[index] = val;
	
	return index;
}

int GetMV_RPC(int index) {
	
	if (index < 0) {
		// print error msg
		printf("GetMV_Syscall: MV index less than zero. Invalid.\n");
		mvLock->Release();
		return -1;
	}
	if (index >= MAX_CONDITIONS) {
		// print error msg
		printf("GetMV_Syscall: MV index >= MAX_MVS. Invalid.\n");
		mvLock->Release();
		return -1;
	}
	
	int val = monitorVars[index];
	
	return (val);
}

void SetMV_RPC(int index, int value) {
	if (index < 0) {
		// print error msg
		printf("GetMV_Syscall: MV index less than zero. Invalid.\n");
		mvLock->Release();
		return;
	}
	if (index >= MAX_CONDITIONS) {
		// print error msg
		printf("GetMV_Syscall: MV index >= MAX_MVS. Invalid.\n");
		mvLock->Release();
		return;
	}
	if (val == 0x9999) {
		// print error msg
		printf("GetMV_Syscall: Cannot set MV to reserved \"uninitialized\" value.\n");
		mvLock->Release();
		return;
	}
	
	monitorVars[index] = val;
	return;
}

void MailTest(int farAddr) {
	//netAddr?
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Hello there!";
    char *ack = "Got it!";
    char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = farAddr;		
    outMailHdr.to = 0;
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(ack) + 1;
    success = postOffice->Send(outPktHdr, outMailHdr, ack); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the ack from the other machine to the first message we sent.
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Then we're done!
    interrupt->Halt();
}

void Server() {
	int farAddr;
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *reply;
    //char buffer[MaxMailSize];
	char buffer[] = "mon set 348 5 62";
	
	char* obj;
	char* act;
	char* param1;
	char* param2;
	char* param3;
	char temp;
	int counter = 0;
	int maxParamSize = 64;
	
	// Syscall Params that will be filled with client message data
	int clientID;
	int lockIndex;
	int cvIndex;
	int mvIndex;
	int length;
	int value;
	int rv = -1;		// Return value from syscall
	
	// Need to add error handling for wrong # params
	while (true) {
		// Receive message from client (other machine)
		postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
		printf("Server: Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from,inMailHdr.from);
		fflush(stdout);
		
		clientID = inMailHdr.from;
		
		// PARSE MESSAGE
		// Message Format:
		// "ObjActParams"
		// "Obj" = Object
		// "Act" = Actions to apply to object
		// "Params" = parameters needed by the appropriate syscall
		
		char* data;
		
		data = strtok(buffer, " "); // Splits spaces between words in buffer
		obj = data;
		
		if (strcmp(data, "loc") == 0) {
			data = strtok (NULL, " ,.-");
			act = data;
			if (strcmp(data, "cre") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				
				length = strlen(param1);
				
				printf("Server: Lock Create, machine = %d, name = %s\n", clientID, param1);
				//rv = ServerCreateLock(param1, length);
			} else if (strcmp(data, "acq") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				
				printf("Server: Lock Acquire, machine = %d, index = %s\n", clientID, param1);
				//rv = ServerAcquire(clientID, param1);
			} else if (strcmp(data, "rel") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				
				lockIndex = atoi(param1);
				
				printf("Server: Lock Release, machine = %d, index = %s\n", clientID, param1);
				//rv = ServerRelease(clientID, lockIndex);
			} else if (strcmp(data, "des") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				
				lockIndex = atoi(param1);
				
				printf("Server: Lock Destroy, machine = %d, index = %s\n", clientID, param1);
				//rv = ServerDestroyLock(clientID, lockIndex);
			} else {
				printf("Server: Bad request.\n");
			}
		} else if (strcmp(data, "con") == 0) {
			data = strtok (NULL, " ,.-");
			act = data;
			if (strcmp(data, "cre") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				
				length = strlen(param1);
				
				printf("Server: Condition Create, machine = %d, name = %s\n", clientID, param1);
				//rv = ServerCreateCV(param1, length);
			} else if (strcmp(data, "wai") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				cvIndex = atoi(param1);
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				lockIndex = atoi(param2);
				
				printf("Server: Condition Wait, machine = %d, cv = %s, lock = %s\n", clientID, param1, param2);
				//rv = ServerWait(clientID, cvIndex, lockIndex);
			} else if (strcmp(data, "sig") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				cvIndex = atoi(param1);
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				lockIndex = atoi(param2);
				
				printf("Server: Condition Signal, machine = %d, cv = %s, lock = %s\n", clientID, param1, param2);
				//rv = ServerSignal(clientID, cvIndex, lockIndex);
			} else if (strcmp(data, "bro") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				cvIndex = atoi(param1);
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				lockIndex = atoi(param2);
				
				printf("Server: Condition Broadcast, machine = %d, cv = %s, lock = %s\n", clientID, param1, param2);
				//rv = ServerBroadcast(clientID, cvIndex, lockIndex);
				// Loop through all of CV's waiting Machines and call Signal syscall
			} else if (strcmp(data, "del") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				cvIndex = atoi(param1);
				
				printf("Server: Condition Delete, machine = %d, cv = %s", clientID, param1);
				//rv = ServerDestroyCV(clientID, cvIndex);
			} else {
				printf("Server: Bad request.\n");
			}
		} else if (strcmp(data, "mon") == 0) {
			data = strtok (NULL, " ,.-");
			act = data;
			if (strcmp(data, "cre") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				length = strlen(param1);
				
				printf("Server: MV Create, machine = %d, name = %s\n", clientID, param1);
				//rv = ServerCreateMV(param1, length);
			} else if (strcmp(data, "get") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				length = strlen(param1);
				
				printf("Server: MV Get, machine = %d, name = %s\n", clientID, param1);
				//rv = ServerGetMV(clientID, mvIndex);
			} else if (strcmp(data, "set") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				value = atoi(param2);
				
				printf("Server: MV Set, machine = %d, index = %s, value = %s\n", clientID, param1, param2);
				//rv = ServerSetMV(clientID, param1, value);
			} else if (strcmp(data, "des") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				// length too?
				
				printf("Server: MV Destroy, machine = %d, name = %s\n", clientID, param1);
				//rv = ServerDestroyMV(clientID, param1);
			} else {
				printf("Server: Bad request.\n");
			}
		} else {
			printf("Server: Bad request.\n");
			rv = 0x9000;//BAD_REQUEST;
		}
		
		printf("Server: forming reply.\n");
		
		// Create reply for client
		//itoa(rv, reply, 10);	// needs library
		//sscanf(reply, "%d", &rv);	// used wrong char buffer
		//reply = reinterpret_cast<char *>(rv);		// segfault
		reply = "hardcoded";
		printf("Reply = %s.\n", reply);
		
		// construct packet, mail header for original message
		// To: destination machine, mailbox clientID
		// From: our machine, reply to: mailbox 0
		
		outPktHdr.to = clientID;		
		outMailHdr.to = clientID;
		outPktHdr.from = 0;
		outMailHdr.from = 0;
		outMailHdr.length = strlen(data) + 1;

		printf("Server: sending reply.\n");
		
		// Send reply to client
		bool success = postOffice->Send(outPktHdr, outMailHdr, reply); 
		
		printf("Server: sent reply.\n");
		
		if ( !success ) {
			printf("Server: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		interrupt->Halt();
	}
}

void Client(int farAddr) {
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "loc acq 3 8";
    char *ack = "Client: Got it!";
    char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = 0;		
    outMailHdr.to = 0;
    outMailHdr.from = farAddr;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("Client: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	// Wait for server reply
	// Wait for the first message from the other machine
    postOffice->Receive(farAddr, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);
	
	
	// BAD REQUEST TEST
	data = "derp";
	
	// construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = 0;		
    outMailHdr.to = 0;
    outMailHdr.from = farAddr;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("Client: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
/*
    // Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Client: Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(ack) + 1;
    success = postOffice->Send(outPktHdr, outMailHdr, ack); 

    if ( !success ) {
      printf("Client: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the ack from the other machine to the first message we sent.
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("Client: Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);*/

    // Then we're done!
    interrupt->Halt();
}

void LockTest (int farAddr) {
	if (farAddr == 0) {
		Client(farAddr);
		return;
	}
	Server();
}

void parseServerFunction(){
	/*int farAddr;
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Server: Hello thar! How can I help you?";
    char *ack = "Server: Okay, I'll be right with you!";
    char buffer[MaxMailSize];
	
	//put parsing here for server
	// Receive message from client (other machine)
	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
	printf("Server: Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
	fflush(stdout);*/
}
