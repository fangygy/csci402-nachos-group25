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

// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our 
//	    original message

int CreateLock_RPC(char* name) {
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char* data;
	char* ack;
	
	char buffer[MaxMailSize];
	int lockIndex;
	
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
    
	//Do data parsing here with lockIndex and buffer
	//lockIndex = buffer?
	
    fflush(stdout);
	
	return lockIndex;
}

void Acquire_RPC(int index) {
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
    printf("Successfully acquired Lock: %d\n", index);
	
    fflush(stdout);
	
}

void Release_RPC(int index) {
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
    printf("Successfully released Lock: %d\n", index);
	
    fflush(stdout);
}

void DestroyLock_RPC(int index) {
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
    printf("Successfully sent a Destroy Request on Lock: %d\n", index);
	
    fflush(stdout);
}

int CreateCV_RPC(char* name) {
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
}

void Wait_RPC(int cIndex, int lIndex) {
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
}

void Signal_RPC(int cIndex, int lIndex) {
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
}

void Broadcast_RPC(int cIndex, int lIndex) {
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
}

void DestroyCV_RPC(int cIndex) {
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
}

int CreateMV_RPC(char* name) {
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char* data;
	char* ack;
	
	char buffer[MaxMailSize];
	
	int mvIndex;
	
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
	
	//Parse buffer to get the mvIndex
	
    fflush(stdout);
	
	return mvIndex;
}

int GetMV_RPC(int index) {
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
	
	char* data;
	char* ack;
	
	char buffer[MaxMailSize];
	
	int mvValue;
	
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
	
	//Parse buffer to get mvValue
	
    fflush(stdout);
	
	return mvValue;
}

void SetMV_RPC(int index, int value) {
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
	printf("Successfully set MV at Index: %d to Value: %d", index, value);
	
    fflush(stdout);
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
