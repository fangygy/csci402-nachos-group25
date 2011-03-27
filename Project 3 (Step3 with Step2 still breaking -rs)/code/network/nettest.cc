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

void Server(int farAddr) {
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Server: Hello thar!";
    char *ack = "Server: Got it!";
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
      printf("Server: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Server: Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(ack) + 1;
    success = postOffice->Send(outPktHdr, outMailHdr, ack); 

    if ( !success ) {
      printf("Server: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the ack from the other machine to the first message we sent.
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("Server: Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Then we're done!
    interrupt->Halt();
}

void Client(int farAddr) {
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Client: Hello there!";
    char *ack = "Client: Got it!";
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
      printf("Client: The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

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
    fflush(stdout);

    // Then we're done!
    interrupt->Halt();
}

void LockTest (int farAddr) {
	if (farAddr == 0) {
		Client(farAddr);
		return;
	}
	Server(farAddr);
}

void parseServerFunction(){
	//put parsing here for server
}