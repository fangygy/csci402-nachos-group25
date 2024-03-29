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
#include <time.h>
#include <cmath>

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
#define ARRAY_MAX 64

//struct timeval tv; 
//struct timezone tz; 
//struct tm *tm; 
//gettimeofday(&tv, &tz); 
//tm = localtime(&tv.tv_sec); 

int numServerLocks = 0;
int numServerCVs = 0;
int numMVs = 0;

struct Holder {
	int machineID;
	int mailboxID;
	
	Holder() {
		machineID = -1;
		mailboxID = -1;
	}
};
	
struct InnerLock {
	bool exists;
	Holder holder;
	List *queue;
	List *machineIDQueue;
	List *mailboxIDQueue;
	Holder clientID[MAX_CLIENTS];
	int numClients;
	
	InnerLock() {
		exists = false;
		queue = new List;
		machineIDQueue = new List;
		mailboxIDQueue = new List;
		numClients = 0;
	}
};

struct ServerLock {
	char* name;
	bool exists;
	InnerLock lock[ARRAY_MAX];

	ServerLock(){
		name = "";
		exists = false;
	}
};

struct InnerCV {
	bool exists;
	int waitingOuterLock;
	int waitingInnerLock;
	List* queue;
	List *machineIDQueue;
	List *mailboxIDQueue;
	Holder clientID[MAX_CLIENTS];
	int numClients;

	InnerCV() {
		exists = false;
		waitingOuterLock = -1;
		waitingInnerLock = -1;
		queue = new List;
		machineIDQueue = new List;
		mailboxIDQueue = new List;
		numClients = 0;
	}
};

struct ServerCV {
	char* name;
	bool exists;
	InnerCV cv[ARRAY_MAX];

	ServerCV() {
		name = "";
		exists = false;
	}
};

struct ServerMV {
	char* name;
	int value[ARRAY_MAX];
	
	ServerMV() {
		name = "";
		for (int i = 0; i < ARRAY_MAX; i++)
			value[i] = 0;
	}
};

ServerLock serverLocks[MAX_LOCKS];
ServerCV serverCVs[MAX_CONDITIONS];
ServerMV serverMVs[MAX_MVS];

struct Message {
	int clientMachineID;
	int clientMailboxID;
	unsigned int timestamp;
	char* message;
	bool sender;
	
	Message() {
		clientMachineID = -1;
		clientMailboxID = -1;
		timestamp = 0;
		message = "";
		sender = false;
	}
	
	Message(int machID, int mailID, unsigned int time, char* mess, bool send) {
		clientMachineID = machID;
		clientMailboxID = mailID;
		timestamp = time;
		message = new char[strlen(mess)+1];
		strcpy(message, mess);
		sender = send;
	}
};

List *messageQ;			// Message Queue

char* reverseString(char* myWord) {
	int len = strlen(myWord);
	char* newWord = new char[len + 1];

	for (int i = 0; i < len; i++) {
		newWord[i] = myWord[len - i - 1];
	}

	newWord[len] = '\0';
	return newWord;
}

char* convertDecToBase(unsigned int num, int base) {
	// can go up to base-72
	//ASCII 126 = '~', 127 = 'DEL'
	if (base <= 10) {
		printf("convertDecToBase only converts base-10 to base greater than 10.\n");
		return NULL;
	}
	if (num == 0) {
		char* buffer = new char[2];
		buffer[0] = (char)(NUM_OFFSET);
		buffer[1] = '\0';
		return buffer;
	}
	
	int index = 0;
	int remainder;
	
	char* buffer = new char[11];
	
	while (num != 0) {
		remainder = num % base;
		num = num / base;

		if (remainder < 10) {
			buffer[index] = (char)(NUM_OFFSET + remainder);//(remainder);
		} else {
			buffer[index] = (char)(CHAR_OFFSET + remainder - 10);//(remainder);
		}
		index++;
		
		// It's MATHEMATICALLY IMPOSSIBLE to get more digits when converting from (base 10) to (base > 10).
		// But, since one of our members is so anal about it...
		if (index > 9) {
			printf("Told you we shoulda made it bigger than 10.\n");
			interrupt->Halt();
		}
	}

	// cutoff char array with null terminator
	buffer[index] = '\0';

	char* newBuffer = reverseString(buffer);
	delete buffer;

	return newBuffer;
}

unsigned int convertBaseToDec(char* numArray, float base) {
	unsigned int decimal = 0;
	int len = strlen(numArray);
	int actual;
	
	for (int i = 0; i < len; i++) {
		actual = numArray[i];
		if (actual <= (NUM_OFFSET + 9)) {
			actual -= NUM_OFFSET;
		} else {
			actual -= CHAR_OFFSET;
			actual += 10;
		}

		decimal += actual * pow(base, (len-i - 1));
		//printf("actual: %d, exp: %d\n", actual, (len - i - 1));
	}
	
	return decimal;
}


unsigned int getTimestamp() {
	struct timeval tv; 
	struct timezone tz; 
	struct tm *tm; 
	gettimeofday(&tv, &tz); 
	tm=localtime(&tv.tv_sec); 
	unsigned int myTimestamp = ((unsigned int)(tv.tv_usec + (tv.tv_sec - lastServerSeconds)*1000000)); 
	
	return myTimestamp;
}

void ServerReply(int clientID, int mailboxID, int rv) {
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
	
	printf("Server: reply array: %s to clientID%d and clientMailboxID%d\n", reply, clientID, mailboxID);
	
	// construct packet, mail header for original message
	// To: destination machine, mailbox clientID
	// From: our machine, reply to: mailbox 0
	
	outPktHdr.to = clientID;
	outPktHdr.from = 0;
	outMailHdr.to = mailboxID;
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

void CreateLock_RPC(bool sender, char* name, int arraySize, int machineID, int mailboxID) {
	//If reached max lock capacity, return -1
	printf("Server - CreateLock_RPC: name = %s\n", name);
	
	if (numServerLocks >= MAX_LOCKS) {
		printf("Server - CreateLock_RPC: Max server lock limit reached, cannot create.\n");
		
		//SEND ERROR MESSAGE BACK
		if (sender)
			ServerReply(machineID, mailboxID, NO_SPACE);
		return;
	}
	
	for (int i = 0; i < MAX_LOCKS; i++) {
		//Check if lock already exists
		//printf("Lock%d name: %s\n", i, serverLocks[i].name);
		if ( strcmp(name, serverLocks[i].name) == 0 && serverLocks[i].exists) {
			
			//printf("%s and %s\n", serverLocks[i].name, name);
			//If it does, check to see if this machine has already created it
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverLocks[i].lock[0].clientID[j].machineID == machineID &&
					serverLocks[i].lock[0].clientID[j].mailboxID == mailboxID) {
					
					printf("Server - CreateLock_RPC: Machine%d has already created this lock.\n", machineID);
					
					//SEND INDEX i IN MESSAGE BACK
					if (sender)
						ServerReply(machineID, mailboxID, i);
					return;
				}
			}
			
			//If this machine hasn't created it, add the ID to the lock's list
			// and increment number of clients, then return the lock index
			for (int j = 0; j < arraySize; j++) {
				for (int k = 0; k < MAX_CLIENTS; k++) {
					if (serverLocks[i].lock[j].clientID[k].machineID == -1 &&
						serverLocks[i].lock[j].clientID[k].mailboxID == -1){
						
						serverLocks[i].lock[j].clientID[k].machineID = machineID;
						serverLocks[i].lock[j].clientID[k].mailboxID = mailboxID;
						//serverLocks[i].lock[j].numClients++;
						
						//SEND INDEX i IN MESSAGE BACK
						//ServerReply(machineID, mailboxID, i);
						//return;
						//interrupt->Halt();
						break;
					}
				}
				serverLocks[i].lock[j].numClients++;
			}
			if (sender)
				ServerReply(machineID, mailboxID, i);
			return;
		}
	}
	
	//If lock doesn't exist, create it and set the name
	// Find an open space in the lock's client list, add this machine
	// and return the lock index
	for (int i = 0; i < MAX_LOCKS; i++) {
		//printf("Lock%d name: %s\n", i, serverLocks[i].name);
		if ( strcmp( "", serverLocks[i].name) == 0 || !serverLocks[i].exists) {
			/*for (int p = 0; p < MAX_LOCKS; p++)
				if (serverLocks[p].exists)
					printf("Lock%d name: %s\n", p, serverLocks[p].name);*/
			printf("Creating new lock\n");
			serverLocks[i].exists = true;
			numServerLocks++;
			
			/*for (int p = 0; p < 10; p++)
				printf("Lock%d name: %s\n", p, serverLocks[p].name);
			
			for (int p = 0; p < 10; p++)
				printf("Lock%d name: %s\n", p, serverLocks[p].name);*/
			//serverLocks[i].name = tempName;
			
			serverLocks[i].name = new char[strlen(name)+1];
			strcpy(serverLocks[i].name, name);
			//serverLocks[i].name = name;
			//strcpy(serverLocks[i].name, tempName);
			
			/*for (int p = 0; p < MAX_LOCKS; p++)
				if ( strcmp( serverLocks[p].name, serverLocks[i].name) == 0 && p != i) {
					serverLocks[p].name = "";
				}*/
			
			/*for (int p = 0; p < 10; p++)
				printf("Lock%d name: %s\n", p, serverLocks[p].name);*/
			
			for (int j = 0; j < arraySize; j++) {
				serverLocks[i].lock[j].exists = true;
				
				for (int k = 0; k < MAX_CLIENTS; k++) {
					if (serverLocks[i].lock[j].clientID[k].machineID == -1 &&
						serverLocks[i].lock[j].clientID[k].mailboxID == -1) {
						
						serverLocks[i].lock[j].clientID[k].machineID = machineID;
						serverLocks[i].lock[j].clientID[k].mailboxID = mailboxID;
						//serverLocks[i].lock[j].numClients++;
						break;
					}
				}
				
				serverLocks[i].lock[j].numClients++;
			}
			
			/*for (int p = 0; p < MAX_LOCKS; p++)
				if (serverLocks[p].exists)
					printf("Lock%d name: %s\n", p, serverLocks[p].name);*/
			if (sender)
				ServerReply(machineID, mailboxID, i);
			return;

		}	 
	}
	
	printf("Reached here?\n");
	//Should never reach here
	return;
}

void Acquire_RPC(bool sender, int outerLockIndex, int innerLockIndex, int machineID, int mailboxID) {
	if (outerLockIndex < 0 || innerLockIndex < 0) {
		printf("Server - Acquire_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex >= MAX_LOCKS || innerLockIndex >= ARRAY_MAX) {
		printf("Server - Acquire_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	//If this lock doesn't exist, return -1
	if (!serverLocks[outerLockIndex].lock[innerLockIndex].exists) {
		printf("Server - Acquire_RPC: Machine%d trying to acquire non-existant ServerLock%d\n", machineID, outerLockIndex);
		
		// SEND ERROR MESSAGE BACK
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		return;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID == machineID &&
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID == mailboxID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Acquire_RPC: Machine%d trying to acquire ServerLock%d that has not been 'created'.\n", machineID, outerLockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}
	
	//If already owner, return 0
	if (serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID == machineID &&
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID == mailboxID) {
		printf("Server - Acquire_RPC: Machine%d is already the owner of ServerLock%d %s\n", machineID, outerLockIndex, serverLocks[outerLockIndex].name);
		
		//SEND MESSAGE BACK
		if (sender)
			ServerReply(machineID, mailboxID, 0);
		return;
	}
	
	if (serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID == -1 &&
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID == -1) {
		
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID = machineID;
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID = mailboxID;
		
		//SEND MESSAGE BACK
		if (sender)
			ServerReply(machineID, mailboxID, 0);
		return;
	}
	else {
		
		Holder queueHolder;
		queueHolder.machineID = machineID;
		queueHolder.mailboxID = mailboxID;
		//printf("Server - Acquire_RPC: machineID%d and mailboxID%d\n", machineID, mailboxID);
		//serverLocks[outerLockIndex].lock[innerLockIndex].queue->Append((void*)queueHolder);
		serverLocks[outerLockIndex].lock[innerLockIndex].machineIDQueue->Append((void*)queueHolder.machineID);
		serverLocks[outerLockIndex].lock[innerLockIndex].mailboxIDQueue->Append((void*)queueHolder.mailboxID);
		
		//DONT SEND MESSAGE
		
		return;
	}
}

void Release_RPC(bool sender, int outerLockIndex, int innerLockIndex, int machineID, int mailboxID) {
	if (outerLockIndex < 0 || innerLockIndex < 0) {
		printf("Server - Release_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex >= MAX_LOCKS || innerLockIndex >= ARRAY_MAX) {
		printf("Server - Release_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	//If this lock doesn't exist, return -1
	if (!serverLocks[outerLockIndex].lock[innerLockIndex].exists) {
		printf("Server - Release_RPC: Machine%d trying to release non-existant ServerLock%d\n", machineID, outerLockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		return;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID == machineID &&
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID == mailboxID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Release_RPC: Machine%d trying to release ServerLock%d that has not been 'created'.\n", machineID, outerLockIndex);
		
		//SEND ERROR MESSAGE BACK HERE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}
	
	//If not owner, return -1
	if (serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID != machineID ||
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID != mailboxID) {
		printf("Server - Release_RPC: Machine%d is not the owner of ServerLock%d %s\n", machineID, outerLockIndex, serverLocks[outerLockIndex].name);
		
		//SEND ERROR MESSAGE BACK HERE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_OWNER);
		return;
	}
	
	//SEND ACTUAL MESSAGE BACK TO machineID
	if (sender)
		ServerReply(machineID, mailboxID, 0);
	
	if (serverLocks[outerLockIndex].lock[innerLockIndex].machineIDQueue->IsEmpty() &&
		serverLocks[outerLockIndex].lock[innerLockIndex].mailboxIDQueue->IsEmpty()) {
		
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID = -1;
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID = -1;
		//ServerReply(machineID, mailboxID, 0);
		return;
	}
	
	/*if (serverLocks[outerLockIndex].lock[innerLockIndex].machineIDQueue->IsEmpty()) {
		printf("MACHINEIDQUEUE IS EMPTY\n");
	}
	if (serverLocks[outerLockIndex].lock[innerLockIndex].mailboxIDQueue->IsEmpty()) {
		printf("MAILBOXIDQUEUE IS EMPTY\n");
	}*/
	
	//Holder nextToAcquire = (Holder)serverLocks[outerLockIndex].lock[innerLockIndex].queue->Remove();
	Holder nextToAcquire;
	nextToAcquire.machineID = (int)serverLocks[outerLockIndex].lock[innerLockIndex].machineIDQueue->Remove();
	nextToAcquire.mailboxID = (int)serverLocks[outerLockIndex].lock[innerLockIndex].mailboxIDQueue->Remove();
	//printf("Server - Acquire_RPC: machineID%d and mailboxID%d\n", nextToAcquire.machineID, nextToAcquire.mailboxID);
	serverLocks[outerLockIndex].lock[innerLockIndex].holder = nextToAcquire;
	
	//SEND MESSAGE TO nextToAcquire
	if (sender)
		ServerReply(nextToAcquire.machineID, nextToAcquire.mailboxID, 0);
	return;
}

void DestroyLock_RPC(bool sender, int outerLockIndex, int innerLockIndex, int machineID, int mailboxID) {
	if (outerLockIndex < 0 || innerLockIndex < 0) {
		printf("Server - DestroyLock_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex >= MAX_LOCKS || innerLockIndex >= ARRAY_MAX) {
		printf("Server - DestroyLock_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	//If this lock doesn't exist, return
	if (!serverLocks[outerLockIndex].lock[innerLockIndex].exists) {
		printf("Server - DestroyLock_RPC: Machine%d trying to destroy non-existant ServerLock%d\n", machineID, outerLockIndex);
		
		//SEND ERROR MESSAGE BACK
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		return;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID == machineID &&
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID == mailboxID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - DestroyLock_RPC: Machine%d trying to destroy ServerLock%d that has not been 'created'.\n", machineID, outerLockIndex);
		
		//SEND ERROR MESSAGE BACK
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}
	
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID == machineID &&
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID == mailboxID) {
			
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID = 0;
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID = 0;
			break;
		}
	}
	serverLocks[outerLockIndex].lock[innerLockIndex].numClients--;
	
	//SEND MESSAGE BACK TO machineID
	if (sender)
		ServerReply(machineID, mailboxID, 0);
	
	//If no more clients, delete lock
	if (serverLocks[outerLockIndex].lock[innerLockIndex].numClients == 0) {
		
		serverLocks[outerLockIndex].lock[innerLockIndex].exists = false;
		
		bool allEmpty = true;
		for (int i = 0; i < ARRAY_MAX; i++) {
			if (serverLocks[outerLockIndex].lock[i].exists) {
				allEmpty = false;
				break;
			}
		}
		
		if (allEmpty) {
			serverLocks[outerLockIndex].name = "";
			serverLocks[outerLockIndex].exists = false;
			numServerLocks--;
		}
		return;
	}
	
	//If this client is current owner, release it
	/*if (serverLocks[lockIndex].holder == machineID) {
		if (serverLocks[lockIndex].queue->IsEmpty()) {
			serverLocks[lockIndex].holder = -1;
			return;
		}
		
		int nextToAcquire = (int)serverLocks[lockIndex].queue->Remove();
	
		serverLocks[lockIndex].holder = nextToAcquire;
		
		//SEND MESSAGE TO nextToAcquire
		ServerReply(nextToAcquire, 0);
		return;
	}*/
	
	return;
}

void CreateCV_RPC(bool sender, char* name, int arraySize, int machineID, int mailboxID) {
	//If reached max cv capacity, return -1
	if (numServerCVs >= MAX_CONDITIONS) {
		printf("Server - CreateCV_RPC: Max server cv limit reached, cannot create.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NO_SPACE);
		return;
	}
		
	for (int i = 0; i < MAX_CONDITIONS; i++) {
		//Check if condition already exists
		if ( (strcmp(name, serverCVs[i].name)) == 0 && serverCVs[i].exists) {
			
			//If it does, check to see if this machine has already created it
			for (int j = 0; j < MAX_CLIENTS; j++) {
				if (serverCVs[i].cv[0].clientID[j].machineID == machineID &&
					serverCVs[i].cv[0].clientID[j].mailboxID == mailboxID) {
					printf("Server - CreateCV_RPC: Machine%d has already created this cv.\n", machineID);
					
					// SEND BACK MESSAGE
					if (sender)
						ServerReply(machineID, mailboxID, i);
					//interrupt->Halt();
					return;
				}
			}
			// If this machine hasn't created it, add the ID to the CV's list
			// and increment number of clients, then return the CV index
			for (int j = 0; j < arraySize; j++) {
				for (int k = 0; k < MAX_CLIENTS; k++) {
					if (serverCVs[i].cv[j].clientID[k].machineID == -1 &&
						serverCVs[i].cv[j].clientID[k].mailboxID == -1){
						
						serverCVs[i].cv[j].clientID[k].machineID = machineID;
						serverCVs[i].cv[j].clientID[k].mailboxID = mailboxID;
						//serverCVs[i].cv[j].numClients++;
						
						//SEND INDEX i IN MESSAGE BACK
						//interrupt->Halt();
						break;
					}
				}
				serverCVs[i].cv[j].numClients++;
			}
			
			if (sender)
				ServerReply(machineID, mailboxID, i);
			return;
		}
	}
	
	// If CV doesn't exist, create it and set the name
	// Find an open space in the CV's client list, add this machine
	// and return the CV index
	for (int i = 0; i < MAX_CONDITIONS; i++) {
		if ( strcmp( "", serverCVs[i].name) == 0 || !serverCVs[i].exists) {
			numServerCVs++;
			serverCVs[i].name = new char[strlen(name)+1];
			strcpy(serverCVs[i].name, name);
			serverCVs[i].exists = true;
			//serverCVs[i].name = name;
			
			for (int j = 0; j < arraySize; j++) {
				serverCVs[i].cv[j].exists = true;
				serverCVs[i].cv[j].numClients++;
				
				for (int k = 0; k < MAX_CLIENTS; k++) {
					if (serverCVs[i].cv[j].clientID[k].machineID == -1 &&
						serverCVs[i].cv[j].clientID[k].mailboxID == -1) {
						
						serverCVs[i].cv[j].clientID[k].machineID = machineID;
						serverCVs[i].cv[j].clientID[k].mailboxID = mailboxID;
						break;
					}
				}
			}
			
			if (sender)
				ServerReply(machineID, mailboxID, i);
			return;
		}	 
	}
}

void Wait_RPC(bool sender, int outerConditionIndex, int innerConditionIndex, int outerLockIndex, int innerLockIndex, int machineID, int mailboxID) {
	if (outerConditionIndex < 0 || innerConditionIndex < 0) {
		printf("Server - Wait_RPC: CV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerConditionIndex >= MAX_CONDITIONS || innerConditionIndex >= ARRAY_MAX) {
		printf("Server - Wait_RPC: CV index >= MAX_CVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex < 0 || innerLockIndex < 0) {
		printf("Server - Wait_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex >= MAX_LOCKS || innerLockIndex >= ARRAY_MAX) {
		printf("Server - Wait_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}

	//If this lock doesn't exist, return -1
	if (!serverLocks[outerLockIndex].lock[innerLockIndex].exists) {
		printf("Server - Wait_RPC: Machine%d trying to wait on non-existant ServerLock%d\n", machineID, outerLockIndex);
		
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		// SEND BACK ERROR MESSAGE

		return;
	}
	
	//If condition doesn't exist, return -1
	if (!serverCVs[outerConditionIndex].cv[innerConditionIndex].exists) {
		printf("Server - Wait_RPC: Machine%d trying to wait on non-existant ServerCV%d\n", machineID, outerConditionIndex);
		
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		// SEND BACK ERROR MESSAGE

		return;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID == machineID &&
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID == mailboxID) {
			
			isClient = true;
			break;
		}
		else if( serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID == -1 ||
				serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID == -1) {
				
				printf("machine%d and mailbox%d\n", serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID, serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID);
		}
	}
	if (!isClient) {
		printf("Server - Wait_RPC: Machine%d trying to wait on ServerLock%d that has not been 'created'.\n", machineID, outerLockIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}
	
	//Same for condition, make sure is client
	isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].machineID == machineID &&
			serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].mailboxID == mailboxID) {
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Wait_RPC: Machine%d trying to wait on ServerCV%d that has not been 'created'\n", machineID, outerConditionIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}	
	
	// If waitingLock is -1, set it to the passed in lock
	// Else if lock is wrong, return -1
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock == -1 &&
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock == -1) {
		
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock = outerLockIndex;
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock = innerLockIndex;
	}
	else if (serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock != outerLockIndex ||
			serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock != innerLockIndex) {
		printf("Server - Wait_RPC: Machine%d trying to wait on wrong ServerLock%d in ServerCV%d\n", machineID, outerLockIndex, outerConditionIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	
	//Needs to be the holder for the lock
	if (serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID != machineID ||
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID != mailboxID) {
		printf("Server - Wait_RPC: Machine%d is not the holder of ServerLock%d\n", machineID, outerLockIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_OWNER);
		return;
	}
	
	// Actually wait now
	Holder waitingClient;
	waitingClient.machineID = machineID;
	waitingClient.mailboxID = mailboxID;
	//serverCVs[outerConditionIndex].cv[innerConditionIndex].queue->Append((void*)waitingClient);
	serverCVs[outerConditionIndex].cv[innerConditionIndex].machineIDQueue->Append((void*)waitingClient.machineID);
	serverCVs[outerConditionIndex].cv[innerConditionIndex].mailboxIDQueue->Append((void*)waitingClient.mailboxID);
	
	// Return 0 if no locks are going to acquire the lock
	// after this one releases it
	if (serverLocks[outerLockIndex].lock[innerLockIndex].machineIDQueue->IsEmpty() &&
		serverLocks[outerLockIndex].lock[innerLockIndex].mailboxIDQueue->IsEmpty() ) {
		
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID = -1;	
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID = -1;		
		//ServerReply(machineID, mailboxID, 0);
		return;
	}
	
	//Holder nextToAcquire = (Holder)serverLocks[outerLockIndex].lock[innerLockIndex].queue->Remove();
	Holder nextToAcquire;
	nextToAcquire.machineID = (int)serverLocks[outerLockIndex].lock[innerLockIndex].machineIDQueue->Remove();
	nextToAcquire.mailboxID = (int)serverLocks[outerLockIndex].lock[innerLockIndex].mailboxIDQueue->Remove();
	
	serverLocks[outerLockIndex].lock[innerLockIndex].holder = nextToAcquire;
	
	//Otherwise return the new lock holder
	//return serverLocks[lockIndex].holder;

	// SEND MESSAGE TO nextToAcquire
	if (sender)
		ServerReply(nextToAcquire.machineID, nextToAcquire.mailboxID, 0);

	return;
}

void Signal_RPC(bool sender, int outerConditionIndex, int innerConditionIndex, int outerLockIndex, int innerLockIndex, int machineID, int mailboxID) {
	if (outerConditionIndex < 0 || innerConditionIndex < 0) {
		printf("Server - Signal_RPC: CV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerConditionIndex >= MAX_CONDITIONS || innerConditionIndex >= ARRAY_MAX) {
		printf("Server - Signal_RPC: CV index >= MAX_CVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex < 0 || innerLockIndex < 0) {
		printf("Server - Signal_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex >= MAX_LOCKS || innerLockIndex >= ARRAY_MAX) {
		printf("Server - Signal_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}

	//If this lock doesn't exist, return -1
	if (!serverLocks[outerLockIndex].lock[innerLockIndex].exists) {
		printf("Server - Signal_RPC: Machine%d trying to signal on non-existant ServerLock%d\n", machineID, outerLockIndex);
		printf("MachineID:%d, MailboxID:%d, lockName:%s, conName:%s\n", machineID, mailboxID, serverLocks[outerLockIndex].name, serverCVs[outerLockIndex].name);
		//interrupt->Halt();

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		return;
	}
	
	//If condition doesn't exist, return -1
	if (!serverCVs[outerConditionIndex].cv[innerConditionIndex].exists) {
		printf("Server - Signal_RPC: Machine%d trying to signal on non-existant ServerCV%d\n", machineID, outerConditionIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		return;
	}

	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID == machineID &&
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID == mailboxID) {
			
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Signal_RPC: Machine%d trying to signal on ServerLock%d that has not been 'created'.\n", machineID, outerLockIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}

	//Same for condition, make sure is client
	isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].machineID == machineID &&
			serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].mailboxID == mailboxID) {
			
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Signal_RPC: Machine%d trying to signal on ServerCV%d that has not been 'created'\n", machineID, outerConditionIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}	
	
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock == -1 &&
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock == -1) {
		
		//printf("Server - Signal_RPC: 
		if (sender)
			ServerReply(machineID, mailboxID, 0);
		return;
	}
	
	
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock != outerLockIndex ||
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock != innerLockIndex) {
		printf("Server - Signal_RPC: Machine%d trying to signal on wrong ServerLock%d in ServerCV%d\n", machineID, outerLockIndex, outerConditionIndex);
		printf("Actual owners is %s\n", serverLocks[serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock].name);
		printf("instead of %s\n", serverLocks[outerLockIndex].name);
		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	
	//Needs to be the holder for the lock
	if (serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID != machineID ||
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID != mailboxID) {
		printf("Server - Signal_RPC: Machine%d is not the owner of ServerLock%d\n", machineID, outerLockIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_OWNER);
		return;
	}

	// Queue must not be empty
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].machineIDQueue->IsEmpty() &&
		serverCVs[outerConditionIndex].cv[innerConditionIndex].mailboxIDQueue->IsEmpty()) {
		printf("Server - Signal_RPC: Condition queue is empty. Nothing waiting.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}

	//Holder nextWaiting = (Holder)serverCVs[outerConditionIndex].cv[innerConditionIndex].queue->Remove();
	Holder nextWaiting;
	nextWaiting.machineID = (int)serverCVs[outerConditionIndex].cv[innerConditionIndex].machineIDQueue->Remove();
	nextWaiting.mailboxID = (int)serverCVs[outerConditionIndex].cv[innerConditionIndex].mailboxIDQueue->Remove();
	
	
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].machineIDQueue->IsEmpty() &&
		serverCVs[outerConditionIndex].cv[innerConditionIndex].mailboxIDQueue->IsEmpty()) {
		printf("Server - Signal_RPC: Condition queue is now empty. Nothing waiting.\n");
		
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock = -1;	
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock = -1;	
	}
	
	if (serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID == -1 &&
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID == -1) {
		
		serverLocks[outerLockIndex].lock[innerLockIndex].holder = nextWaiting;

		// SEND MESSAGE TO nextWaiting
		if (sender)
			ServerReply(nextWaiting.machineID, nextWaiting.mailboxID, 0);
	}
	else {
		//serverLocks[outerLockIndex].lock[innerLockIndex].queue->Append((void*)nextWaiting);
		serverLocks[outerLockIndex].lock[innerLockIndex].machineIDQueue->Append((void*)nextWaiting.machineID);
		serverLocks[outerLockIndex].lock[innerLockIndex].mailboxIDQueue->Append((void*)nextWaiting.mailboxID);
	}		
	
	if (sender)
		ServerReply(machineID, mailboxID, 0);
}

void Broadcast_RPC(bool sender, int outerConditionIndex, int innerConditionIndex, int outerLockIndex, int innerLockIndex, int machineID, int mailboxID) {
	if (outerConditionIndex < 0 || innerConditionIndex < 0) {
		printf("Server - Broadcast_RPC: CV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerConditionIndex >= MAX_CONDITIONS || innerConditionIndex >= ARRAY_MAX) {
		printf("Server - Broadcast_RPC: CV index >= MAX_CVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex < 0 || innerLockIndex < 0) {
		printf("Server - Broadcast_RPC: Lock index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerLockIndex >= MAX_LOCKS || innerLockIndex >= ARRAY_MAX) {
		printf("Server - Broadcast_RPC: Lock index >= MAX_LOCKS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	//If this lock doesn't exist, return -1
	if (!serverLocks[outerLockIndex].lock[innerLockIndex].exists) {
		printf("Server - Signal_RPC: Machine%d trying to signal on non-existant ServerLock%d\n", machineID, outerLockIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		return;
	}
	
	//If condition doesn't exist, return -1
	if (!serverCVs[outerConditionIndex].cv[innerConditionIndex].exists) {
		printf("Server - Signal_RPC: Machine%d trying to signal on non-existant ServerCV%d\n", machineID, outerConditionIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		return;
	}

	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].machineID == machineID &&
			serverLocks[outerLockIndex].lock[innerLockIndex].clientID[i].mailboxID == mailboxID) {
			
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Signal_RPC: Machine%d trying to signal on ServerLock%d that has not been 'created'.\n", machineID, outerLockIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}

	//Same for condition, make sure is client
	isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].machineID == machineID &&
			serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].mailboxID == mailboxID) {
			
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - Signal_RPC: Machine%d trying to signal on ServerCV%d that has not been 'created'\n", machineID, outerConditionIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}	
	
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock == -1 &&
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock == -1) {
		
		//printf("Server - Signal_RPC: 
		if (sender)
			ServerReply(machineID, mailboxID, 0);
		return;
	}
	
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock != outerLockIndex ||
		serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock != innerLockIndex) {
		printf("Server - Broadcast_RPC: Machine%d trying to signal on wrong ServerLock%d in ServerCV%d\n", machineID, outerLockIndex, outerConditionIndex);
		printf("Actual owners is %s\n", serverLocks[serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock].name);
		printf("instead of %s\n", serverLocks[outerLockIndex].name);
		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	
	//Needs to be the holder for the lock
	if (serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID != machineID ||
		serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID != mailboxID) {
		printf("Server - Signal_RPC: Machine%d is not the owner of ServerLock%d\n", machineID, outerLockIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_OWNER);
		return;
	}

	// Queue must not be empty
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].machineIDQueue->IsEmpty() &&
		serverCVs[outerConditionIndex].cv[innerConditionIndex].mailboxIDQueue->IsEmpty()) {
		printf("Server - Signal_RPC: Condition queue is empty. Nothing waiting.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	
	while (!serverCVs[outerConditionIndex].cv[innerConditionIndex].machineIDQueue->IsEmpty() ||
			!serverCVs[outerConditionIndex].cv[innerConditionIndex].mailboxIDQueue->IsEmpty()) {
	
		//Holder nextWaiting = (Holder)serverCVs[outerConditionIndex].cv[innerConditionIndex].queue->Remove();
		Holder nextWaiting;
		nextWaiting.machineID = (int)serverCVs[outerConditionIndex].cv[innerConditionIndex].machineIDQueue->Remove();
		nextWaiting.mailboxID = (int)serverCVs[outerConditionIndex].cv[innerConditionIndex].mailboxIDQueue->Remove();
	
		if (serverLocks[outerLockIndex].lock[innerLockIndex].holder.machineID == -1 &&
			serverLocks[outerLockIndex].lock[innerLockIndex].holder.mailboxID == -1) {
			
			serverLocks[outerLockIndex].lock[innerLockIndex].holder = nextWaiting;

			// SEND MESSAGE TO nextWaiting
			if (sender)
				ServerReply(nextWaiting.machineID, nextWaiting.mailboxID, 0);
		}
		else {
			//serverLocks[outerLockIndex].lock[innerLockIndex].queue->Append((void*)nextWaiting);
			serverLocks[outerLockIndex].lock[innerLockIndex].machineIDQueue->Append((void*)nextWaiting.machineID);
			serverLocks[outerLockIndex].lock[innerLockIndex].mailboxIDQueue->Append((void*)nextWaiting.mailboxID);
	
		}		
	}

	// SEND MESSAGE BACK TO machineID
	if (sender)
		ServerReply(machineID, mailboxID, 0);
	printf("Server - Signal_RPC: Condition queue is now empty. Nothing waiting.\n");
	serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingOuterLock = -1;
	serverCVs[outerConditionIndex].cv[innerConditionIndex].waitingInnerLock = -1;
}

void DestroyCV_RPC(bool sender, int outerConditionIndex, int innerConditionIndex, int machineID, int mailboxID) {
	if (outerConditionIndex < 0 || innerConditionIndex < 0) {
		printf("Server - DestroyCV_RPC: CV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerConditionIndex >= MAX_CONDITIONS || innerConditionIndex >= ARRAY_MAX) {
		printf("Server - DestroyCV_RPC: CV index >= MAX_CVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}

	//If this CV doesn't exist, return
	if (!serverCVs[outerConditionIndex].cv[innerConditionIndex].exists) {
		printf("Server - DestroyCV_RPC: Machine%d trying to destroy non-existant ServerCV%d\n", machineID, outerConditionIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, DELETED);
		return;
	}
	
	//Make sure this machine is a client of the lock
	bool isClient = false;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].machineID == machineID &&
			serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].mailboxID == mailboxID) {
			
			isClient = true;
			break;
		}
	}
	if (!isClient) {
		printf("Server - DestroyCV_RPC: Machine%d trying to destroy ServerLock%d that has not been 'created'.\n", machineID, outerConditionIndex);

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NOT_CREATED);
		//interrupt->Halt();
		return;
	}
	
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].machineID == machineID &&
			serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].mailboxID == mailboxID) {
			
			serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].machineID = -1;
			serverCVs[outerConditionIndex].cv[innerConditionIndex].clientID[i].mailboxID = -1;
			break;
		}
	}
	serverCVs[outerConditionIndex].cv[innerConditionIndex].numClients--;
	
	// SEND BACK MESSAGE
	if (sender)
		ServerReply(machineID, mailboxID, 0);
	
	//If no more clients, delete lock
	if (serverCVs[outerConditionIndex].cv[innerConditionIndex].numClients == 0) {
	
		serverCVs[outerConditionIndex].cv[innerConditionIndex].exists = false;
		
		bool allEmpty = true;
		for (int i = 0; i < ARRAY_MAX; i++) {
			if (serverCVs[outerConditionIndex].cv[i].exists) {
				allEmpty = false;
				break;
			}
		}
		
		if (allEmpty) {
			serverCVs[outerConditionIndex].name = "";
			numServerCVs--;
			serverCVs[outerConditionIndex].exists = false;
		}

		return;
	}
}

void CreateMV_RPC(bool sender, char* name, int arraySize, int val, int machineID, int mailboxID) {
	if (numMVs >= MAX_MVS) {
		printf("Server - CreateMV__RPC: Error: Number of Monitor Vars exceeded maximum Monitor Vars limit.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, NO_SPACE);
		return;
	}
	
	printf("CreateMV\n");

	for (int i = 0; i < MAX_MVS; i++) {
		if(strcmp(serverMVs[i].name, name) == 0){
			if (val == 0x9999) {
				printf("Server - CreateMV_Syscall: Cannot set MV to reserved \"uninitialized\" value.\n");
				
				// SEND i BACK IN MESSAGE
				if (sender)
					ServerReply(machineID, mailboxID, i);
				return;
			}
			else{
				for (int j = 0; j < arraySize; j++) {
					serverMVs[i].value[j] = val;
				}
				// SEND i BACK IN MESSAGE
				if (sender)
					ServerReply(machineID, mailboxID, i);
				return;
			}
		}
	}

	// Otherwise, it doesn't exist
	// find 1st vacancy in the list
	for (int i = 0; i < MAX_MVS; i++) {
		if(strcmp(serverMVs[i].name, "") == 0){
			//serverMVs[i].name = name;
			serverMVs[i].name = new char[strlen(name)+1];
			strcpy(serverMVs[i].name, name);
			if (val == 0x9999) {
				printf("Server - CreateMV_Syscall: Cannot set MV to reserved \"uninitialized\" value.\n");
				
				// SEND i BACK IN MESSAGE
				if (sender)
					ServerReply(machineID, mailboxID, i);
				return;
			}
			else{
				for (int j = 0; j < arraySize; j++) {
					serverMVs[i].value[j] = val;
				}

				// SEND i BACK IN MESSAGE
				if (sender)
					ServerReply(machineID, mailboxID, i);
				return;
			}
		}
	}	
}

void GetMV_RPC(bool sender, int outerIndex, int innerIndex, int machineID, int mailboxID) {
	if (outerIndex < 0 || innerIndex < 0) {
		printf("Server - GetMV_RPC: MV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerIndex >= MAX_MVS || innerIndex >= ARRAY_MAX) {
		printf("Server - GetMV_RPC: MV index >= MAX_MVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	
	int val = serverMVs[outerIndex].value[innerIndex];
	
	// SEND BACK val IN MESSAGE
	if (sender)
		ServerReply(machineID, mailboxID, val);
}

void SetMV_RPC(bool sender, int outerIndex, int innerIndex, int val, int machineID, int mailboxID) {
	if (outerIndex < 0 || innerIndex < 0) {
		printf("Server - SetMV_RPC: MV index less than zero. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (outerIndex >= MAX_MVS || innerIndex >= ARRAY_MAX) {
		printf("Server - SetMV_RPC: MV index >= MAX_MVS. Invalid.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	if (val == 0x9999) {
		printf("Server - GetMV_RPC: Cannot set MV to reserved \"uninitialized\" value.\n");

		// SEND BACK ERROR MESSAGE
		if (sender)
			ServerReply(machineID, mailboxID, BAD_INDEX);
		return;
	}
	
	serverMVs[outerIndex].value[innerIndex] = val;

	// SEND BACK MESSAGE
	if (sender)
		ServerReply(machineID, mailboxID, 0);
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

void TransServer() {
	PacketHeader inPktHdr;
    MailHeader inMailHdr;
	
	unsigned int timestamp;
	int fwdMachineID;
	int fwdMailboxID;
	int clientMachineID;
	int clientMailboxID;
	
	while (true) {
		PacketHeader outPktHdr;
		MailHeader outMailHdr;
		char msg[MaxMailSize];
		postOffice->Receive(0, &inPktHdr, &inMailHdr, msg);
		fflush(stdout);
		
		printf("TransServer: Got \"%s\" from %d, box %d\n", msg, inPktHdr.from,inMailHdr.from);
		
		char* fwdmsg = "";
		char* timechar = "";
		char* msgData = "";
		/*char* msg = "";*/
		
		// extract timestamp
		printf("TransServer: Extracting Timestamp.\n");
		timechar = strtok(msg, " "); // Splits spaces between words in buffer
		timestamp = convertBaseToDec(timechar, 72);
		
		msgData = strtok(NULL, "");		// Get the rest of the msg string, don't split it
		
		printf("TransServer: Checking for Server Update message.\n");
		
		if (strcmp(msgData, "su") == 0) {
			// If the incoming message was just an UPDATE SERVER SECONDS message,
			// Just update my lastServerSeconds
			lastServerSeconds = timestamp;
			printf("TransServer: UPDATED Last Server Second: %u\n", lastServerSeconds);
		} else {
			// forward the message to all servers
			fwdMachineID = netname;
			fwdMailboxID = 0;
			clientMachineID = inPktHdr.from;
			clientMailboxID = inMailHdr.from;
			
			sprintf(fwdmsg, "%s %d %d %s", timechar, clientMachineID, clientMailboxID, msgData);
			
			for (int i = 0; i < NUM_SERVERS; i++) {
				outPktHdr.to = i;
				outMailHdr.to = 1;
				outMailHdr.from = 0;
				outMailHdr.length = strlen(fwdmsg) + 1;
				
				postOffice->Send(outPktHdr, outMailHdr, fwdmsg);
			}
		}
	}
}

void SendTimestamp(unsigned int timestamp) {
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char reply[MaxMailSize];
	
	char* timechar = convertDecToBase(timestamp, 72);
	sprintf(reply, "%s ts", timechar);
	
	/*printf("Server: reply array: %s to clientID%d and clientMailboxID%d\n", reply, clientID, mailboxID);*/
	
	// construct packet, mail header for original message
	// To: destination machine, mailbox clientID
	// From: our machine, reply to: mailbox 0
	
	outPktHdr.to = netname;
	outPktHdr.from = netname;
	outMailHdr.to = 0;
	outMailHdr.from = 1;
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

void UpdateServerSeconds() {
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char reply[30];
	
	outPktHdr.from = netname;
	outMailHdr.from = 1;
	
	outMailHdr.length = strlen(reply) + 1;
	
	struct timeval tv;
	lastServerSeconds = tv.tv_sec;
	
	sprintf(reply, "%u su", lastServerSeconds);
	printf("Server: NEWLY CREATED lastServerSeconds: %u\n",
		lastServerSeconds);
	
	// Send this lastServerSeconds value to all preceding Servers
	for (int i = 0; i < netname; i++) {
		outPktHdr.to = i;
		outMailHdr.to = 0;
		
		bool success = postOffice->Send(outPktHdr, outMailHdr, reply); 
	
		printf("Server: Server Seconds Update msg to MachID: %d.\n", i);
	
		if ( !success ) {
			printf("Server: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
	}
}

void Server() {
	
	/*
	printf("Server: HEX TEST\n");
	char* derp = new char[32];
	sprintf(derp, "%x\0", "1111111111111111111111111111111111111111");
	
	printf("Server: derp char*: %s\n", derp);
	
	int derpint = atoi(derp);
	printf("Server: derp int: %d\n", derpint);
	
	printf("Server: END HEX TEST\n");
	*/
	
	Thread *t;
	t = new Thread("TransServer");
	t->Fork((VoidFunctionPtr) TransServer, 0);
	currentThread->Yield();
	
	for (int i = 0; i < 20; i++) {
		unsigned int myTimestamp = getTimestamp();
		printf("Server: Test: current timestamp: %u\n", myTimestamp);
	}
	
	
	UpdateServerSeconds();
	
	printf("Starting Server.\n");
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
	int lockIndex;
	int lockIndex2;
	int cvIndex;
	int cvIndex2;
	int mvIndex;
	int mvIndex2;
	int length;
	int arraySize;
	int value;
	int rv = -1;		// Return value from syscall
	
	// Total Ordering Algorithm data
	unsigned int timestamp;
	int fwdMachineID;
	int fwdMailboxID;
	
	int clientMailboxID;
	int clientMachineID;
	
	unsigned int smallestTimestamp;
	int smallestMachineID;
	
	Message* head;
	unsigned int LTR[NUM_SERVERS];
	
	// Initialize LTR Table
	for(int i = 0; i < NUM_SERVERS; i++) {
		LTR[i] = 0;
	}
	
	// Initialize Message Queue
	messageQ = new List;
	
	while (true) {
		char buffer[MaxMailSize];
		char* obj = "";
		char* act = "";
		char* param1 = "";
		char* param2 = "";
		char* param3 = "";
		char* param4 = "";
		char* msg = "";
		bool sender = false;
		// Receive message from client (other machine)
		
		printf("Server: Waiting to receive message\n");
		postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
		printf("Server: Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from,inMailHdr.from);
		fflush(stdout);
		
		printf("Server: Starting to parse message.\n");
		
		char* fwdData = "";
		fwdData = strtok(buffer, " "); // Splits spaces between words in buffer
		
		// Step 1.) Extract TS & forwarding server machineID
		timestamp = convertBaseToDec(fwdData, 72);//atoi(fwdData);
		fwdData = strtok(NULL, " ");
		
		fwdMachineID = inPktHdr.from;
		
		fwdMailboxID = inMailHdr.from;
		
		clientMachineID = atoi(fwdData);
		fwdData = strtok(NULL, " ");
		
		clientMailboxID = atoi(fwdData);
		fwdData = strtok(NULL, "");
		
		//msg = fwdData;
		strcpy(msg, fwdData);
		
		
		
		// if my machineID == forwarder's machineID, then I send reply to the client
		if (netname == fwdMachineID)
			sender = true;
		else 
			sender = false;
		
		printf("Server: Starting Step 2: Message Q.\n");
		// Step 2.) If not a TS msg, put msg into messageQ
		if (strcmp(msg, "ts") != 0) {
			printf("Server: Step 2 Not a TS message. Append to Message Q.\n");
			//printf("Server: msg: %s\n", msg);
			Message* newMessage = new Message(clientMachineID, clientMailboxID, timestamp, msg, sender);
			//printf("Server: msg: %s\n", newMessage->message);
			printf("Server: Made the message object, Appending to Message Q.\n");
			messageQ->SortedInsert((void*)newMessage, timestamp);
		}
		
		printf("Server: Starting Step 3: Update LTR.\n");
		// Step 3.) Update LTR Table
		LTR[fwdMachineID] = timestamp;
		
		printf("Server: Starting Step 4: Smallest TS.\n");
		// Step 4.) Scan LTR Table and extract smallest TS
		smallestMachineID = 0;
		smallestTimestamp = LTR[0];
		for (int i = 0; i < NUM_SERVERS; i++) {
			if (LTR[i] < smallestTimestamp) {
				smallestMachineID = i;
				smallestTimestamp = LTR[i];
			}
		}
		
		printf("Server: Starting Step 6: Get 1st TS in Message Q.\n");
		// Step 6.) Get TS and machineID from 1st msg in messageQ
		if (!messageQ->IsEmpty()) {
			head = (Message*)messageQ->Remove();
		}
		else {
			head = NULL;
		}
		
		printf("Server: Starting Step 7: Deciding whether or not to send TS msg.\n");
		// Step 7.) If not a TS msg, tell trans-server to fwd TS msg to all other Servers
		if (strcmp(msg, "ts") != 0) {
			printf("Did not segfault1\n");
			//time_t currTime;
			//time (&currTime);
			//char *currentTime = ctime(&currTime);
			//unsigned int myTimestamp = ((unsigned int)(tv.tv_usec + tv.tv_sec*1000000));
			unsigned int myTimestamp = getTimestamp();
			//int intCurrTime = atoi(currentTime);
			SendTimestamp(myTimestamp);
		}
		printf("Did not segfault2\n");
	
		if (head == NULL) {
			continue;
		}
		printf("Server: Comparing timestamps, head:%u and smallest:%u\n", head->timestamp, smallestTimestamp);
		if (head->timestamp > smallestTimestamp) {
			messageQ->Prepend((void*)head);
		}
		// Need while loop here: while (head ID & timestamp <= smallest ID & timestamp)
		while (head->timestamp <= smallestTimestamp) {
			char* currentMsg = new char[strlen(head->message) + 1];
			strcpy(currentMsg, head->message);
			printf("Server: currentMsg: %s\n", currentMsg);
			
			printf("Server: Before splitting in Parsing\n");

		
			char* data = "";
			data = strtok(currentMsg, " "); // Splits spaces between words in currentMsg
			obj = data;
			
			printf("Server: Before 'if' checking\n");
			printf("Server: currentMsg: %s\n", currentMsg);
			if (strcmp(data, "l") == 0) {
				data = strtok (NULL, " ,.-");
				act = data;
				if (strcmp(data, "c") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					arraySize = atoi(param2);
					
					length = strlen(param1);
					
					printf("Server: Lock Create, machine = %d, name = %s\n", head->clientMachineID, param1);
					CreateLock_RPC(head->sender, param1, arraySize, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "a") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					lockIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					lockIndex2 = atoi(param2);
					
					printf("Server: Lock Acquire, machine = %d, index = %s\n", head->clientMachineID, serverLocks[lockIndex].name);
					Acquire_RPC(head->sender, lockIndex, lockIndex2, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "r") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					lockIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					lockIndex2 = atoi(param2);
					
					printf("Server: Lock Release, machine = %d, index = %s\n", head->clientMachineID, serverLocks[lockIndex].name);
					Release_RPC(head->sender, lockIndex, lockIndex2, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "d") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					lockIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					lockIndex2 = atoi(param2);
					
					printf("Server: Lock Destroy, machine = %d, index = %s\n", head->clientMachineID, serverLocks[lockIndex].name);
					DestroyLock_RPC(head->sender, lockIndex, lockIndex2, head->clientMachineID, head->clientMailboxID);
				} else {
					printf("Server: Bad request.\n");
					if (head->sender)
						ServerReply(head->clientMachineID, head->clientMailboxID, BAD_FORMAT);
				}
			} else if (strcmp(data, "c") == 0) {
				data = strtok (NULL, " ,.-");
				act = data;
				if (strcmp(data, "c") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					length = strlen(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					arraySize = atoi(param2);
					
					printf("Server: Condition Create, machine = %d, name = %s\n", head->clientMachineID, param1);
					CreateCV_RPC(head->sender, param1, arraySize, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "w") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					cvIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					cvIndex2 = atoi(param2);
					
					data = strtok (NULL, " ,.-");
					param3 = data;
					lockIndex = atoi(param3);
					
					data = strtok (NULL, " ,.-");
					param4 = data;
					lockIndex2 = atoi(param4);
					
					printf("Server: Condition Wait, machine = %d, cv = %s, lock = %s\n", head->clientMachineID, serverCVs[cvIndex].name, serverLocks[lockIndex].name);
					Wait_RPC(head->sender, cvIndex, cvIndex2, lockIndex, lockIndex2, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "s") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					cvIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					cvIndex2 = atoi(param2);
					
					data = strtok (NULL, " ,.-");
					param3 = data;
					lockIndex = atoi(param3);
					
					data = strtok (NULL, " ,.-");
					param4 = data;
					lockIndex2 = atoi(param4);
					
					printf("Server: Condition Signal, machine = %d, cv = %s, lock = %s\n", head->clientMachineID, serverCVs[cvIndex].name, serverLocks[lockIndex].name);
					Signal_RPC(head->sender, cvIndex, cvIndex2, lockIndex, lockIndex2, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "b") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					cvIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					cvIndex2 = atoi(param2);
					
					data = strtok (NULL, " ,.-");
					param3 = data;
					lockIndex = atoi(param3);
					
					data = strtok (NULL, " ,.-");
					param4 = data;
					lockIndex2 = atoi(param4);
					
					printf("Server: Condition Broadcast, machine = %d, cv = %s, lock = %s\n", head->clientMachineID, serverCVs[cvIndex].name, serverLocks[lockIndex].name);
					Broadcast_RPC(head->sender, cvIndex, cvIndex2, lockIndex, lockIndex2, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "d") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					cvIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					cvIndex2 = atoi(param2);
					
					printf("Server: Condition Delete, machine = %d, cv = %s", head->clientMachineID, param1);
					DestroyCV_RPC(head->sender, cvIndex, cvIndex2, head->clientMachineID, head->clientMailboxID);
				} else {
					printf("Server: Bad request.\n");
					if (head->sender)
						ServerReply(head->clientMachineID, head->clientMailboxID, BAD_FORMAT);
				}
			} else if (strcmp(data, "m") == 0) {
				data = strtok (NULL, " ,.-");
				act = data;
				//printf("This far\n");
				if (strcmp(data, "c") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					arraySize = atoi(param2);
					
					data = strtok (NULL, " ,.-");
					param3 = data;
					value = atoi(param3);
					
					printf("Server: MV Create, machine = %d, name = %s, val = %d\n", head->clientMachineID, param1, value);
					CreateMV_RPC(head->sender, param1, arraySize, value, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "g") == 0) {
					data = strtok (NULL, " ,.-");
					param1 = data;
					mvIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					mvIndex2 = atoi(param2);
					
					printf("Server: MV Get, machine = %d, name = %s\n", head->clientMachineID, serverMVs[mvIndex].name);
					GetMV_RPC(head->sender, mvIndex, mvIndex2, head->clientMachineID, head->clientMailboxID);
				} else if (strcmp(data, "s") == 0) {
					//printf("This farr\n");
					data = strtok (NULL, " ,.-");
					param1 = data;
					mvIndex = atoi(param1);
					
					data = strtok (NULL, " ,.-");
					param2 = data;
					mvIndex2 = atoi(param2);
					
					//printf("This farr\n");
					data = strtok (NULL, " ,.-");
					param3 = data;
					value = atoi(param3);
					
					//printf("This farr\n");
					printf("Server: MV Set, machine = %d, index = %s, value = %d\n", head->clientMachineID, serverMVs[mvIndex].name, value);
					SetMV_RPC(head->sender, mvIndex, mvIndex2, value, head->clientMachineID, head->clientMailboxID);
				} else {
					printf("Server: Bad request.\n");
					if (head->sender)
						ServerReply(head->clientMachineID, head->clientMailboxID, BAD_FORMAT);
				}
			} else {
				printf("Server: Bad request.\n");
				if (head->sender)
					ServerReply(head->clientMachineID, head->clientMailboxID, BAD_FORMAT);
			}
			
			// Step 6 Again.) Get TS and machineID from 1st msg in messageQ
			if (messageQ->IsEmpty())
				break;
			head = (Message*)messageQ->Remove();
			if (head->timestamp > smallestTimestamp) {
				messageQ->Prepend((void*)head);
			}
		}
	}
}

void Client(int farAddr) {
}

void LockTest (int farAddr) {
	if (farAddr == 0) {
		Client(farAddr);
		return;
	}
	Server();
}

void parseServerFunction(){
}
