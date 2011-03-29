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
		exists = false;
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
	
	ServerMV() {
		name = NULL;
		value = 0;
	}
};

ServerLock serverLocks[MAX_LOCKS];
ServerCV serverCVs[MAX_CONDITIONS];
ServerMV serverMVs[MAX_MVS];

void ServerReply(int clientID, int rv) {
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char reply[10];
	
	/*int neg = 0;
	if (rv < 0) {
		neg = 1;
	}
	
	int thousands = abs((rv % 10000)/1000);
	int hundreds = abs((rv % 1000)/100);
	int tens = abs((rv % 100)/10);
	int ones = abs((rv % 10));*/
	
	//printf("Server: neg: %d, thousands: %d, hundreds: %d, tens: %d, ones: %d\n", neg, thousands, hundreds, tens, ones);
	
	//sprintf(reply, "%d%d%d%d%d", neg, thousands, hundreds, tens, ones);
	sprintf(reply, "%d", rv);
	
	printf("Server: reply array: %s\n", reply);
	
	// construct packet, mail header for original message
	// To: destination machine, mailbox clientID
	// From: our machine, reply to: mailbox 0
	
	outPktHdr.to = clientID;
	outPktHdr.from = 0;
	outMailHdr.to = 0;
	outMailHdr.from = 0;
	outMailHdr.length = strlen(reply) + 1;
	
	printf("Server: sending reply.\n");
	
	// Send reply to client
	bool success = postOffice->Send(outPktHdr, outMailHdr, reply); 
	
	printf("Server: sent reply.\n");
	
	if ( !success ) {
		printf("Server: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}
}

void CreateLock_RPC(char* name, int machineID) {
	//If reached max lock capacity, return -1
	if (numServerLocks >= MAX_LOCKS) {
		printf("Server - CreateLock_RPC: Max server lock limit reached, cannot create.\n");
		
		//SEND ERROR MESSAGE BACK
		ServerReply(machineID, NO_SPACE);
		return;
	}
	
	for (int i = 0; i < MAX_LOCKS; i++) {
		//Check if lock already exists
		if ( strcmp(name, serverLocks[i].name) == 0 && serverLocks[i].exists) {
			
			//printf("%s and %s\n", serverLocks[i].name, name);
			//If it does, check to see if this machine has already created it
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverLocks[i].clientID[j] == machineID) {
					printf("Server - CreateLock_RPC: Machine%d has already created this lock.\n", machineID);
					
					//SEND INDEX i IN MESSAGE BACK
					ServerReply(machineID, i);
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
					ServerReply(machineID, i);
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
			strcpy(serverLocks[i].name, name);
			
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverLocks[i].clientID[j] == 0) {
					serverLocks[i].clientID[j] = machineID;
					serverLocks[i].numClients++;
					
					//SEND INDEX i IN MESSAGE
					ServerReply(machineID, i);
					return;
				}
			}
		}	 
	}
	
	//Should never reach here
	return;
}

void Acquire_RPC(int lockIndex, int machineID) {
	if (lockIndex < 0) {
		printf("Server - Acquire_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex >= MAX_LOCKS) {
		printf("Server - Acquire_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("Server - Acquire_RPC: Machine%d trying to acquire non-existant ServerLock%d\n", machineID, lockIndex);
		
		// SEND ERROR MESSAGE BACK
		ServerReply(machineID, DELETED);
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
		ServerReply(machineID, NOT_CREATED);
		return;
	}
	
	//If already owner, return 0
	if (serverLocks[lockIndex].holder == machineID) {
		printf("Server - Acquire_RPC: Machine%d is already the owner of ServerLock%d\n", machineID, lockIndex);
		
		//SEND MESSAGE BACK
		ServerReply(machineID, 0);
		return;
	}
	
	if (serverLocks[lockIndex].holder == -1) {
		serverLocks[lockIndex].holder = machineID;
		
		//SEND MESSAGE BACK
		ServerReply(machineID, 0);
		return;
	}
	else {
		serverLocks[lockIndex].queue->Append((void*)machineID);
		
		//DONT SEND MESSAGE
		
		return;
	}
}

void Release_RPC(int lockIndex, int machineID) {
	if (lockIndex < 0) {
		printf("Server - Release_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex >= MAX_LOCKS) {
		printf("Server - Release_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("Server - Release_RPC: Machine%d trying to release non-existant ServerLock%d\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		ServerReply(machineID, DELETED);
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
		ServerReply(machineID, NOT_CREATED);
		return;
	}
	
	//If not owner, return -1
	if (serverLocks[lockIndex].holder != machineID) {
		printf("Server - Release_RPC: Machine%d is not the owner of ServerLock%d\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		ServerReply(machineID, NOT_OWNER);
		return;
	}
	
	//SEND ACTUAL MESSAGE BACK TO machineID
	ServerReply(machineID, 0);
	
	if (serverLocks[lockIndex].queue->IsEmpty()) {
		serverLocks[lockIndex].holder = -1;
		ServerReply(machineID, 0);
		return;
	}
	
	int nextToAcquire = (int)serverLocks[lockIndex].queue->Remove();
	
	serverLocks[lockIndex].holder = nextToAcquire;
	
	//SEND MESSAGE TO nextToAcquire
	ServerReply(nextToAcquire, 0);
	return;
}

void DestroyLock_RPC(int lockIndex, int machineID) {
	if (lockIndex < 0) {
		printf("Server - DestroyLock_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex >= MAX_LOCKS) {
		printf("Server - DestroyLock_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	//If this lock doesn't exist, return
	if (!serverLocks[lockIndex].exists) {
		printf("Server - DestroyLock_RPC: Machine%d trying to destroy non-existant ServerLock%d\n", machineID, lockIndex);
		
		//SEND ERROR MESSAGE BACK
		ServerReply(machineID, DELETED);
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
		ServerReply(machineID, NOT_CREATED);
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
	ServerReply(machineID, 0);
	
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
		ServerReply(nextToAcquire, 0);
		return;
	}
	
	return;
}

void CreateCV_RPC(char* name, int machineID) {
	//If reached max cv capacity, return -1
	if (numServerCVs >= MAX_CONDITIONS) {
		printf("Server - CreateCV_RPC: Max server cv limit reached, cannot create.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NO_SPACE);
		return;
	}
		
	for (int i = 0; i < MAX_CONDITIONS; i++) {
		//Check if condition already exists
		if ( (strcmp(name, serverCVs[i].name)) == 0 && serverCVs[i].exists) {
			
			//If it does, check to see if this machine has already created it
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverCVs[i].clientID[j] == machineID) {
					printf("Server - CreateCV_RPC: Machine%d has already created this cv.\n", machineID);
					
					// SEND BACK MESSAGE
					ServerReply(machineID, i);
					return;
				}
			}
			// If this machine hasn't created it, add the ID to the CV's list
			// and increment number of clients, then return the CV index
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverCVs[i].clientID[j] == 0) {
					serverCVs[i].clientID[j] = machineID;
					serverCVs[i].numClients++;

					// SEND i IN MESSAGE BACK
					ServerReply(machineID, i);
					return;
				}
			}
		}
	}
	
	// If CV doesn't exist, create it and set the name
	// Find an open space in the CV's client list, add this machine
	// and return the CV index
	for (int i = 0; i < MAX_CONDITIONS; i++) {
		if (!serverCVs[i].exists) {
			numServerCVs++;
			serverCVs[i].exists = true;
			strcpy(serverCVs[i].name, name);
			//serverCVs[i].name = name;
			
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverCVs[i].clientID[j] == 0) {
					serverCVs[i].clientID[j] = machineID;
					serverCVs[i].numClients++;
					// SEND i IN MESSAGE BACK
					ServerReply(machineID, i);
					return;
				}
			}
		}	 
	}
}

void Wait_RPC(int conditionIndex, int lockIndex, int machineID) {
	if (conditionIndex < 0) {
		printf("Server - Wait_RPC: CV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (conditionIndex >= MAX_CONDITIONS) {
		printf("Server - Wait_RPC: CV index >= MAX_CVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex < 0) {
		printf("Server - Wait_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex >= MAX_LOCKS) {
		printf("Server - Wait_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}

	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("Server - Wait_RPC: Machine%d trying to wait on non-existant ServerLock%d\n", machineID, lockIndex);
		ServerReply(machineID, DELETED);
		// SEND BACK ERROR MESSAGE

		return;
	}
	
	//If condition doesn't exist, return -1
	if (!serverCVs[conditionIndex].exists) {
		printf("Server - Wait_RPC: Machine%d trying to wait on non-existant ServerCV%d\n", machineID, conditionIndex);
		ServerReply(machineID, DELETED);
		// SEND BACK ERROR MESSAGE

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
		printf("Server - Wait_RPC: Machine%d trying to wait on ServerLock%d that has not been 'created'.\n", machineID, lockIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_CREATED);
		return;
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
		printf("Server - Wait_RPC: Machine%d trying to wait on ServerCV%d that has not been 'created'\n", machineID, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_CREATED);
		return;
	}	
	
	// If waitingLock is -1, set it to the passed in lock
	// Else if lock is wrong, return -1
	if (serverCVs[conditionIndex].waitingLock == -1) {
		serverCVs[conditionIndex].waitingLock = lockIndex;
	}
	else if (serverCVs[conditionIndex].waitingLock != lockIndex) {
		printf("Server - Wait_RPC: Machine%d trying to wait on wrong ServerLock%d in ServerCV%d\n", machineID, lockIndex, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	
	//Needs to be the holder for the lock
	if (serverLocks[lockIndex].holder != machineID) {
		printf("Server - Wait_RPC: Machine%d is not the holder of ServerLock%d\n", machineID, lockIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_OWNER);
		return;
	}
	
	// Actually wait now
	serverCVs[conditionIndex].queue->Append((void*)machineID);
	
	// Return 0 if no locks are going to acquire the lock
	// after this one releases it
	if (serverLocks[lockIndex].queue->IsEmpty()) {
		serverLocks[lockIndex].holder = -1;		
		ServerReply(machineID, 0);
		return;
	}
	
	int nextToAcquire = (int)serverLocks[lockIndex].queue->Remove();
	
	serverLocks[lockIndex].holder = nextToAcquire;
	
	//Otherwise return the new lock holder
	//return serverLocks[lockIndex].holder;

	// SEND MESSAGE TO nextToAcquire
	ServerReply(nextToAcquire, 0);

	return;
}

void Signal_RPC(int conditionIndex, int lockIndex, int machineID) {
	if (conditionIndex < 0) {
		printf("Server - Signal_RPC: CV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (conditionIndex >= MAX_CONDITIONS) {
		printf("Server - Signal_RPC: CV index >= MAX_CVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex < 0) {
		printf("Server - Signal_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex >= MAX_LOCKS) {
		printf("Server - Signal_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}

	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("Server - Signal_RPC: Machine%d trying to signal on non-existant ServerLock%d\n", machineID, lockIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, DELETED);
		return;
	}
	
	//If condition doesn't exist, return -1
	if (!serverCVs[conditionIndex].exists) {
		printf("Server - Signal_RPC: Machine%d trying to signal on non-existant ServerCV%d\n", machineID, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, DELETED);
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
		printf("Server - Signal_RPC: Machine%d trying to signal on ServerLock%d that has not been 'created'.\n", machineID, lockIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_CREATED);
		return;
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
		printf("Server - Signal_RPC: Machine%d trying to signal on ServerCV%d that has not been 'created'\n", machineID, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_CREATED);
		return;
	}	
	
	// If waitingLock is -1, set it to the passed in lock
	// Else if lock is wrong, return -1
	if (serverCVs[conditionIndex].waitingLock != lockIndex) {
		printf("Server - Signal_RPC: Machine%d trying to signal on wrong ServerLock%d in ServerCV%d\n", machineID, lockIndex, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	
	//Needs to be the holder for the lock
	if (serverLocks[lockIndex].holder != machineID) {
		printf("Server - Signal_RPC: Machine%d is not the owner of ServerLock%d\n", machineID, lockIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_OWNER);
		return;
	}

	// Queue must not be empty
	if (serverCVs[conditionIndex].queue->IsEmpty()) {
		printf("Server - Signal_RPC: Condition queue is empty. Nothing waiting.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}

	int nextWaiting = (int)serverCVs[conditionIndex].queue->Remove();

	if (serverCVs[conditionIndex].queue->IsEmpty()) {
		printf("Server - Signal_RPC: Condition queue is now empty. Nothing waiting.\n");
		serverCVs[conditionIndex].waitingLock = -1;	
	}
	
	if (serverLocks[lockIndex].holder == -1) {
		serverLocks[lockIndex].holder = nextWaiting;

		// SEND MESSAGE TO nextWaiting
		ServerReply(0, nextWaiting);
	}
	else {
		serverLocks[lockIndex].queue->Append((void*)nextWaiting);
	}		
	
	ServerReply(machineID, 0);
}

void Broadcast_RPC(int conditionIndex, int lockIndex, int machineID) {
	if (conditionIndex < 0) {
		printf("Server - Broadcast_RPC: CV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (conditionIndex >= MAX_CONDITIONS) {
		printf("Server - Broadcast_RPC: CV index >= MAX_CVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex < 0) {
		printf("Server - Broadcast_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (lockIndex >= MAX_LOCKS) {
		printf("Server - Broadcast_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	//If this lock doesn't exist, return -1
	if (!serverLocks[lockIndex].exists) {
		printf("Server - Signal_RPC: Machine%d trying to signal on non-existant ServerLock%d\n", machineID, lockIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, DELETED);
		return;
	}
	
	//If condition doesn't exist, return -1
	if (!serverCVs[conditionIndex].exists) {
		printf("Server - Signal_RPC: Machine%d trying to signal on non-existant ServerCV%d\n", machineID, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, DELETED);
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
		printf("Server - Signal_RPC: Machine%d trying to signal on ServerLock%d that has not been 'created'.\n", machineID, lockIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_CREATED);
		return;
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
		printf("Server - Signal_RPC: Machine%d trying to signal on ServerCV%d that has not been 'created'\n", machineID, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_CREATED);
		return;
	}	
	
	// If waitingLock is -1, set it to the passed in lock
	// Else if lock is wrong, return -1
	if (serverCVs[conditionIndex].waitingLock != lockIndex) {
		printf("Server - Signal_RPC: Machine%d trying to signal on wrong ServerLock%d in ServerCV%d\n", machineID, lockIndex, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	
	//Needs to be the holder for the lock
	if (serverLocks[lockIndex].holder != machineID) {
		printf("Server - Signal_RPC: Machine%d is not the owner of ServerLock%d\n", machineID, lockIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_OWNER);
		return;
	}

	// Queue must not be empty
	if (serverCVs[conditionIndex].queue->IsEmpty()) {
		printf("Server - Signal_RPC: Condition queue is empty. Nothing waiting.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	
	while (!serverCVs[conditionIndex].queue->IsEmpty()) {
	
		int nextWaiting = (int)serverCVs[conditionIndex].queue->Remove();
		
		if (serverLocks[lockIndex].holder == -1) {
			serverLocks[lockIndex].holder = nextWaiting;

			// SEND MESSAGE TO nextWaiting
			ServerReply(0, nextWaiting);
		}
		else {
			serverLocks[lockIndex].queue->Append((void*)nextWaiting);
		}		
	}

	// SEND MESSAGE BACK TO machineID
	ServerReply(machineID, 0);
	printf("Server - Signal_RPC: Condition queue is now empty. Nothing waiting.\n");
	serverCVs[conditionIndex].waitingLock = -1;	
}

void DestroyCV_RPC(int conditionIndex, int machineID) {
	if (conditionIndex < 0) {
		printf("Server - DestroyCV_RPC: CV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (conditionIndex >= MAX_CONDITIONS) {
		printf("Server - DestroyCV_RPC: CV index >= MAX_CVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}

	//If this CV doesn't exist, return
	if (!serverCVs[conditionIndex].exists) {
		printf("Server - DestroyCV_RPC: Machine%d trying to destroy non-existant ServerCV%d\n", machineID, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, DELETED);
		return;
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
		printf("Server - DestroyCV_RPC: Machine%d trying to destroy ServerLock%d that has not been 'created'.\n", machineID, conditionIndex);

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NOT_CREATED);
		return;
	}
	
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[conditionIndex].clientID[i] == machineID) {
			serverCVs[conditionIndex].clientID[i] = 0;
			break;
		}
	}
	serverCVs[conditionIndex].numClients--;
	
	// SEND BACK MESSAGE
	ServerReply(machineID, 0);
	
	//If no more clients, delete lock
	if (serverCVs[conditionIndex].numClients == 0) {
		serverCVs[conditionIndex].exists = false;
		serverCVs[conditionIndex].name = "";
		numServerCVs--;

		return;
	}
}

void CreateMV_RPC(char* name, int val, int machineID) {
	if (numMVs >= MAX_MVS) {
		printf("Server - CreateMV__RPC: Error: Number of Monitor Vars exceeded maximum Monitor Vars limit.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, NO_SPACE);
		return;
	}

	int index = -1;
	for (int i = 0; i < MAX_MVS; i++) {
		if(strcmp(serverMVs[i].name, name) == 0){
			if (val == 0x9999) {
				printf("Server - CreateMV_Syscall: Cannot set MV to reserved \"uninitialized\" value.\n");
				
				// SEND i BACK IN MESSAGE
				ServerReply(machineID, i);
				return;
			}
			else{
				serverMVs[i].value = val;

				// SEND i BACK IN MESSAGE
				ServerReply(machineID, i);
				return;
			}
		}
	}

	// Otherwise, it doesn't exist
	// find 1st vacancy in the list
	for (int i = 0; i < MAX_MVS; i++) {
		if(serverMVs[i].name == NULL){
			//serverMVs[i].name = name;
			strcpy(serverMVs[i].name, name);
			if (val == 0x9999) {
				printf("Server - CreateMV_Syscall: Cannot set MV to reserved \"uninitialized\" value.\n");
				
				// SEND i BACK IN MESSAGE
				ServerReply(machineID, i);
				return;
			}
			else{
				serverMVs[i].value = val;

				// SEND i BACK IN MESSAGE
				ServerReply(machineID, i);
				return;
			}
		}
	}	
}

void GetMV_RPC(int index, int machineID) {
	if (index < 0) {
		printf("Server - GetMV_RPC: MV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (index >= MAX_MVS) {
		printf("Server - GetMV_RPC: MV index >= MAX_MVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	
	int val = serverMVs[index].value;
	
	// SEND BACK val IN MESSAGE
	ServerReply(val, machineID);
}

void SetMV_RPC(int index, int val, int machineID) {
	if (index < 0) {
		printf("Server - SetMV_RPC: MV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (index >= MAX_MVS) {
		printf("Server - SetMV_RPC: MV index >= MAX_MVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	if (val == 0x9999) {
		printf("Server - GetMV_RPC: Cannot set MV to reserved \"uninitialized\" value.\n");

		// SEND BACK ERROR MESSAGE
		ServerReply(machineID, BAD_INDEX);
		return;
	}
	
	serverMVs[index].value = val;

	// SEND BACK MESSAGE
	ServerReply(machineID, 0);
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
    //char *reply;
	
	/*char* obj;
	char* act;
	char* param1;
	char* param2;
	char* param3;*/
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
		char buffer[MaxMailSize];
		char* obj;
		char* act;
		char* param1;
		char* param2;
		char* param3;
		// Receive message from client (other machine)
		postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
		printf("Server: Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from,inMailHdr.from);
		fflush(stdout);
		
		clientID = inPktHdr.from;
		
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
				CreateLock_RPC(param1, clientID);
			} else if (strcmp(data, "acq") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				lockIndex = atoi(param1);
				
				printf("Server: Lock Acquire, machine = %d, index = %s\n", clientID, param1);
				Acquire_RPC(lockIndex, clientID);
			} else if (strcmp(data, "rel") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				lockIndex = atoi(param1);
				
				printf("Server: Lock Release, machine = %d, index = %s\n", clientID, param1);
				Release_RPC(lockIndex, clientID);
			} else if (strcmp(data, "des") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				lockIndex = atoi(param1);
				
				printf("Server: Lock Destroy, machine = %d, index = %s\n", clientID, param1);
				DestroyLock_RPC(lockIndex, clientID);
			} else {
				printf("Server: Bad request.\n");
				ServerReply(clientID, BAD_FORMAT);
			}
		} else if (strcmp(data, "con") == 0) {
			data = strtok (NULL, " ,.-");
			act = data;
			if (strcmp(data, "cre") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				length = strlen(param1);
				
				printf("Server: Condition Create, machine = %d, name = %s\n", clientID, param1);
				CreateCV_RPC(param1, clientID);
			} else if (strcmp(data, "wai") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				cvIndex = atoi(param1);
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				lockIndex = atoi(param2);
				
				printf("Server: Condition Wait, machine = %d, cv = %s, lock = %s\n", clientID, param1, param2);
				Wait_RPC(cvIndex, lockIndex, clientID);
			} else if (strcmp(data, "sig") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				cvIndex = atoi(param1);
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				lockIndex = atoi(param2);
				
				printf("Server: Condition Signal, machine = %d, cv = %s, lock = %s\n", clientID, param1, param2);
				Signal_RPC(cvIndex, lockIndex, clientID);
			} else if (strcmp(data, "bro") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				cvIndex = atoi(param1);
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				lockIndex = atoi(param2);
				
				printf("Server: Condition Broadcast, machine = %d, cv = %s, lock = %s\n", clientID, param1, param2);
				Broadcast_RPC(cvIndex, lockIndex, clientID);
			} else if (strcmp(data, "del") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				cvIndex = atoi(param1);
				
				printf("Server: Condition Delete, machine = %d, cv = %s", clientID, param1);
				DestroyCV_RPC(cvIndex, clientID);
			} else {
				printf("Server: Bad request.\n");
				ServerReply(clientID, BAD_FORMAT);
			}
		} else if (strcmp(data, "mon") == 0) {
			data = strtok (NULL, " ,.-");
			act = data;
			if (strcmp(data, "cre") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				value = atoi(param2);
				
				printf("Server: MV Create, machine = %d, name = %s, val = %d\n", clientID, param1, value);
				CreateMV_RPC(param1, value, clientID);
			} else if (strcmp(data, "get") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				mvIndex = atoi(param1);
				
				printf("Server: MV Get, machine = %d, name = %s\n", clientID, param1);
				GetMV_RPC(mvIndex, clientID);
			} else if (strcmp(data, "set") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				mvIndex = atoi(param1);
				
				data = strtok (NULL, " ,.-");
				param2 = data;
				value = atoi(param2);
				
				printf("Server: MV Set, machine = %d, index = %s, value = %s\n", clientID, mvIndex, value);
				SetMV_RPC(mvIndex, value, clientID);
			/*} else if (strcmp(data, "des") == 0) {
				data = strtok (NULL, " ,.-");
				param1 = data;
				// length too?
				
				printf("Server: MV Destroy, machine = %d, name = %s\n", clientID, param1);
				//rv = ServerDestroyMV(clientID, param1);*/
			} else {
				printf("Server: Bad request.\n");
				ServerReply(clientID, BAD_FORMAT);
			}
		} else {
			printf("Server: Bad request.\n");
			ServerReply(clientID, BAD_FORMAT);
		}
	}
}

void Client(int farAddr) {
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "lac acq 3 8";
    char *ack = "Client: Got it!";
    char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = 0;
    outMailHdr.to = 0;
    outMailHdr.from = 0;
    outMailHdr.length = strlen(data) + 1;
	
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 
	
    if ( !success ) {
      printf("Client: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
	
	// Wait for server reply
	// Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);
	
	int rv = 0;
	
	/*char temp[2];
	temp[0] = buffer[0];
	int neg = atoi(temp);
	
	temp[0] = buffer[1];
	int thousands = atoi(temp);
	
	temp[0] = buffer[2];
	int hundreds = atoi(temp);
	
	temp[0] = buffer[3];
	int tens = atoi(temp);
	
	temp[0] = buffer[4];
	int ones = atoi(temp);
	
	rv = (thousands * 1000) + (hundreds * 100) + (tens * 10) + ones;
	
	if (neg == 1) {
		rv *= -1;
	}*/
	rv = atoi(buffer);
	printf("Client: rv = %d\n", rv);
	
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
