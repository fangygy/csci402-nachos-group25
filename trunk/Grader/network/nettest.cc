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

class ServerLock{
public:
	ServerLock(char* debugName);
	~ServerLock(){ /*delete waitQueue*/};
	char* getName() { return name; };
	void markForDeletion() { markedForDeletion = true; };
	bool isFree() { return (state == FREE ? true : false); };
	bool isMarked() { return markedForDeletion; };
	
	int owner;
	char* name;
	enum State{FREE, BUSY};
	State state;
	List *waitQueue;
	bool markedForDeletion;
};

ServerLock::ServerLock(char* debugName){
	name = debugName;
	owner = -1;
	state = FREE;
	waitQueue = new List;
	markedForDeletion = false;
}

class ServerCondition{
public:
	ServerCondition(char* debugName);
	char* getName() { return name; };
	~ServerCondition() { delete waitQueue; };
	void markForDeletion() { markedForDeletion = true; };
	bool isFree() { return waitQueue->IsEmpty(); };
	bool isMarked() { return markedForDeletion; };
	
	char* name;
	List* waitQueue;
	int myLockID;
	bool markedForDeletion;
};

ServerCondition::ServerCondition(char* debugName){
	name = debugName;
	waitQueue = new List;
	myLockID = -1;
	markedForDeletion = false;
}

class ServerMV{
public:
	ServerMV(char* debugName) { name = debugName; value = 0;};
	~ServerMV() { };
	char* getName() { return name; };
	
	char* name;
	int value;
};

void forwardMessage(int server, int mailboxFrom, char* msg){
	PacketHeader outPktHdr, inPktHdr;
	MailHeader outMailHdr, inMailHdr;
	bool success;
	
	// Send a reply msg - maybe
	outPktHdr.to = server;
	outMailHdr.to = 0;
	outMailHdr.from = mailboxFrom;
	outMailHdr.length = strlen(msg) + 1;
	DEBUG('w', "Server forwarding message to Server %d: %s\n",server,msg);
	success = postOffice->Send(outPktHdr, outMailHdr, msg); 

	if ( !success ) {
		DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}
}

////////////////////////
////     GLOBALS  /////
////              /////
///////////////////////
	Table serverLockTable(MAX_LOCKS);
	Table serverConditionTable(MAX_CONDITIONS);
	Table serverMVTable(MAX_MVS);


/*********************************** 
	HELPER FUNCTIONS FOR CREATION
***********************************/
void createServerLock(int addr){
	//no inhdrs because we are a helper function- this data is passed to us in addr
	PacketHeader outPktHdr;
	MailHeader outMailHdr;
	bool success;
	char buffer[MaxMailSize] = "";
	char num[32] = "";
	char ack[MaxMailSize] = "";
	char data[MaxMailSize] = "";
	char* debugName = new char[64];
	debugName = currentThread->getName();//store lock name
	int myMailbox = mailboxBitmap.Find();
	//set message data based on our fork arguments
	outMailHdr.from = myMailbox;
	outPktHdr.to = addr/1000;
	outMailHdr.to = addr%1000;

	// Lookup name in this server's table
	int lid = -1;
	for(int i = 0; i<MAX_LOCKS; i++){
		if(serverLockTable.Get(i) != 0){
			//We found something in our table
			//Now we see if it's the lock we want
			if(!strcmp(((ServerLock*)serverLockTable.Get(i))->getName(),debugName)){
				lid = i;
				DEBUG('w',"LockHelper: serverLock %s already exists in the serverLock table with id %d.\n",debugName,lid);
				lid = (machineID*1000)+lid;
				break;
			}
		}
	}
	
	// Lookup name in the other servers' tables
	if(lid == -1){
		for(int i=0; i<numServers;i++){
			if(i!=machineID){
				PacketHeader outPktHdrServerCheck, inPktHdrServerCheck;
				MailHeader outMailHdrServerCheck, inMailHdrServerCheck;		
				outMailHdrServerCheck.to = 0;
				outMailHdrServerCheck.from = myMailbox;
				strcpy(data, "RT = CHSL NA = ");
				strcat(data,debugName);

				// check the other servers to see if the name
				// exists 
				outPktHdrServerCheck.to = i;		  
				outMailHdrServerCheck.length = strlen(data) + 1;
				DEBUG('w',"LockHelper Sending %s to Server...\n",data);
				// create & send a request msg to the server  
				success = postOffice->Send(outPktHdrServerCheck, outMailHdrServerCheck, data); 

				if ( !success ) {
				DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
				}
				DEBUG('w',"LockHelper Waiting for reply...\n");
				// Wait for a request msg
				postOffice->Receive(myMailbox, &inPktHdrServerCheck, &inMailHdrServerCheck, buffer);
				DEBUG('w',"LockHelper Got \"%s\" from %d, box %d\n",buffer,inPktHdrServerCheck.from,inMailHdrServerCheck.from);
				fflush(stdout);
				
				// Get the request type (must be first in a msg)
				sscanf(buffer,"%*s %*c %d", &lid); 
				
				if(lid != -1){
					DEBUG('w',"LockHelper found ServerLock with name %s in Server %d\n\n",debugName,inPktHdrServerCheck.from);
					break;
				}
			}
		}
	}
		
	ServerLock* temp;
	if(lid == -1){	
		//Make it		
		temp = new ServerLock(debugName);
		lid = serverLockTable.Put(temp);
		//printf("Lock %s\n",debugName);
		DEBUG('w',"LockHelper putting new serverLock %s in the serverLock table with id %d.\n",debugName,lid);
		//set it to unique value
		lid = (machineID*1000)+lid;
	}
	
	//and give back the lock number
	strcpy(ack, "SLID = ");
	sprintf(num,"%d",lid);
	strcat(ack,num);

	if(lid == -1){
		DEBUG('w',"LockHelper: COULD NOT CREATE SERVER LOCK %s BECAUSE THE SERVER LOCK TABLE IS FULL!\n",debugName);
		delete temp;
	}
	

	// send reply message
	outMailHdr.length = strlen(ack) + 1;
	DEBUG('w', "LockHelper sending acknowledgement: %s\n",ack);
	success = postOffice->Send(outPktHdr, outMailHdr, ack); 

	if ( !success ) {
		DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}
	mailboxBitmap.Clear(myMailbox);
}

void createServerCondition(int addr){
	//no inhdrs because we are a helper function- this data is passed to us in addr
	PacketHeader outPktHdr;
	MailHeader outMailHdr;
	bool success;
	char buffer[MaxMailSize] = "";
	char num[32] = "";
	char ack[MaxMailSize] = "";
	char data[MaxMailSize] = "";
	char* debugName = new char[64];
	debugName = currentThread->getName();//store condition name
	int myMailbox = mailboxBitmap.Find();
	//set message data based on our fork arguments
	outMailHdr.from = myMailbox;
	outPktHdr.to = addr/1000;
	outMailHdr.to = addr%1000;

	// Lookup name
	int cid = -1;
	for(int i = 0; i<MAX_CONDITIONS; i++){
		if(serverConditionTable.Get(i) != 0){
			//We found something in our table
			//Now we see if it's the condition we want
			if(!strcmp(((ServerCondition*)serverConditionTable.Get(i))->getName(),debugName)){
				cid = i;
				DEBUG('w',"ConditionHelper: serverCondition %s already exists in the serverCondition table with id %d.\n",debugName,cid);
				cid = (machineID*1000)+cid;
				break;
			}
		}
	}
	
	// Lookup name in the other servers' tables
	if(cid == -1){
		for(int i=0; i<numServers;i++){
			if(i!=machineID){
				PacketHeader outPktHdrServerCheck, inPktHdrServerCheck;
				MailHeader outMailHdrServerCheck, inMailHdrServerCheck;		
				outMailHdrServerCheck.to = 0;
				outMailHdrServerCheck.from = myMailbox;
				strcpy(data, "RT = CHSC NA = ");
				strcat(data,debugName);

				// check the other servers to see if the name
				// exists 
				outPktHdrServerCheck.to = i;		  
				outMailHdrServerCheck.length = strlen(data) + 1;
				DEBUG('w',"ConditionHelper Sending %s to Server...\n",data);
				// create & send a request msg to the server  
				success = postOffice->Send(outPktHdrServerCheck, outMailHdrServerCheck, data); 

				if ( !success ) {
				DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
				}
				DEBUG('w',"ConditionHelper Waiting for reply...\n");
				// Wait for a request msg
				postOffice->Receive(myMailbox, &inPktHdrServerCheck, &inMailHdrServerCheck, buffer);
				DEBUG('w',"ConditionHelper Got \"%s\" from %d, box %d\n",buffer,inPktHdrServerCheck.from,inMailHdrServerCheck.from);
				fflush(stdout);
				
				// Get the request type (must be first in a msg)
				sscanf(buffer,"%*s %*c %d", &cid); 
				
				if(cid != -1){
					DEBUG('w',"ConditionHelper found ServerCondition with name %s in Server %d\n\n",debugName,inPktHdrServerCheck.from);
					break;
				}
			}
		}
	}
	
	ServerCondition* temp;
	if(cid == -1){	
		//Make it		
		temp = new ServerCondition(debugName);
		cid = serverConditionTable.Put(temp);
		//printf("Condition %s\n",debugName);
		DEBUG('w',"ConditionHelper putting new serverCondition %s in the serverCondition table with id %d.\n",debugName,cid);
		cid = (machineID*1000)+cid;
	}
		
	//and give back the condition number
	strcpy(ack, "SCID = ");
	sprintf(num,"%d",cid);
	strcat(ack,num);

	if(cid == -1){
		DEBUG('w',"ConditionHelper: COULD NOT CREATE SERVER CONDITION %s BECAUSE THE SERVER CONDITION TABLE IS FULL!\n",debugName);
		delete temp;
	}

	// send reply message
	outMailHdr.length = strlen(ack) + 1;
	DEBUG('w', "ConditionHelper sending acknowledgement: %s\n",ack);
	success = postOffice->Send(outPktHdr, outMailHdr, ack); 

	if ( !success ) {
		DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}
	
	mailboxBitmap.Clear(myMailbox);
}


void createServerMV(int addr){
	//no inhdrs because we are a helper function- this data is passed to us in addr
	PacketHeader outPktHdr;
	MailHeader outMailHdr;
	bool success;
	char buffer[MaxMailSize] = "";
	char num[32] = "";
	char ack[MaxMailSize] = "";
	char data[MaxMailSize] = "";
	char* debugName = new char[64];
	debugName = currentThread->getName();//store MV name
	int myMailbox = mailboxBitmap.Find();
	//set message data based on our fork arguments
	outMailHdr.from = myMailbox;
	outPktHdr.to = addr/1000;
	outMailHdr.to = addr%1000;
	
	// Lookup name
	int id = -1;
	for(int i = 0; i<MAX_MVS; i++){
		if(serverMVTable.Get(i) != 0){
			//We found something in our table
			//Now we see if it's the MV we want
			if(!strcmp(((ServerMV*)serverMVTable.Get(i))->getName(),debugName)){
				id = i;
				DEBUG('w',"MVHelper: serverMV %s already exists in the serverMV table with id %d.\n",debugName,id);
				id = (machineID*1000)+id;
				break;
			}
		}
	}
	
	// Lookup name in the other servers' tables
	if(id == -1){
		for(int i=0; i<numServers;i++){
			if(i!=machineID){
				PacketHeader outPktHdrServerCheck, inPktHdrServerCheck;
				MailHeader outMailHdrServerCheck, inMailHdrServerCheck;		
				outMailHdrServerCheck.to = 0;
				outMailHdrServerCheck.from = myMailbox;
				strcpy(data, "RT = CHMV NA = ");
				strcat(data,debugName);

				// check the other servers to see if the name
				// exists 
				outPktHdrServerCheck.to = i;		 
				outMailHdrServerCheck.length = strlen(data) + 1;
				DEBUG('w',"MVHelper Sending %s to Server...\n",data);
				// create & send a request msg to the server  
				success = postOffice->Send(outPktHdrServerCheck, outMailHdrServerCheck, data); 

				if ( !success ) {
				DEBUG('u',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
				}
				DEBUG('w',"MVHelper Waiting for reply...\n");
				// Wait for a request msg
				postOffice->Receive(myMailbox, &inPktHdrServerCheck, &inMailHdrServerCheck, buffer);
				DEBUG('w',"MVHelper Got \"%s\" from %d, box %d\n",buffer,inPktHdrServerCheck.from,inMailHdrServerCheck.from);
				fflush(stdout);
				
				// Get the request type (must be first in a msg)
				sscanf(buffer,"%*s %*c %d", &id); 
				
				if(id != -1){
					DEBUG('w',"MVHelper found ServerLock with name %s in Server %d\n\n",debugName,inPktHdrServerCheck.from);
					break;
				}
			}
		}
	}	
			
	ServerMV* temp;
	if(id == -1){	
		//Make it		
		temp = new ServerMV(debugName);
		id = serverMVTable.Put(temp);
		//printf("MV %s\n",debugName);
		DEBUG('w',"MVHelper putting new serverMV %s in the serverMV table with id %d.\n",debugName,id);
		id = (machineID*1000)+id;
	}

	//and give back the MV number
	strcpy(ack, "SMVID = ");
	sprintf(num,"%d",id);
	strcat(ack,num);

	if(id == -1){
		DEBUG('w',"MVHelper: COULD NOT CREATE SERVER MV %s BECAUSE THE SERVER MV TABLE IS FULL!\n",debugName);
		delete temp;
	}

	// send reply message
	outMailHdr.length = strlen(ack) + 1;
	DEBUG('w', "MVHelper sending acknowledgement: %s\n",ack);
	success = postOffice->Send(outPktHdr, outMailHdr, ack); 

	if ( !success ) {
		DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}

	mailboxBitmap.Clear(myMailbox);
}

void serverWaitRequest(int addr){
	//no inhdrs because we are a helper function- this data is passed to us in addr
	PacketHeader outPktHdr,inPktHdr;
	MailHeader outMailHdr,inMailHdr;
	bool success;
	char requestType[MaxMailSize] = "";
	char buffer[MaxMailSize] = "";
	char num[32] = "";
	char ack[MaxMailSize] = "";
	char data[MaxMailSize] = "";
	strcpy(buffer,currentThread->getName());
	int myMailbox = mailboxBitmap.Find();
	//set message data based on our fork arguments
	outMailHdr.from = myMailbox;
	
	int cid, lid;
	sscanf(buffer,"%*s %*c %s %*s %*c %d %*s %*c %d", requestType, &cid, &lid);

	// make sure the request is being made
	// on the correct server, if not forward
	// the lock request later
	bool forwardLockRelease = false;
	if(lid/1000 != machineID){
		forwardLockRelease = true;
	}
	else{
		lid = lid%1000;
	}

	cid = cid%1000;

	// if the message was forwarded,
	// we need to parse the "TO" field
	// to see who to reply to
	if(!strcmp (requestType,"FSW"))
		sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %*d %*s %*c %d", &inPktHdr.from);

	// Make sure both things exist in their respective tables
	if(serverConditionTable.Get(cid) != 0 && (forwardLockRelease || serverLockTable.Get(lid) != 0) ){
		ServerCondition* cond = (ServerCondition*)serverConditionTable.Get(cid);
	
		if(cond->myLockID == lid || cond->myLockID == -1){
			if(cond->myLockID == -1){
				if(!forwardLockRelease)
					cond->myLockID = machineID*1000+lid;
				else
					cond->myLockID = lid;
			}
			if(!forwardLockRelease){
				//Release the lock
				ServerLock* lock = (ServerLock*)serverLockTable.Get(lid);
				if(!lock->waitQueue->IsEmpty()){
					//someone was waiting for this lock, so reply to them
					DEBUG('w',"WaitHelper sent the next message on queue for lock %s, using id %d.\n",lock->getName(),lid);
					Mail* newMail=(Mail*)lock->waitQueue->Remove();
					
					//set new owner
					DEBUG('w', "WaitHelper setting lock %s with id %d lock->owner to %d\n",lock->getName(),lid,newMail->pktHdr.to * 1000 + newMail->mailHdr.to);
					lock->owner = newMail->pktHdr.to * 1000 + newMail->mailHdr.to;
					
					// send reply msg to a thread waiting for a release
					DEBUG('w', "WaitHelper sending acknowledgement: %s\n",newMail->data);
					success = postOffice->Send(newMail->pktHdr, newMail->mailHdr, newMail->data); 

					if ( !success ) {
						DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}
				}
				else { 
					// there was no one waiting for the lock, so update it's status/owner
					lock->owner = NULL;
					lock->state = ServerLock::FREE;
					DEBUG('w',"WaitHelper syscall released lock %s with id %d (lock->owner set to NULL and lock->state set to FREE).\n",lock->getName(),lid);
				}
			}
			// we need to forward the release request
			else{
				// release the lock and wait for a reply
				strcpy(data, "RT = WSR LID = ");
				sprintf(num,"%d",lid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",addr);
				strcat(data,num);
				
				outPktHdr.to = lid/1000;		
				outMailHdr.to = 0;    
				outMailHdr.length = strlen(data) + 1;
				DEBUG('w',"\nWaitHelper forwarding lock release request %s to Server...\n",data);
				// create & send a request msg to the server  
				success = postOffice->Send(outPktHdr, outMailHdr, data); 

				if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
				}

				// wait for the reply msg from release request
				DEBUG('w',"WaitHelper waiting for reply...\n");
				postOffice->Receive(myMailbox, &inPktHdr, &inMailHdr, buffer);
				DEBUG('w',"WaitHelper Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
				fflush(stdout);

				sscanf(buffer,"%*s %*c %d", &lid);
				if(lid!=-1){
					DEBUG('w',"WaitHelper called by thread %s has Released server lock with id %d.\n",currentThread->getName(),lid);
				} 
				else{
					// create error reply msg & send
					cid = -1;
					lid = -1;
					strcpy(ack, "CID = ");
					sprintf(num,"%d",cid);
					strcat(ack,num);
					strcat(ack, " LID = ");
					sprintf(num,"%d",lid);
					strcat(ack,num);
					strcat(ack," ServerWaitInvalidLockNotReleaseNETTESTLINE549");
					
					// Send a reply msg - maybe
					outPktHdr.to = addr/1000;
					outMailHdr.to = addr%1000;
					outMailHdr.length = strlen(ack) + 1;
					DEBUG('w',"WaitHelper called by thread %s tried to release an invalid server lock.\n",currentThread->getName());
					DEBUG('w', "WaitHelper sending acknowledgement: %s\n",ack);
					success = postOffice->Send(outPktHdr, outMailHdr, ack); 

					if ( !success ) {
						DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}	
					return;
				}
			}
			
			// make the reply msg
			strcpy(ack, "CID = ");
			sprintf(num,"%d",cid);
			strcat(ack,num);
			strcat(ack, " LID = ");
			sprintf(num,"%d",lid);
			strcat(ack,num);
			
			// Send a reply msg - later
			outPktHdr.to = addr/1000;
			outMailHdr.to = addr%1000;
			outMailHdr.length = strlen(ack) + 1;
			
			// put reply msg in wait Q
			if(!forwardLockRelease)
				DEBUG('w',"WaitHelper called on condition %s using id %d with lock %s using id %d is now ASLEEP and the reply message is being place on the waitQueue.\n",cond->getName(),cid,((ServerLock*)serverLockTable.Get(lid))->getName(),lid);
			else
				DEBUG('w',"WaitHelper called on condition %s using id %d with lock using id %d is now ASLEEP and the reply message is being place on the waitQueue.\n",cond->getName(),cid,lid);
			Mail* newMail = new Mail(outPktHdr, outMailHdr, ack);
			cond->waitQueue->Append(newMail);
			/*
			DEBUG('w', "WaitHelper sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack); 

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
			*/
		}
		//lock being passed isn't the lock used for this condition currently
		else{
			// create error reply msg & send
			cid = -1;
			lid = -1;
			strcpy(ack, "CID = ");
			sprintf(num,"%d",cid);
			strcat(ack,num);
			strcat(ack, " LID = ");
			sprintf(num,"%d",lid);
			strcat(ack,num);
			strcat(ack," ServerWaitLockNotForConditionNETTESTLINE608");
			
			// Send a reply msg - maybe
			outPktHdr.to = addr/1000;
			outMailHdr.to = addr%1000;
			outMailHdr.length = strlen(ack) + 1;
			DEBUG('w', "WaitHelper: wait request FAILED because the condition was neither the owner of the lock nor -1!\n");
			DEBUG('w', "WaitHelper sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack); 

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}					
		}	
	}
	// not a valid #
	else {
		// create error reply msg & send
		cid = -1;
		lid = -1;
		strcpy(ack, "CID = ");
		sprintf(num,"%d",cid);
		strcat(ack,num);
		strcat(ack, " LID = ");
		sprintf(num,"%d",lid);
		strcat(ack,num);
		strcat(ack," ServerWaitNonExistentLockOrCVNETTESTLINE635");
		
		// Send a reply msg - maybe
		outPktHdr.to = addr/1000;
		outMailHdr.to = addr%1000;
		outMailHdr.length = strlen(ack) + 1;
		DEBUG('w', "WaitHelper: wait request FAILED because either the condition or the lock was invalid!\n");
		DEBUG('w', "WaitHelper sending acknowledgement: %s\n",ack);
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) {
			DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
	}

	mailboxBitmap.Clear(myMailbox);
}

void serverSignalRequest(int addr){
	//no inhdrs because we are a helper function- this data is passed to us in addr
	PacketHeader outPktHdr,inPktHdr;
	MailHeader outMailHdr,inMailHdr;
	bool success;
	char requestType[MaxMailSize] = "";
	char buffer[MaxMailSize] = "";
	char num[32] = "";
	char ack[MaxMailSize] = "";
	char data[MaxMailSize] = "";
	strcpy(buffer,currentThread->getName());
	int myMailbox = mailboxBitmap.Find();
	//set message data based on our fork arguments
	outMailHdr.from = myMailbox;
	
	int cid, lid;
	sscanf(buffer,"%*s %*c %s %*s %*c %d %*s %*c %d", requestType, &cid, &lid);

	// make sure the request is being made
	// on the correct server, if not forward
	// the lock request later
	bool forwardLockAcquire = false;
	if(lid/1000 != machineID){
		forwardLockAcquire = true;
	}
	else{
		lid = lid%1000;
	}

	cid = cid%1000;

	// if the message was forwarded,
	// we need to parse the "TO" field
	// to see who to reply to
	if(!strcmp (requestType,"FSS"))
		sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %*d %*s %*c %d", &inPktHdr.from);
	
	if(serverConditionTable.Get(cid) != 0 && (forwardLockAcquire || serverLockTable.Get(lid) != 0)){
		
		if(!forwardLockAcquire){
			ServerLock* lock = (ServerLock*)serverLockTable.Get(lid);
			if(lock->owner!=addr){
				//thread that sent this request wasn't the owner!
								// create error reply msg & send
				DEBUG('w',"SignalHelper tried to signal with lock %s with id %d, but %d wasn't the owner\n",lock->getName(),lid, addr);
				cid = -1;
				lid = -1;
				strcpy(ack, "CID = ");
				sprintf(num,"%d",cid);
				strcat(ack,num);
				strcat(ack, " LID = ");
				sprintf(num,"%d",lid);
				strcat(ack,num);
				strcat(ack," ServerSignalNotLockOwnerNETTESTLINE707");
				
				// Send a reply msg - maybe
				outPktHdr.to = addr/1000;
				outMailHdr.to = addr%1000;
				//outPktHdr.to = inPktHdr.from;
				//outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				
				DEBUG('w', "SignalHelper sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
				return;
				
			}
		}
		// send a request to see if to
		// the server containing the lock
		// to see if currentThread is the owner
		else{

			strcpy(data, "RT = CHSLO LID = ");
			sprintf(num,"%d",lid);
			strcat(data,num);
			strcat(data," TO = ");
			sprintf(num,"%d",addr);
			strcat(data,num);
			
			outPktHdr.to = lid/1000;		
			outMailHdr.to = 0;    
			outMailHdr.length = strlen(data) + 1;
			DEBUG('w',"\nSignalHelper sending check lock owner request %s to Server...\n",data);
			// create & send a request msg to the server  
			success = postOffice->Send(outPktHdr, outMailHdr, data); 

			if ( !success ) {
			DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
			}

			// wait for the reply msg
			// return (or not) a value to the user program
			DEBUG('w',"SignalHelper Waiting...\n");
			postOffice->Receive(myMailbox, &inPktHdr, &inMailHdr, buffer);
			DEBUG('w',"SignalHelper Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
			fflush(stdout);

			sscanf(buffer,"%*s %*c %d", &lid);
			if(lid!=-1){
				DEBUG('w',"SignalHelper: lock %d is owned by addr %d.\n",lid,addr);
			} 
			else{
				// create error reply msg & send
				cid = -1;
				lid = -1;
				strcpy(ack, "CID = ");
				sprintf(num,"%d",cid);
				strcat(ack,num);
				strcat(ack, " LID = ");
				sprintf(num,"%d",lid);
				strcat(ack,num);
				strcat(ack," ServerSignalNotLockOwnerNETTESTLINE769");
				
				// Send a reply msg - maybe
				outPktHdr.to = addr/1000;
				outMailHdr.to = addr%1000;
				outMailHdr.length = strlen(ack) + 1;
				DEBUG('w',"SignalHelper: lock %d is not owned by addr %d.\n",lid,addr);
				DEBUG('w', "SignalHelper sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}	
				return;
			}

		}
		//owner is correct
		ServerCondition* cond = (ServerCondition*)serverConditionTable.Get(cid);
		
		//check if the lock is the right lock for this condition
		if(cond->myLockID == lid){
			if(!cond->waitQueue->IsEmpty()){
				//someone was waiting for this condition, so reply to them
				Mail* newMail=(Mail*)cond->waitQueue->Remove();
				
				if(!forwardLockAcquire){
					ServerLock* lock = (ServerLock*)serverLockTable.Get(lid);
					//Need to acquire on behalf of the waiter
					if(lock->state == ServerLock::FREE){
						DEBUG('w',"SignalHelper: the lock was FREE and the waitQueue for condition %s with %d was NOT EMPTY so acquiring lock %s with id %d on behalf of waiter and setting lock->owner to %d and lock->state to BUSY.\n",cond->getName(),cid,lock->getName(),lid,(newMail->pktHdr.to*1000 + newMail->mailHdr.to));
						lock->owner = (newMail->pktHdr.to*1000 + newMail->mailHdr.to);
						lock->state = ServerLock::BUSY;
					} 
					else{
						//Make a new mail with the sender set as our recipient-to-be
						Mail* newerMail = new Mail(newMail->pktHdr, newMail->mailHdr, newMail->data);
						newerMail->pktHdr.from = newMail->pktHdr.to;
						newerMail->mailHdr.from = newMail->pktHdr.to;
						lock->waitQueue->Append(newerMail);
						DEBUG('w',"SignalHelper: lock %s with id %d was BUSY and waitQueue for condition %s with id %d is NOT EMPTY so message placed on lock's waitQueue.\n",lock->getName(),lid,cond->getName(),cid);
					}
				}
				// forward server lock acquire
				else{
					// acquire the lock on behalf of 
					// waiter and wait for a reply
					strcpy(data, "RT = SSA LID = ");
					sprintf(num,"%d",lid);
					strcat(data,num);
					strcat(data," TO = ");
					sprintf(num,"%d",(newMail->pktHdr.to*1000 + newMail->mailHdr.to));
					strcat(data,num);
					strcat(data," NO = ");
					sprintf(num,"%d",(newMail->pktHdr.to*1000 + newMail->mailHdr.to));
					strcat(data,num);
					strcat(data," DA = ");
					strcat(data,newMail->data);
					
					outPktHdr.to = lid/1000;		
					outMailHdr.to = 0;    
					outMailHdr.length = strlen(data) + 1;
					DEBUG('w',"SignalHelper forwarding lock acquire request %s to Server...\n",data);
					// create & send a request msg to the server  
					success = postOffice->Send(outPktHdr, outMailHdr, data); 

					if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
					}

					// wait for the reply msg
					// return (or not) a value to the user program
					DEBUG('w',"SignalHelper Waiting...\n");
					postOffice->Receive(myMailbox, &inPktHdr, &inMailHdr, buffer);
					DEBUG('w',"SignalHelper Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
					fflush(stdout);

					sscanf(buffer,"%*s %*c %d", &lid);
					if(lid!=-1){
						DEBUG('w',"SignalHelper called by thread %s has acquired server lock with id %d.\n",currentThread->getName(),lid);
					} 
					else{
						// create error reply msg & send
						cid = -1;
						lid = -1;
						strcpy(ack, "CID = ");
						sprintf(num,"%d",cid);
						strcat(ack,num);
						strcat(ack, " LID = ");
						sprintf(num,"%d",lid);
						strcat(ack,num);
						strcat(ack," ServerSignalInvalidLockNotAcquiredNETTESTLINE862");
						
						// Send a reply msg - maybe
						outPktHdr.to = addr/1000;
						outMailHdr.to = addr%1000;
						outMailHdr.length = strlen(ack) + 1;
						DEBUG('w',"SignalHelper called by thread %s tried to acquire an invalid server lock.\n",currentThread->getName());
						DEBUG('w', "SignalHelper sending acknowledgement: %s\n",ack);
						success = postOffice->Send(outPktHdr, outMailHdr, ack); 

						if ( !success ) {
							DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
							interrupt->Halt();
						}	
						return;
					}
				}
			}

			//if no more waiters
			if(cond->waitQueue->IsEmpty()){
				if(cond->isMarked()){
					//set cid and lid to our special values for this particular case
					cid = -2;
					lid = -2;
					DEBUG('w',"SignalHelper setting condition %s with id %d to -2 (special value) and deleting it and setting lock with id %d to -2 (special value).\n",cond->getName(),cid,lid);
					delete ((ServerCondition*)serverConditionTable.Get(cid));				
				}
				else{
					DEBUG('w',"SignalHelper setting cond->myLockID %s with id %d to -1.\n",cond->getName(),cid);
					cond->myLockID = -1;
				}
			}

			strcpy(ack, "CID = ");
			sprintf(num,"%d",cid);
			strcat(ack,num); 
			strcat(ack, " LID = ");
			sprintf(num,"%d",lid);
			strcat(ack,num);
			
			// Send a reply msg - maybe
			outPktHdr.to = addr/1000;
			outMailHdr.to = addr%1000;
			outMailHdr.length = strlen(ack) + 1;
			
			if(!forwardLockAcquire)
				DEBUG('w',"SignalHelper signalled condition %s using id %d with lock %s using id %d.\n",cond->getName(),cid,((ServerLock*)serverLockTable.Get(lid))->getName(),lid);
			else
				DEBUG('w',"SignalHelper signalled condition %s using id %d with lock using id %d.\n",cond->getName(),cid,lid);
			DEBUG('w', "SignalHelper sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack);
			
			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		//lock passed is not lock that condition is using currently
		else{
			// create error reply msg & send
			cid = -1;
			lid = -1;
			strcpy(ack, "CID = ");
			sprintf(num,"%d",cid);
			strcat(ack,num); 
			strcat(ack, " LID = ");
			sprintf(num,"%d",lid);
			strcat(ack,num);
			strcat(ack," ServerSignalLockNotForConditionNETTESTLINE931");
			
			// Send a reply msg - maybe
			outPktHdr.to = addr/1000;
			outMailHdr.to = addr%1000;
			outMailHdr.length = strlen(ack) + 1;				
			
			DEBUG('w', "SignalHelper: signal request FAILED because the condition was not the owner of the lock!\n");
			DEBUG('w', "SignalHelper sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack); 

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		
	}
	// not a valid #
	else {
		// create error reply msg & send
		cid = -1;
		lid = -1;
		strcpy(ack, "CID = ");
		sprintf(num,"%d",cid);
		strcat(ack,num); 
		strcat(ack, " LID = ");
		sprintf(num,"%d",lid);
		strcat(ack,num);
		strcat(ack," ServerSignalNonexistentLockOrCVNETTESTLINE960");
		
		// Send a reply msg - maybe
		outPktHdr.to = addr/1000;
		outMailHdr.to = addr%1000;
		outMailHdr.length = strlen(ack) + 1;				
		
		DEBUG('w', "SignalHelper: signal request FAILED because either the condition or the lock was invalid!\n");
		DEBUG('w', "SignalHelper sending acknowledgement: %s\n",ack);
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) {
			DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
	}

	mailboxBitmap.Clear(myMailbox);
}

void serverBroadcastRequest(int addr){
	//no inhdrs because we are a helper function- this data is passed to us in addr
	PacketHeader outPktHdr,inPktHdr;
	MailHeader outMailHdr,inMailHdr;
	bool success;
	char requestType[MaxMailSize] = "";
	char buffer[MaxMailSize] = "";
	char num[32] = "";
	char ack[MaxMailSize] = "";
	char data[MaxMailSize] = "";
	strcpy(buffer,currentThread->getName());
	int myMailbox = mailboxBitmap.Find();
	//set message data based on our fork arguments
	outMailHdr.from = myMailbox;
	
	int cid, lid;
	sscanf(buffer,"%*s %*c %s %*s %*c %d %*s %*c %d", requestType, &cid, &lid);

	// make sure the request is being made
	// on the correct server, if not forward
	// the lock request later
	bool forwardLockAcquire = false;
	if(lid/1000 != machineID){
		forwardLockAcquire = true;
	}
	else{
		lid = lid%1000;
	}

	cid = cid%1000;

	// if the message was forwarded,
	// we need to parse the "TO" field
	// to see who to reply to
	if(!strcmp (requestType,"FSB"))
		sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %*d %*s %*c %d", &inPktHdr.from);
	
	if(serverConditionTable.Get(cid) != 0 && (forwardLockAcquire || serverLockTable.Get(lid) != 0)){
		
		if(!forwardLockAcquire){
			ServerLock* lock = (ServerLock*)serverLockTable.Get(lid);
			if(lock->owner!=addr){
				//thread that sent this request wasn't the owner!
								// create error reply msg & send
				DEBUG('w',"BroadcastHelper tried to signal with lock %s with id %d, but %d wasn't the owner\n",lock->getName(),lid, addr);
				cid = -1;
				lid = -1;
				strcpy(ack, "CID = ");
				sprintf(num,"%d",cid);
				strcat(ack,num);
				strcat(ack, " LID = ");
				sprintf(num,"%d",lid);
				strcat(ack,num);
				strcat(ack," ServerBroadcastNotLockOwnerNETTESTLINE1033");
				
				// Send a reply msg - maybe
				outPktHdr.to = addr/1000;
				outMailHdr.to = addr%1000;
				//outPktHdr.to = inPktHdr.from;
				//outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				
				DEBUG('w', "BroadcastHelper sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
				return;
				
			}
		}

		// send a request to see if to
		// the server containing the lock
		// to see if currentThread is the owner
		else{

			strcpy(data, "RT = CHSLO LID = ");
			sprintf(num,"%d",lid);
			strcat(data,num);
			strcat(data," TO = ");
			sprintf(num,"%d",addr);
			strcat(data,num);
			
			outPktHdr.to = lid/1000;		
			outMailHdr.to = 0;    
			outMailHdr.length = strlen(data) + 1;
			DEBUG('w',"\nBroadcastHelper sending check lock owner request %s to Server...\n",data);
			// create & send a request msg to the server  
			success = postOffice->Send(outPktHdr, outMailHdr, data); 

			if ( !success ) {
			DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
			}

			// wait for the reply msg
			// return (or not) a value to the user program
			DEBUG('w',"BroadcastHelper Waiting...\n");
			postOffice->Receive(myMailbox, &inPktHdr, &inMailHdr, buffer);
			DEBUG('w',"BroadcastHelper Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
			fflush(stdout);

			sscanf(buffer,"%*s %*c %d", &lid);
			if(lid!=-1){
				DEBUG('w',"BroadcastHelper: lock %d is owned by addr %d.\n",lid,addr);
			} 
			else{
				// create error reply msg & send
				cid = -1;
				lid = -1;
				strcpy(ack, "CID = ");
				sprintf(num,"%d",cid);
				strcat(ack,num);
				strcat(ack, " LID = ");
				sprintf(num,"%d",lid);
				strcat(ack,num);
				strcat(ack," ServerBroadcastNotLockOwnerNETTESTLINE1096");
				
				// Send a reply msg - maybe
				outPktHdr.to = addr/1000;
				outMailHdr.to = addr%1000;
				outMailHdr.length = strlen(ack) + 1;
				DEBUG('w',"BroadcastHelper: lock %d is not owned by addr %d.\n",lid,addr);
				DEBUG('w', "BroadcastHelper sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}	
				return;
			}

		}
		//owner is correct
		ServerCondition* cond = (ServerCondition*)serverConditionTable.Get(cid);
		
		//check if the lock is the right lock for this condition
		if(cond->myLockID == lid){
			while(!cond->waitQueue->IsEmpty()){
				//someone was waiting for this condition, so reply to them
				Mail* newMail=(Mail*)cond->waitQueue->Remove();
				
				if(!forwardLockAcquire){
					ServerLock* lock = (ServerLock*)serverLockTable.Get(lid);
					//Need to acquire on behalf of the waiter
					if(lock->state == ServerLock::FREE){
						DEBUG('w',"BroadcastHelper: the lock was FREE and the waitQueue for condition %s with %d was NOT EMPTY so acquiring lock %s with id %d on behalf of waiter and setting lock->owner to %d and lock->state to BUSY.\n",cond->getName(),cid,lock->getName(),lid,(newMail->pktHdr.to*1000 + newMail->mailHdr.to));
						lock->owner = (newMail->pktHdr.to*1000 + newMail->mailHdr.to);
						lock->state = ServerLock::BUSY;
					} 
					else{
						//Make a new mail with the sender set as our recipient-to-be
						Mail* newerMail = new Mail(newMail->pktHdr, newMail->mailHdr, newMail->data);
						newerMail->pktHdr.from = newMail->pktHdr.to;
						newerMail->mailHdr.from = newMail->pktHdr.to;
						lock->waitQueue->Append(newerMail);
						DEBUG('w',"BroadcastHelper: lock %s with id %d was BUSY and waitQueue for condition %s with id %d is NOT EMPTY so message placed on lock's waitQueue.\n",lock->getName(),lid,cond->getName(),cid);
					}
				}
				// forward server lock acquire
				else{
					// acquire the lock on behalf of 
					// waiter and wait for a reply
					strcpy(data, "RT = BSA LID = ");
					sprintf(num,"%d",lid);
					strcat(data,num);
					strcat(data," TO = ");
					sprintf(num,"%d",(newMail->pktHdr.to*1000 + newMail->mailHdr.to));
					strcat(data,num);
					strcat(data," NO = ");
					sprintf(num,"%d",(newMail->pktHdr.to*1000 + newMail->mailHdr.to));
					strcat(data,num);
					strcat(data," DA = ");
					strcat(data,newMail->data);
					
					outPktHdr.to = lid/1000;		
					outMailHdr.to = 0;    
					outMailHdr.length = strlen(data) + 1;
					DEBUG('w',"BroadcastHelper forwarding lock acquire request %s to Server...\n",data);
					// create & send a request msg to the server  
					success = postOffice->Send(outPktHdr, outMailHdr, data); 

					if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
					}

					// wait for the reply msg
					// return (or not) a value to the user program
					DEBUG('w',"BroadcastHelper Waiting...\n");
					postOffice->Receive(myMailbox, &inPktHdr, &inMailHdr, buffer);
					DEBUG('w',"BroadcastHelper Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
					fflush(stdout);

					sscanf(buffer,"%*s %*c %d", &lid);
					if(lid!=-1){
						DEBUG('w',"BroadcastHelper called by thread %s has acquired server lock with id %d.\n",currentThread->getName(),lid);
					} 
					else{
						// create error reply msg & send
						cid = -1;
						lid = -1;
						strcpy(ack, "CID = ");
						sprintf(num,"%d",cid);
						strcat(ack,num);
						strcat(ack, " LID = ");
						sprintf(num,"%d",lid);
						strcat(ack,num);
						strcat(ack," ServerBroadcastInvalidLockNotReleasedNETTESTLINE1189");
						
						// Send a reply msg - maybe
						outPktHdr.to = addr/1000;
						outMailHdr.to = addr%1000;
						outMailHdr.length = strlen(ack) + 1;
						DEBUG('w',"BroadcastHelper called by thread %s tried to acquire an invalid server lock.\n",currentThread->getName());
						DEBUG('w', "BroadcastHelper sending acknowledgement: %s\n",ack);
						success = postOffice->Send(outPktHdr, outMailHdr, ack); 

						if ( !success ) {
							DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
							interrupt->Halt();
						}	
						return;
					}
				}
			}

			//if no more waiters
			if(cond->waitQueue->IsEmpty()){
				if(cond->isMarked()){
					//set cid and lid to our special values for this particular case
					cid = -2;
					lid = -2;
					DEBUG('w',"BroadcastHelper setting condition %s with id %d to -2 (special value) and deleting it and setting lock with id %d to -2 (special value).\n",cond->getName(),cid,lid);
					delete ((ServerCondition*)serverConditionTable.Get(cid));				
				}
				
				else{
					DEBUG('w',"BroadcastHelper setting cond->myLockID %s with id %d to -1.\n",cond->getName(),cid);
					cond->myLockID = -1;
				}
			}

			strcpy(ack, "CID = ");
			sprintf(num,"%d",cid);
			strcat(ack,num); 
			strcat(ack, " LID = ");
			sprintf(num,"%d",lid);
			strcat(ack,num);
			
			// Send a reply msg - maybe
			outPktHdr.to = addr/1000;
			outMailHdr.to = addr%1000;
			outMailHdr.length = strlen(ack) + 1;
			
			if(!forwardLockAcquire)
				DEBUG('w',"BroadcastHelper signalled condition %s using id %d with lock %s using id %d.\n",cond->getName(),cid,((ServerLock*)serverLockTable.Get(lid))->getName(),lid);
			else
				DEBUG('w',"BroadcastHelper signalled condition %s using id %d with lock using id %d.\n",cond->getName(),cid,lid);
			DEBUG('w', "BroadcastHelper sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack);
			
			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		//lock passed is not lock that condition is using currently
		else{
			// create error reply msg & send
			
			if(!forwardLockAcquire)
				DEBUG('w',"BroadcastHelper signal request FAILED because condition %s using id %d was not the owner of lock %s using id %d.\n",cond->getName(),cid,((ServerLock*)serverLockTable.Get(lid))->getName(),lid);
			else
				DEBUG('w',"BroadcastHelper signal request FAILED because condition %s using id %d was not the owner of lock using id %d.\n",cond->getName(),cid,lid);
			cid = -1;
			lid = -1;
			strcpy(ack, "CID = ");
			sprintf(num,"%d",cid);
			strcat(ack,num); 
			strcat(ack, " LID = ");
			sprintf(num,"%d",lid);
			strcat(ack,num);
			strcat(ack," ServerBroadcastLockNotForCVNETTESTLINE1264");
			
			// Send a reply msg - maybe
			outPktHdr.to = addr/1000;
			outMailHdr.to = addr%1000;
			outMailHdr.length = strlen(ack) + 1;				
	
			DEBUG('w', "BroadcastHelper sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack); 

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		
	}
	// not a valid #
	else {
		// create error reply msg & send
		cid = -1;
		lid = -1;
		strcpy(ack, "CID = ");
		sprintf(num,"%d",cid);
		strcat(ack,num); 
		strcat(ack, " LID = ");
		sprintf(num,"%d",lid);
		strcat(ack,num);
		strcat(ack," ServerBroadcastNonexistentLockOrCVNETTESTLINE1292");
		
		// Send a reply msg - maybe
		outPktHdr.to = addr/1000;
		outMailHdr.to = addr%1000;
		outMailHdr.length = strlen(ack) + 1;				
		
		DEBUG('w', "BroadcastHelper: signal request FAILED because either the condition or the lock was invalid!\n");
		DEBUG('w', "BroadcastHelper sending acknowledgement: %s\n",ack);
		success = postOffice->Send(outPktHdr, outMailHdr, ack); 

		if ( !success ) {
			DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
	}
	
	mailboxBitmap.Clear(myMailbox);
}


// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our 
//	    original message

void
MyServer()
{
	PacketHeader outPktHdr, inPktHdr;
	MailHeader outMailHdr, inMailHdr;
	char data[MaxMailSize] = "";
	char ack[MaxMailSize] = "";
	char buffer[MaxMailSize] = "";
	char num[32] = "";
	bool success;
	char requestType[MaxMailSize] = "";		
	outMailHdr.to = 0;
	outMailHdr.from = 0;
	
	/*
	ServerCondition* strudle = new ServerCondition("strudle");
	ServerLock* pie=new ServerLock("pie");
	serverConditionTable.Put(strudle);
	serverLockTable.Put(pie);
	char fakeData[MaxMailSize] = "RT = SW CID = 0 LID = 0";
	outPktHdr.to = 0;		
	outMailHdr.to = 0;
	outMailHdr.from = 0;    
	outMailHdr.length = strlen(fakeData) + 1;
	DEBUG('u',"Userprog Sending %s to Server...\n",fakeData);
	// create & send a request msg to the server  
	postOffice->Send(outPktHdr, outMailHdr, fakeData); 
	*/
	
	while(true){
	
		DEBUG('w',"\nSERVER WAITING FOR REQUEST...\n\n");
		// Wait for a request msg
		postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
		DEBUG('w',"\nServer Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
		fflush(stdout);
		if(inMailHdr.from>1000)
			printf("inMailHdr.from = %d\n",inMailHdr.from);
		inMailHdr.from = inMailHdr.from%1000;
		// Get the request type (must be first in a msg)
		sscanf(buffer,"%*s %*c %s", requestType); 
		DEBUG('w',"Server received RequestType = %s\n\n",requestType);
	
		/*********************************** 
		// HANDLE CREATE SERVER LOCK REQUEST
		***********************************/
		if(strcmp (requestType,"CSL")==0){
		
			// Server "behavior" on a Create request		
			// Parse the rest of msg
			char* debugName = new char[64];
			sscanf(buffer,"%*s %*c %*s %*s %*c %s", debugName); 
			
			Thread* t = new Thread(debugName); //the thread's name is the lock name
			int i = inPktHdr.from * 1000 + inMailHdr.from; //this should give the helper function a unique number
			DEBUG('w',"Server forking helper thread to create server lock.\n");
			t->Fork((VoidFunctionPtr)createServerLock, i);

		}
		/**************************************** 
		// HANDLE CREATE SERVER CONDITION REQUEST
		****************************************/
		else if(strcmp (requestType,"CSC")==0){

			// Server "behavior" on a Create request		
			// Parse the rest of msg
			char* debugName = new char[64];
			sscanf(buffer,"%*s %*c %*s %*s %*c %s", debugName); 
			
			Thread* t = new Thread(debugName); //the thread's name is the lock name
			int i = inPktHdr.from * 1000 + inMailHdr.from; //this should give the helper function a unique number
			DEBUG('w',"Server forking helper thread to create server condition.\n");
			t->Fork((VoidFunctionPtr)createServerCondition, i);
			
		} 
		/**************************************** 
		// HANDLE CREATE MONITOR VARIABLE REQUEST
		****************************************/
		else if(strcmp (requestType,"CMV")==0){
			
			// Server "behavior" on a Create request		
			// Parse the rest of msg
			char* debugName = new char[64];
			sscanf(buffer,"%*s %*c %*s %*s %*c %s", debugName); 
			
			Thread* t = new Thread(debugName); //the thread's name is the lock name
			int i = inPktHdr.from * 1000 + inMailHdr.from; //this should give the helper function a unique number
			DEBUG('w',"Server forking helper thread to create server mv.\n");
			t->Fork((VoidFunctionPtr)createServerMV, i);
			
		}
		/********************************** 
		// HANDLE CHECK SERVER LOCK REQUEST
		**********************************/ 
		else if(strcmp (requestType,"CHSL")==0){

			// Server "behavior" on a Create request		
			// Parse the rest of msg
			char* debugName = new char[64];
			sscanf(buffer,"%*s %*c %*s %*s %*c %s", debugName); 
			
			// Lookup name in this server's table
			int lid = -1;
			for(int i = 0; i<MAX_LOCKS; i++){
				if(serverLockTable.Get(i) != 0){
					//We found something in our table
					//Now we see if it's the lock we want
					if(!strcmp(((ServerLock*)serverLockTable.Get(i))->getName(),debugName)){
						lid = i;
						break;
					}
				}
			}
			
			if(lid == -1){	
				DEBUG('w',"Server did not find serverLock %s in the serverLock table.\n",debugName);			
			}
			else{
				DEBUG('w',"Server serverLock %s found in the serverLock table with id %d.\n",debugName,lid);
				lid = (machineID*1000)+lid;
			}
			//and give back the lock number
			strcpy(ack, "SLID = ");
			sprintf(num,"%d",lid);
			strcat(ack,num);

			// Send a reply msg - maybe
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.length = strlen(ack) + 1;
			DEBUG('w', "Server sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack); 

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		/*************************************** 
		// HANDLE CHECK SERVER CONDITION REQUEST
		***************************************/
		else if(strcmp (requestType,"CHSC")==0){

			// Server "behavior" on a Create request		
			// Parse the rest of msg
			char* debugName = new char[64];
			sscanf(buffer,"%*s %*c %*s %*s %*c %s", debugName); 
			
			// Lookup name in this server's table
			int cid = -1;
			for(int i = 0; i<MAX_CONDITIONS; i++){
				if(serverConditionTable.Get(i) != 0){
					//We found something in our table
					//Now we see if it's the condition we want
					if(!strcmp(((ServerCondition*)serverConditionTable.Get(i))->getName(),debugName)){
						cid = i;
						break;
					}
				}
			}
			
			if(cid == -1){	
				DEBUG('w',"Server did not find serverCondition %s in the serverCondition table.\n",debugName);			
			}
			else{
				DEBUG('w',"Server serverCondition %s found in the serverCondition table with id %d.\n",debugName,cid);
				cid = (machineID*1000)+cid;
			}
			//and give back the condition number
			strcpy(ack, "SCID = ");
			sprintf(num,"%d",cid);
			strcat(ack,num);

			// Send a reply msg - maybe
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.length = strlen(ack) + 1;
			DEBUG('w', "Server sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack); 

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		} 
		/*************************************** 
		// HANDLE CHECK MONITOR VARIABLE REQUEST
		***************************************/
		else if(strcmp (requestType,"CHMV")==0){

			// Server "behavior" on a Create request		
			// Parse the rest of msg
			char* debugName = new char[64];
			sscanf(buffer,"%*s %*c %*s %*s %*c %s", debugName); 
			
			// Lookup name in this server's table
			int mvid = -1;
			for(int i = 0; i<MAX_MVS; i++){
				if(serverMVTable.Get(i) != 0){
					//We found something in our table
					//Now we see if it's the mv we want
					if(!strcmp(((ServerMV*)serverMVTable.Get(i))->getName(),debugName)){
						mvid = i;
						break;
					}
				}
			}
			
			if(mvid == -1){	
				DEBUG('w',"Server did not find serverMV %s in the serverMV table.\n",debugName);			
			}
			else{
				DEBUG('w',"Server serverMV %s found in the serverMV table with id %d.\n",debugName,mvid);
				mvid = (machineID*1000)+mvid;
			}
			//and give back the lock number
			strcpy(ack, "MVID = ");
			sprintf(num,"%d",mvid);
			strcat(ack,num);

			// Send a reply msg - maybe
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.length = strlen(ack) + 1;
			DEBUG('w', "Server sending acknowledgement: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack); 

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		/************************************ 
		// HANDLE DESTROY SERVER LOCK REQUEST
		************************************/
		else if(!strcmp (requestType,"DSL") || !strcmp (requestType,"FDSL")){

			// Server "behavior" on a Destroy request		
			// Parse the rest of msg
			
			int lid;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d", &lid); 
			
			// make sure the request is being made
			// on the correct server, if not forward
			if(lid/1000 != machineID){
				strcpy(data, "RT = FDSL LID = ");
				sprintf(num,"%d",lid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(lid/1000,inMailHdr.from,data);
				continue;
			}
			
			lid = lid%1000;
			
			//First check if the lock is even in the table
			if(serverLockTable.Get(lid) != 0){
				if(((ServerLock*)serverLockTable.Get(lid))->isFree()){
					DEBUG('w',"Server destroying server lock %s with id %d.\n",((ServerLock*)serverLockTable.Get(lid))->getName(),lid);
					delete (Lock*)serverLockTable.Remove(lid);
					lid = 1;
				} else{
					DEBUG('w',"Server marking for deletion server lock %s with id %d.\n",((ServerLock*)serverLockTable.Get(lid))->getName(), lid);
					((ServerLock*)serverLockTable.Get(lid))->markForDeletion();
					lid = 0;
				}
				
				//and give back the lock number
				strcpy(ack, "LID = ");
				sprintf(num,"%d",(machineID*1000)+lid);
				strcat(ack,num);

				// Send a reply msg - maybe
				
				// if the message was forwarded,
				// we need to parse the "TO" field
				// to see who to reply to
				if(!strcmp (requestType,"FDSL"))
					sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %d", &inPktHdr.from); 
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack);
			}
			else{
				//lock requested isn't in our table
				lid = -1;
				//tell them they suck
				strcpy(ack, "LID = ");
				sprintf(num,"%d",lid);
				strcat(ack,num);

				// Send a reply msg - maybe
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack);
			}

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		} 
		/***************************************** 
		// HANDLE DESTROY SERVER CONDITION REQUEST
		*****************************************/
		else if(!strcmp (requestType,"DSC") || !strcmp (requestType,"FDSC")){

			// Server "behavior" on a Destroy request		
			// Parse the rest of msg
			
			int cid;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d", &cid); 
			
			// make sure the request is being made
			// on the correct server, if not forward
			if(cid/1000 != machineID){
				strcpy(data, "RT = FDSC CID = ");
				sprintf(num,"%d",cid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(cid/1000,inMailHdr.from,data);
				continue;
			}
			
			cid = cid%1000;
			
			if(serverConditionTable.Get(cid) != 0){
				if(((ServerCondition*)serverConditionTable.Get(cid))->isFree()){
					DEBUG('u',"Server destroying server condition %s with id %d.\n",((ServerCondition*)serverConditionTable.Get(cid))->getName(),cid);
					delete (ServerCondition*)serverConditionTable.Remove(cid);
					cid = 1;
				} else{
					DEBUG('u',"Server marking for deletion server condition %s with id %d.\n",((ServerCondition*)serverConditionTable.Get(cid))->getName(), cid);
					((ServerCondition*)serverConditionTable.Get(cid))->markForDeletion();
					cid = 0;
				}
				
				//and give back the condition number
				strcpy(ack, "CID = ");
				sprintf(num,"%d",(machineID*1000)+cid);
				strcat(ack,num);

				// Send a reply msg - maybe
				
				// if the message was forwarded,
				// we need to parse the "TO" field
				// to see who to reply to
				if(!strcmp (requestType,"FDSC"))
					sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %d", &inPktHdr.from); 
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 
			}
			else{
				//condition requested isn't in our table
				cid = -1;
				//tell them they suck
				strcpy(ack, "CID = ");
				sprintf(num,"%d",cid);
				strcat(ack,num);

				// Send a reply msg - maybe
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack);
			}

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		/***************************************** 
		// HANDLE DESTROY MONITOR VARIABLE REQUEST
		*****************************************/
		else if( !strcmp (requestType,"DMV") || !strcmp (requestType,"FDMV")){
			//DestroyMV
			// Server "behavior" on a Destroy request		
			// Parse the rest of msg
			
			int mvid;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d", &mvid); 

			// make sure the request is being made
			// on the correct server, if not forward
			if(mvid/1000 != machineID){
				strcpy(data, "RT = FDMV MVID = ");
				sprintf(num,"%d",mvid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(mvid/1000,inMailHdr.from,data);
				continue;
			}
			
			mvid = mvid%1000;
			
			//and give back the MV number
			strcpy(ack, "MVID = ");
			sprintf(num,"%d",(machineID*1000)+mvid);
			strcat(ack,num);
			
			//Delete it
			DEBUG('w',"Server destroying server MV %s with id %d.\n",((ServerMV*)serverMVTable.Get(mvid))->getName(),mvid);
			delete (ServerMV*)serverMVTable.Remove(mvid);

			// Send a reply msg - maybe
			
			// if the message was forwarded,
			// we need to parse the "TO" field
			// to see who to reply to
			if(!strcmp (requestType,"FDMV"))
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %d", &inPktHdr.from); 
			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;
			outMailHdr.length = strlen(ack) + 1;
			DEBUG('w', "Server sending acknowledgment: %s\n",ack);
			success = postOffice->Send(outPktHdr, outMailHdr, ack); 

			if ( !success ) {
				DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		/************************************* 
		// HANDLE SET MONITOR VARIABLE REQUEST
		*************************************/
		else if(!strcmp (requestType,"SMV") || !strcmp (requestType,"FSMV") ){
			// Server-Side Handling a SetMV request
			// Get "mv" number and value from msg
			// Validate the number
			int mvid, mvv;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d %*s %*c %d", &mvid, &mvv); 

			// make sure the request is being made
			// on the correct server, if not forward
			if(mvid/1000 != machineID){
				strcpy(data, "RT = FSMV MVID = ");
				sprintf(num,"%d",mvid);
				strcat(data,num);
				strcat(data," MVV = ");
				sprintf(num,"%d",mvv);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(mvid/1000,inMailHdr.from,data);
				continue;
			}

			// if the message was forwarded,
			// we need to parse the "TO" field
			// to see who to reply to
			if(!strcmp (requestType,"FSMV"))
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %*d %*s %*c %d", &inPktHdr.from); 

			mvid = mvid%1000;

			if(serverMVTable.Get(mvid) != 0){
				ServerMV* mv = (ServerMV*)serverMVTable.Get(mvid);
				
				
				//Set the value
				DEBUG('w',"SetMV syscall called setting MV %s, using id %d, with value %d.\n",((ServerMV*)serverMVTable.Get(mvid))->getName(),mvid,mvv);
				mv->value = mvv;
				
				// send reply msg
				strcpy(ack, "MVID = ");
				sprintf(num,"%d",mvid);
				strcat(ack,num);
				strcat(ack, " MVV = ");
				sprintf(num,"%d",mvv);
				strcat(ack,num);
				
				// Send a reply msg - maybe
				
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
			}
			// not a valid #
			else {
				// create error reply msg & send
				DEBUG('w', "Server unable to setMV because invalid id %d was passed!",mvid);
				mvid = -1;
				mvv = -1;
				strcpy(ack, "MVID = ");
				sprintf(num,"%d",mvid);
				strcat(ack,num);
				strcat(ack, " MVV = ");
				sprintf(num,"%d",mvv);
				strcat(ack,num);
				
				// Send a reply msg - maybe
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
			}
		}
		/************************************* 
		// HANDLE GET MONITOR VARIABLE REQUEST
		*************************************/
		else if(!strcmp (requestType,"GMV") || !strcmp (requestType,"FGMV")){
			// Server-Side Handling a SetMV request
			// Get "mv" number from msg
			// Validate the number
			int mvid, mvv;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d", &mvid); 

			// make sure the request is being made
			// on the correct server, if not forward
			if(mvid/1000 != machineID){
				strcpy(data, "RT = FGMV MVID = ");
				sprintf(num,"%d",mvid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(mvid/1000,inMailHdr.from,data);
				continue;
			}
			
			// if the message was forwarded,
			// we need to parse the "TO" field
			// to see who to reply to
			if(!strcmp (requestType,"FGMV"))
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %d", &inPktHdr.from); 
			
			mvid = mvid % 1000;
			
			if(serverMVTable.Get(mvid) != 0){
				ServerMV* mv = (ServerMV*)serverMVTable.Get(mvid);
				
				//Get the value
				DEBUG('w',"GetMV syscall called getting MV %s, using id %d\n",((ServerMV*)serverMVTable.Get(mvid))->getName(),mvid);
				mvv = mv->value;
				
				// send reply msg
				strcpy(ack, "MVID = ");
				sprintf(num,"%d",mvid);
				strcat(ack,num);
				strcat(ack, " MVV = ");
				sprintf(num,"%d",mvv);
				strcat(ack,num);
				
				// Send a reply msg - maybe
				
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;				
				
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
			}
			// not a valid #
			else {
				// create error reply msg & send
				mvid = -1;
				mvv = -1;
				strcpy(ack, "MVID = ");
				sprintf(num,"%d",mvid);
				strcat(ack,num);
				strcat(ack, " MVV = ");
				sprintf(num,"%d",mvv);
				strcat(ack,num);
				
				// Send a reply msg - maybe
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;				
				
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
			}
		}
		/******************************* 
		// HANDLE SERVER ACQUIRE REQUEST
		*******************************/
		else if( !strcmp (requestType,"SA") || !strcmp (requestType,"FSA") || !strcmp (requestType,"SSA") || !strcmp (requestType,"BSA")){
			// Server-Side Handling an Acquire request
			// Get "lock" number from msg
			// Validate the number
			int lid;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d", &lid); 

			int addr = (inPktHdr.from*1000) + (inMailHdr.from);
			char newMailData[MaxMailSize] = "";
			int newMailAddr;	

			
			// make sure the request is being made
			// on the correct server, if not forward
			if(lid/1000 != machineID){
				strcpy(data, "RT = FSA LID = ");
				sprintf(num,"%d",lid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",(inPktHdr.from*1000)+inMailHdr.from);
				strcat(data,num);
				forwardMessage(lid/1000,inMailHdr.from,data);
				continue;
			}

			lid = lid%1000;
			// if the message was forwarded,
			// we need to parse the "TO" field
			// to see who to reply to
			if(!strcmp (requestType,"FSA")){
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %d", &addr); 
				inPktHdr.from = addr/1000;
			}
			if(!strcmp (requestType,"SSA")||!strcmp (requestType,"BSA"))
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %d %*s %*c %d %*s %*c %s", &addr,&newMailAddr,newMailData);

			outPktHdr.to = inPktHdr.from;
			outMailHdr.to = inMailHdr.from;

			if(serverLockTable.Get(lid) != 0){
				ServerLock* lock = (ServerLock*)serverLockTable.Get(lid);
				if(lock->isFree()){					
					// set owner - machine ID/mailbox #
					if(!strcmp (requestType,"SSA")){ //Need to acquire on behalf of the waiter
						DEBUG('w',"ServerAcquire syscall via SignalHelper: acquiring FREE lock %s with id %d on behalf of waiter and setting lock->owner to %d and lock state to BUSY.\n",lock->getName(),lid,newMailAddr);
						lock->owner = newMailAddr;
					}
					else if(!strcmp (requestType,"BSA")){ //Need to acquire on behalf of the waiter
						DEBUG('w',"ServerAcquire syscall via BroadcastHelper: acquiring FREE lock %s with id %d on behalf of waiter and setting lock->owner to %d and lock state to BUSY.\n",lock->getName(),lid,newMailAddr);
						lock->owner = newMailAddr;
					}
					else{
						DEBUG('w',"ServerAcquire syscall: acquiring FREE lock %s with id %d and setting lock->owner to %d and lock state to BUSY.\n",lock->getName(),lid,addr);
						lock->owner = addr;
					}
					// set state to "busy"
					lock->state = ServerLock::BUSY;
					
					// send reply msg
					strcpy(ack, "LID = ");
					sprintf(num,"%d",(machineID*1000)+lid);
					strcat(ack,num);
					outMailHdr.length = strlen(ack) + 1;
					
					// Send a reply msg - maybe
					if(!strcmp (requestType,"SSA")){
						// send reply msg to a thread waiting for a release
						outPktHdr.to = newMailAddr/1000;
						outMailHdr.to = newMailAddr%1000;
						DEBUG('w',"SignalHelper sent the next message on queue for condition with lock %s, using id %d.\n",lock->getName(),lid);
						DEBUG('w', "SignalHelper sending acknowledgement: %s\n",newMailData);
						success = postOffice->Send(outPktHdr, outMailHdr, newMailData); 

						if ( !success ) {
							DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
							interrupt->Halt();
						}
					}
					else if(!strcmp (requestType,"BSA")){
						// send reply msg to a thread waiting for a release
						outPktHdr.to = newMailAddr/1000;
						outMailHdr.to = newMailAddr%1000;
						DEBUG('w',"BroadcastHelper sent the next message on queue for condition with lock %s, using id %d.\n",lock->getName(),lid);
						DEBUG('w', "BroadcastHelper sending acknowledgement: %s\n",newMailData);
						success = postOffice->Send(outPktHdr, outMailHdr, newMailData); 

						if ( !success ) {
							DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
							interrupt->Halt();
						}
					}
					else{
						DEBUG('w', "Server sending acknowledgement: %s\n",ack);	
						success = postOffice->Send(outPktHdr, outMailHdr, ack); 
					}
					if ( !success ) {
						DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}
				}				
				// lock not available
				else { 
					if(lock->owner == addr ){
						DEBUG('w',"ServerAcquire syscall called on lock %s with %d by its owner %d, equivalent to an empty acquire.\n",lock->getName(),lid,addr);
						// create error reply msg & send
						strcpy(ack, "LID = ");
						sprintf(num,"%d",(machineID*1000)+lid);
						strcat(ack,num);
						
						// Send a reply msg - maybe
						outMailHdr.length = strlen(ack) + 1;
						
						DEBUG('w', "Server sending acknowledgement: %s\n",ack);
						success = postOffice->Send(outPktHdr, outMailHdr, ack); 

						if ( !success ) {
							DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
							interrupt->Halt();
						}
					}
					else{
						if(!strcmp (requestType,"SSA")){
						
							PacketHeader outPktHdrSSA;
							MailHeader outMailHdrSSA;	
							outMailHdrSSA.from = 0;
							outPktHdrSSA.to = newMailAddr/1000;
							outMailHdrSSA.to = newMailAddr%1000;
							//printf("outPktHdr.from = %d outMailHdr.t
							Mail* newerMail = new Mail(outPktHdrSSA, outMailHdrSSA, newMailData);
							lock->waitQueue->Append(newerMail);
							DEBUG('w',"Server via SignalHelper acquired a BUSY lock %s so message placed on lock's waitQueue.\n",lock->getName());
							
							// create error reply msg & send
							strcpy(ack, "LID = ");
							sprintf(num,"%d",(machineID*1000)+lid);
							strcat(ack,num);
							
							// Send a reply msg - maybe
							outMailHdr.length = strlen(ack) + 1;
							
							DEBUG('w', "Server sending acknowledgement: %s\n",ack);
							success = postOffice->Send(outPktHdr, outMailHdr, ack); 

							if ( !success ) {
								DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
								interrupt->Halt();
							}
						}
						else if(!strcmp (requestType,"BSA")){
						
							PacketHeader outPktHdrBSA;
							MailHeader outMailHdrBSA;	
							outMailHdrBSA.from = 0;
							outPktHdrBSA.to = newMailAddr/1000;
							outMailHdrBSA.to = newMailAddr%1000;
							//printf("outPktHdr.from = %d outMailHdr.t
							Mail* newerMail = new Mail(outPktHdrBSA, outMailHdrBSA, newMailData);
							lock->waitQueue->Append(newerMail);
							DEBUG('w',"Server via BroadcastHelper acquired a BUSY lock %s so message placed on lock's waitQueue.\n",lock->getName());
							
							// create error reply msg & send
							strcpy(ack, "LID = ");
							sprintf(num,"%d",(machineID*1000)+lid);
							strcat(ack,num);
							
							// Send a reply msg - maybe
							outMailHdr.length = strlen(ack) + 1;
							
							DEBUG('w', "Server sending acknowledgement: %s\n",ack);
							success = postOffice->Send(outPktHdr, outMailHdr, ack); 

							if ( !success ) {
								DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
								interrupt->Halt();
							}
						}
						else{
							// make the reply msg
							strcpy(ack, "LID = ");
							sprintf(num,"%d",(machineID*1000)+lid);
							strcat(ack,num);
							
							// Send a reply msg - maybe
							outMailHdr.length = strlen(ack) + 1;
							
							// put reply msg in wait Q
							DEBUG('w',"ServerAcquire syscall called on BUSY lock %s using id %d so message placed on lock's waitQueue.\n",lock->getName(),lid);
							Mail* newMail = new Mail(outPktHdr, outMailHdr, ack);
							lock->waitQueue->Append(newMail);
						}
					}
				}
			}
			// not a valid #
			else {
				// create error reply msg & send
				lid = -1;
				strcpy(ack, "LID = ");
				sprintf(num,"%d",lid);
				strcat(ack,num);
				
				// Send a reply msg - maybe
				outMailHdr.length = strlen(ack) + 1;
				
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
			}
		}
		/******************************* 
		// HANDLE SERVER RELEASE REQUEST
		*******************************/
		else if( !strcmp (requestType,"SR") || !strcmp (requestType,"FSR") || !strcmp (requestType,"WSR")){
			// Server-Side Handling a Release request
			// Get "lock" number from msg
			// Validate the number
			int lid;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d", &lid); 
			
			int addr = (inPktHdr.from*1000) + (inMailHdr.from);

			// make sure the request is being made
			// on the correct server, if not forward
			if(lid/1000 != machineID){
				strcpy(data, "RT = FSR LID = ");
				sprintf(num,"%d",lid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(lid/1000,inMailHdr.from,data);
				continue;
			}

			lid = lid%1000;
			// if the message was forwarded,
			// we need to parse the "TO" field
			// to see who to reply to
			if(!strcmp (requestType,"FSR")){
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %d", &inPktHdr.from);
				addr = (inPktHdr.from*1000) + (inMailHdr.from);	
			}
			if(!strcmp (requestType,"WSR"))
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %d", &addr);

			if(serverLockTable.Get(lid) != 0){
				ServerLock* lock = (ServerLock*)serverLockTable.Get(lid);
				if(lock->owner!=addr){
					//thread that sent this request wasn't the owner!
									// create error reply msg & send
					DEBUG('w',"ServerRelease syscall tried to release lock %s with id %d but %d wasn't the owner\n",lock->getName(),lid,addr);
					lid = -1;
					strcpy(ack, "LID = ");
					sprintf(num,"%d",lid);
					strcat(ack,num);
					
					// Send a reply msg - maybe
					
					outPktHdr.to = inPktHdr.from;
					outMailHdr.to = inMailHdr.from;
					outMailHdr.length = strlen(ack) + 1;
					
					DEBUG('w', "Server sending acknowledgement: %s\n",ack);
					success = postOffice->Send(outPktHdr, outMailHdr, ack); 

					if ( !success ) {
						DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}
					
				}
				else{
					if(!lock->waitQueue->IsEmpty()){
						//someone was waiting for this lock, so reply to them
						Mail* newMail=(Mail*)lock->waitQueue->Remove();
						
						//set new owner
						lock->owner = newMail->pktHdr.to * 1000 + newMail->mailHdr.to;
						
						// send reply msg to a thread waiting for a release
						if(!strcmp (requestType,"WSR"))
							DEBUG('w',"ServerRelease syscall via WaitHelper: waitQueue for lock %s with id %d is NOT EMPTY so setting lock->owner to %d and sending the next message %s to waiter.\n",lock->getName(),lid,newMail->pktHdr.to * 1000 + newMail->mailHdr.to,newMail->data);
						else
							DEBUG('w',"ServerRelease syscall: waitQueue for lock %s with id %d is NOT EMPTY so setting lock->owner to %d and sending the next message %s to waiter.\n",lock->getName(),lid,newMail->pktHdr.to * 1000 + newMail->mailHdr.to,newMail->data);
						success = postOffice->Send(newMail->pktHdr, newMail->mailHdr, newMail->data); 
						if ( !success ) {
							DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
							interrupt->Halt();
						}
					
						strcpy(ack, "LID = ");
						sprintf(num,"%d",(machineID*1000)+lid);
						strcat(ack,num);
						
						// Send a reply msg - maybe
						outPktHdr.to = inPktHdr.from;
						outMailHdr.to = inMailHdr.from;
						outMailHdr.length = strlen(ack) + 1;
						if(!strcmp (requestType,"WSR"))
							DEBUG('w',"ServerRelease syscall via WaitHelper released lock %s, using id %d.\n",lock->getName(),lid);
						else
							DEBUG('w',"ServerRelease syscall released lock %s, using id %d.\n",lock->getName(),lid);
						DEBUG('w', "Server sending acknowledgement: %s\n",ack);
						success = postOffice->Send(outPktHdr, outMailHdr, ack);
						
						if ( !success ) {
							DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
							interrupt->Halt();
						}
						
						
					}
					else { 
						// there was no one waiting for the lock, so update it's status/owner
						lock->owner = NULL;
						lock->state = ServerLock::FREE;
						if(!strcmp (requestType,"WSR"))
							DEBUG('w',"ServerRelease syscall via WaitHelper: waitQueue for lock %s with id %d was EMPTY so lock released (lock->owner set to NULL and lock->state set to FREE).\n",lock->getName(),lid);
						else
							DEBUG('w',"ServerRelease syscall: waitQueue for lock %s with id %d was EMPTY so lock released (lock->owner set to NULL and lock->state set to FREE).\n",lock->getName(),lid);
						
						if(!strcmp (requestType,"WSR")){
							if(lock->isMarked()){
								//delete and remove, then set lid to special value to signify that it is now gone
								DEBUG('w',"ServerRelease syscall via WaitHelper deleting lock %s with id %d which was marked for deletion and setting lid to -2 (special value).\n",lock->getName(),lid);
								delete ((ServerLock*)serverLockTable.Remove(lid));
								lid = -2;
							}
						}
						strcpy(ack, "LID = ");
						sprintf(num,"%d",(machineID*1000)+lid);
						strcat(ack,num);
						
						// Send a reply msg - maybe
						outPktHdr.to = inPktHdr.from;
						outMailHdr.to = inMailHdr.from;
						outMailHdr.length = strlen(ack) + 1;
						
						DEBUG('w', "Server sending acknowledgement: %s\n",ack);
						success = postOffice->Send(outPktHdr, outMailHdr, ack);
						
						if ( !success ) {
							DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
							interrupt->Halt();
						}
					}
				}
			}
			// not a valid #
			else {
				// create error reply msg & send
				lid = -1;
				strcpy(ack, "LID = ");
				sprintf(num,"%d",lid);
				strcat(ack,num);
				
				// Send a reply msg - maybe
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;
				if(!strcmp (requestType,"WSR"))
					DEBUG('w',"Server via WaitHelper tried to released an invalid lock using id %d.\n",lid);
				else				
					DEBUG('w',"Server tried to released an invalid lock using id %d.\n",lid);
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
			}
		}
		/**************************** 
		// HANDLE SERVER WAIT REQUEST
		****************************/
		else if(!strcmp (requestType,"SW") || !strcmp (requestType,"FSW")){
			// Wait
			// never has a reply msg to the waiter,
			// always put something in wait Q, don't 
			// forget to give up the lock - Release
			
			// Server-Side Handling a Wait request
			// Get "condition" and "lock" number from msg
			// Validate the number
			
			int cid, lid;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d %*s %*c %d", &cid, &lid);

			// make sure the request is being made
			// on the correct server, if not forward
			if(cid/1000 != machineID){
				strcpy(data, "RT = FSW CID = ");
				sprintf(num,"%d",cid);
				strcat(data,num);
				strcat(data," LID = ");
				sprintf(num,"%d",lid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(cid/1000,inMailHdr.from,data);
				continue;
			}
			
			Thread* t = new Thread(buffer); //the thread's name is the lock name
			
			if(!strcmp (requestType,"FSW")){
				int to;
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %*d %*s %*c %d", &to);
				inPktHdr.from = to;
			}
			int i = inPktHdr.from * 1000 + inMailHdr.from; //this should give the helper function a unique number
			DEBUG('w',"Server forking helper thread to handle server wait request.\n");
			t->Fork((VoidFunctionPtr)serverWaitRequest, i);
		
		}
		/****************************** 
		// HANDLE SERVER SIGNAL REQUEST
		******************************/
		else if(!strcmp (requestType,"SS")||!strcmp (requestType,"FSS")){
			// Signal
			// wake up a thread by sending a reply msg
			// always send a reply to signaler
			// what if there is a waiter?
				// pull a msg out of wait q & do 
				// an Acquire
				
			// Server-Side Handling a Signal request
			// Get "condition" and "lock" number from msg
			// Validate the number
			int cid, lid;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d %*s %*c %d", &cid, &lid); 

			// make sure the request is being made
			// on the correct server, if not forward
			if(cid/1000 != machineID){
				strcpy(data, "RT = FSS CID = ");
				sprintf(num,"%d",cid);
				strcat(data,num);
				strcat(data," LID = ");
				sprintf(num,"%d",lid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(cid/1000,inMailHdr.from,data);
				continue;
			}
			
			Thread* t = new Thread(buffer); //the thread's name is the lock name
			
			if(!strcmp (requestType,"FSS")){
				int to;
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %*d %*s %*c %d", &to);
				inPktHdr.from = to;
			}
			int i = inPktHdr.from * 1000 + inMailHdr.from; //this should give the helper function a unique number
			DEBUG('w',"Server forking helper thread to handle server signal request.\n");
			t->Fork((VoidFunctionPtr)serverSignalRequest, i);

		}
		/**************************************** 
		// HANDLE CHECK SERVER LOCK OWNER REQUEST
		****************************************/
		else if(!strcmp (requestType,"CHSLO") ){
			
			int lid, addr;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d %*s %*c %d", &lid,&addr); 

			// make sure the request is being made
			// on the correct server, if not forward
			if(lid/1000 != machineID){
				DEBUG('w',"Server received lock %d which is not from this server's table!",lid);
				continue;
			}

			lid = lid%1000;
			
			if(serverLockTable.Get(lid) != 0){
				ServerLock* lock = (ServerLock*)serverLockTable.Get(lid);
				if(lock->owner!=addr){
					//thread that sent this request wasn't the owner!
									// create error reply msg & send
					DEBUG('w',"Server: addr %d wasn't the owner of lock %s with id %d\n",addr,lock->getName(),lid);
					lid = -1;
					strcpy(ack, "LID = ");
					sprintf(num,"%d",lid);
					strcat(ack,num);
					
					// Send a reply msg - maybe
					
					outPktHdr.to = inPktHdr.from;
					outMailHdr.to = inMailHdr.from;
					outMailHdr.length = strlen(ack) + 1;
					
					DEBUG('w', "Server sending acknowledgement: %s\n",ack);
					success = postOffice->Send(outPktHdr, outMailHdr, ack); 

					if ( !success ) {
						DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}
					
				}
				else{
					strcpy(ack, "LID = ");
					sprintf(num,"%d",(machineID*1000)+lid);
					strcat(ack,num);
					
					// Send a reply msg - maybe
					outPktHdr.to = inPktHdr.from;
					outMailHdr.to = inMailHdr.from;
					outMailHdr.length = strlen(ack) + 1;
					
					DEBUG('w',"Server: addr %d was the owner of lock %s with id %d\n",addr,lock->getName(),lid);
					DEBUG('w', "Server sending acknowledgement: %s\n",ack);
					success = postOffice->Send(outPktHdr, outMailHdr, ack);
					
					if ( !success ) {
						DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}	
				}
			}
			// not a valid #
			else {
				// create error reply msg & send
				lid = -1;
				strcpy(ack, "LID = ");
				sprintf(num,"%d",lid);
				strcat(ack,num);
				
				// Send a reply msg - maybe
				outPktHdr.to = inPktHdr.from;
				outMailHdr.to = inMailHdr.from;
				outMailHdr.length = strlen(ack) + 1;				
				DEBUG('w',"Server tried to access an invalid lock using id %d.\n",lid);
				DEBUG('w', "Server sending acknowledgement: %s\n",ack);
				success = postOffice->Send(outPktHdr, outMailHdr, ack); 

				if ( !success ) {
					DEBUG('w',"The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
			}

		}
		/********************************* 
		// HANDLE SERVER BROADCAST REQUEST
		*********************************/
		else if(!strcmp (requestType,"SB") || !strcmp (requestType,"FSB")){
			// Broadcast
			// wake up all thread by sending a reply msg
			// always send a reply to signaler
			// what if there is a waiter?
				// pull a msg out of wait q & do 
				// an Acquire
				
			// Server-Side Handling a Signal request
			// Get "condition" and "lock" number from msg
			// Validate the number
			int cid, lid;
			sscanf(buffer,"%*s %*c %*s %*s %*c %d %*s %*c %d", &cid, &lid); 

			// make sure the request is being made
			// on the correct server, if not forward
			if(cid/1000 != machineID){
				strcpy(data, "RT = FSB CID = ");
				sprintf(num,"%d",cid);
				strcat(data,num);
				strcat(data," LID = ");
				sprintf(num,"%d",lid);
				strcat(data,num);
				strcat(data," TO = ");
				sprintf(num,"%d",inPktHdr.from);
				strcat(data,num);
				forwardMessage(cid/1000,inMailHdr.from,data);
				continue;
			}
			
			Thread* t = new Thread(buffer); //the thread's name is the lock name
			
			if(!strcmp (requestType,"FSB")){
				int to;
				sscanf(buffer,"%*s %*c %*s %*s %*c %*d %*s %*c %*d %*s %*c %d", &to);
				inPktHdr.from = to;
			}
			int i = inPktHdr.from * 1000 + inMailHdr.from; //this should give the helper function a unique number
			DEBUG('w',"Server forking helper thread to handle server broadcast request.\n");
			t->Fork((VoidFunctionPtr)serverBroadcastRequest, i);
		}
	}
}
