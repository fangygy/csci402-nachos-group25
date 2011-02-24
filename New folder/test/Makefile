# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
#       location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
#       to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = /auto/home-scf-22/csci402/testgnu/decstation-ultrix/bin/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult sort testfiles testYield testFork testExec testPrint\
	testCreateLock testCreateCondition testDestroyLock testDestroyCondition\
	testAcquire testAcquire2 testRelease testRelease2 testSignal testWait\
	testBroadcast trainsim multiTrain multiThread test2Matmult test2Sort\
	test2MatmultExec test2SortExec test2Mixed test2MixedExec testCreateServerLock testCreateServerCondition\
	testDestroyServerLock testDestroyServerCondition testServerAcquire testServerAcquire2\
	testServerRelease testServerRelease2 testCreateServerMV testSetServerMV\
	testGetServerMV testGetServerMV2 testDestroyServerMV testServerWait\
	testServerSignal testServerBroadcast simpleNetwork simpleNetwork2 testNetworkLockKill\
	passenger conductor chef steward porter coachAttendant waiter ticketChecker\
	nettrain fullNettrain
	
nettrain: passenger conductor chef steward porter coachAttendant waiter ticketChecker\
	nettrain1 nettrain2 nettrain3 nettrain4 fullNettrain

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles

testYield.o: testYield.c
	$(CC) $(CFLAGS) -c testYield.c
testYield: testYield.o start.o
	$(LD) $(LDFLAGS) start.o testYield.o -o testYield.coff
	../bin/coff2noff testYield.coff testYield
	
testFork.o: testFork.c
	$(CC) $(CFLAGS) -c testFork.c
testFork: testFork.o start.o
	$(LD) $(LDFLAGS) start.o testFork.o -o testFork.coff
	../bin/coff2noff testFork.coff testFork		
	
trainsim.o: trainsim.c
	$(CC) $(CFLAGS) -c trainsim.c
trainsim: trainsim.o start.o
	$(LD) $(LDFLAGS) start.o trainsim.o -o trainsim.coff
	../bin/coff2noff trainsim.coff trainsim

testExec.o: testExec.c
	$(CC) $(CFLAGS) -c testExec.c
testExec: testExec.o start.o
	$(LD) $(LDFLAGS) start.o testExec.o -o testExec.coff
	../bin/coff2noff testExec.coff testExec

testPrint.o: testPrint.c
	$(CC) $(CFLAGS) -c testPrint.c
testPrint: testPrint.o start.o
	$(LD) $(LDFLAGS) start.o testPrint.o -o testPrint.coff
	../bin/coff2noff testPrint.coff testPrint

testCreateLock.o: testCreateLock.c
	$(CC) $(CFLAGS) -c testCreateLock.c
testCreateLock: testCreateLock.o start.o
	$(LD) $(LDFLAGS) start.o testCreateLock.o -o testCreateLock.coff
	../bin/coff2noff testCreateLock.coff testCreateLock 
	
testCreateCondition.o: testCreateCondition.c
	$(CC) $(CFLAGS) -c testCreateCondition.c
testCreateCondition: testCreateCondition.o start.o
	$(LD) $(LDFLAGS) start.o testCreateCondition.o -o testCreateCondition.coff
	../bin/coff2noff testCreateCondition.coff testCreateCondition

testDestroyLock.o: testDestroyLock.c
	$(CC) $(CFLAGS) -c testDestroyLock.c
testDestroyLock: testDestroyLock.o start.o
	$(LD) $(LDFLAGS) start.o testDestroyLock.o -o testDestroyLock.coff
	../bin/coff2noff testDestroyLock.coff testDestroyLock 
	
testDestroyCondition.o: testDestroyCondition.c
	$(CC) $(CFLAGS) -c testDestroyCondition.c
testDestroyCondition: testDestroyCondition.o start.o
	$(LD) $(LDFLAGS) start.o testDestroyCondition.o -o testDestroyCondition.coff
	../bin/coff2noff testDestroyCondition.coff testDestroyCondition 

testAcquire.o: testAcquire.c
	$(CC) $(CFLAGS) -c testAcquire.c
testAcquire: testAcquire.o start.o
	$(LD) $(LDFLAGS) start.o testAcquire.o -o testAcquire.coff
	../bin/coff2noff testAcquire.coff testAcquire 

testRelease.o: testRelease.c
	$(CC) $(CFLAGS) -c testRelease.c
testRelease: testRelease.o start.o
	$(LD) $(LDFLAGS) start.o testRelease.o -o testRelease.coff
	../bin/coff2noff testRelease.coff testRelease 

testAcquire2.o: testAcquire2.c
	$(CC) $(CFLAGS) -c testAcquire2.c
testAcquire2: testAcquire2.o start.o
	$(LD) $(LDFLAGS) start.o testAcquire2.o -o testAcquire2.coff
	../bin/coff2noff testAcquire2.coff testAcquire2 

testRelease2.o: testRelease2.c
	$(CC) $(CFLAGS) -c testRelease2.c
testRelease2: testRelease2.o start.o
	$(LD) $(LDFLAGS) start.o testRelease2.o -o testRelease2.coff
	../bin/coff2noff testRelease2.coff testRelease2 

testSignal.o: testSignal.c
	$(CC) $(CFLAGS) -c testSignal.c
testSignal: testSignal.o start.o
	$(LD) $(LDFLAGS) start.o testSignal.o -o testSignal.coff
	../bin/coff2noff testSignal.coff testSignal 

testWait.o: testWait.c
	$(CC) $(CFLAGS) -c testWait.c
testWait: testWait.o start.o
	$(LD) $(LDFLAGS) start.o testWait.o -o testWait.coff
	../bin/coff2noff testWait.coff testWait 

testBroadcast.o: testBroadcast.c
	$(CC) $(CFLAGS) -c testBroadcast.c
testBroadcast: testBroadcast.o start.o
	$(LD) $(LDFLAGS) start.o testBroadcast.o -o testBroadcast.coff
	../bin/coff2noff testBroadcast.coff testBroadcast 
			
multiTrain.o: multiTrain.c
	$(CC) $(CFLAGS) -c multiTrain.c
multiTrain: multiTrain.o start.o
	$(LD) $(LDFLAGS) start.o multiTrain.o -o multiTrain.coff
	../bin/coff2noff multiTrain.coff multiTrain 
	
multiThread.o: multiThread.c
	$(CC) $(CFLAGS) -c multiThread.c
multiThread: multiThread.o start.o
	$(LD) $(LDFLAGS) start.o multiThread.o -o multiThread.coff
	../bin/coff2noff multiThread.coff multiThread 

test2Matmult.o: test2Matmult.c
	$(CC) $(CFLAGS) -c test2Matmult.c
test2Matmult: test2Matmult.o start.o
	$(LD) $(LDFLAGS) start.o test2Matmult.o -o test2Matmult.coff
	../bin/coff2noff test2Matmult.coff test2Matmult 
	
test2Sort.o: test2Sort.c
	$(CC) $(CFLAGS) -c test2Sort.c
test2Sort: test2Sort.o start.o
	$(LD) $(LDFLAGS) start.o test2Sort.o -o test2Sort.coff
	../bin/coff2noff test2Sort.coff test2Sort 
	
test2MatmultExec.o: test2MatmultExec.c
	$(CC) $(CFLAGS) -c test2MatmultExec.c
test2MatmultExec: test2MatmultExec.o start.o
	$(LD) $(LDFLAGS) start.o test2MatmultExec.o -o test2MatmultExec.coff
	../bin/coff2noff test2MatmultExec.coff test2MatmultExec 

test2SortExec.o: test2SortExec.c
	$(CC) $(CFLAGS) -c test2SortExec.c
test2SortExec: test2SortExec.o start.o
	$(LD) $(LDFLAGS) start.o test2SortExec.o -o test2SortExec.coff
	../bin/coff2noff test2SortExec.coff test2SortExec 
	
test2Mixed.o: test2Mixed.c
	$(CC) $(CFLAGS) -c test2Mixed.c
test2Mixed: test2Mixed.o start.o
	$(LD) $(LDFLAGS) start.o test2Mixed.o -o test2Mixed.coff
	../bin/coff2noff test2Mixed.coff test2Mixed 
	
test2MixedExec.o: test2MixedExec.c
	$(CC) $(CFLAGS) -c test2MixedExec.c
test2MixedExec: test2MixedExec.o start.o
	$(LD) $(LDFLAGS) start.o test2MixedExec.o -o test2MixedExec.coff
	../bin/coff2noff test2MixedExec.coff test2MixedExec 

testCreateServerLock.o: testCreateServerLock.c
	$(CC) $(CFLAGS) -c testCreateServerLock.c
testCreateServerLock: testCreateServerLock.o start.o
	$(LD) $(LDFLAGS) start.o testCreateServerLock.o -o testCreateServerLock.coff
	../bin/coff2noff testCreateServerLock.coff testCreateServerLock 

testCreateServerCondition.o: testCreateServerCondition.c
	$(CC) $(CFLAGS) -c testCreateServerCondition.c
testCreateServerCondition: testCreateServerCondition.o start.o
	$(LD) $(LDFLAGS) start.o testCreateServerCondition.o -o testCreateServerCondition.coff
	../bin/coff2noff testCreateServerCondition.coff testCreateServerCondition

testDestroyServerLock.o: testDestroyServerLock.c
	$(CC) $(CFLAGS) -c testDestroyServerLock.c
testDestroyServerLock: testDestroyServerLock.o start.o
	$(LD) $(LDFLAGS) start.o testDestroyServerLock.o -o testDestroyServerLock.coff
	../bin/coff2noff testDestroyServerLock.coff testDestroyServerLock 

testDestroyServerCondition.o: testDestroyServerCondition.c
	$(CC) $(CFLAGS) -c testDestroyServerCondition.c
testDestroyServerCondition: testDestroyServerCondition.o start.o
	$(LD) $(LDFLAGS) start.o testDestroyServerCondition.o -o testDestroyServerCondition.coff
	../bin/coff2noff testDestroyServerCondition.coff testDestroyServerCondition 

testServerAcquire.o: testServerAcquire.c
	$(CC) $(CFLAGS) -c testServerAcquire.c
testServerAcquire: testServerAcquire.o start.o
	$(LD) $(LDFLAGS) start.o testServerAcquire.o -o testServerAcquire.coff
	../bin/coff2noff testServerAcquire.coff testServerAcquire 

testServerAcquire2.o: testServerAcquire2.c
	$(CC) $(CFLAGS) -c testServerAcquire2.c
testServerAcquire2: testServerAcquire2.o start.o
	$(LD) $(LDFLAGS) start.o testServerAcquire2.o -o testServerAcquire2.coff
	../bin/coff2noff testServerAcquire2.coff testServerAcquire2 

testServerRelease.o: testServerRelease.c
	$(CC) $(CFLAGS) -c testServerRelease.c
testServerRelease: testServerRelease.o start.o
	$(LD) $(LDFLAGS) start.o testServerRelease.o -o testServerRelease.coff
	../bin/coff2noff testServerRelease.coff testServerRelease  

testServerRelease2.o: testServerRelease2.c
	$(CC) $(CFLAGS) -c testServerRelease2.c
testServerRelease2: testServerRelease2.o start.o
	$(LD) $(LDFLAGS) start.o testServerRelease2.o -o testServerRelease2.coff
	../bin/coff2noff testServerRelease2.coff testServerRelease2 

testCreateServerMV.o: testCreateServerMV.c
	$(CC) $(CFLAGS) -c testCreateServerMV.c
testCreateServerMV: testCreateServerMV.o start.o
	$(LD) $(LDFLAGS) start.o testCreateServerMV.o -o testCreateServerMV.coff
	../bin/coff2noff testCreateServerMV.coff testCreateServerMV 

testSetServerMV.o: testSetServerMV.c
	$(CC) $(CFLAGS) -c testSetServerMV.c
testSetServerMV: testSetServerMV.o start.o
	$(LD) $(LDFLAGS) start.o testSetServerMV.o -o testSetServerMV.coff
	../bin/coff2noff testSetServerMV.coff testSetServerMV 
	
testGetServerMV.o: testGetServerMV.c
	$(CC) $(CFLAGS) -c testGetServerMV.c
testGetServerMV: testGetServerMV.o start.o
	$(LD) $(LDFLAGS) start.o testGetServerMV.o -o testGetServerMV.coff
	../bin/coff2noff testGetServerMV.coff testGetServerMV 

testGetServerMV2.o: testGetServerMV2.c
	$(CC) $(CFLAGS) -c testGetServerMV2.c
testGetServerMV2: testGetServerMV2.o start.o
	$(LD) $(LDFLAGS) start.o testGetServerMV2.o -o testGetServerMV2.coff
	../bin/coff2noff testGetServerMV2.coff testGetServerMV2 

testDestroyServerMV.o: testDestroyServerMV.c
	$(CC) $(CFLAGS) -c testDestroyServerMV.c
testDestroyServerMV: testDestroyServerMV.o start.o
	$(LD) $(LDFLAGS) start.o testDestroyServerMV.o -o testDestroyServerMV.coff
	../bin/coff2noff testDestroyServerMV.coff testDestroyServerMV 

testServerWait.o: testServerWait.c
	$(CC) $(CFLAGS) -c testServerWait.c
testServerWait: testServerWait.o start.o
	$(LD) $(LDFLAGS) start.o testServerWait.o -o testServerWait.coff
	../bin/coff2noff testServerWait.coff testServerWait 

testServerSignal.o: testServerSignal.c
	$(CC) $(CFLAGS) -c testServerSignal.c
testServerSignal: testServerSignal.o start.o
	$(LD) $(LDFLAGS) start.o testServerSignal.o -o testServerSignal.coff
	../bin/coff2noff testServerSignal.coff testServerSignal 

testServerBroadcast.o: testServerBroadcast.c
	$(CC) $(CFLAGS) -c testServerBroadcast.c
testServerBroadcast: testServerBroadcast.o start.o
	$(LD) $(LDFLAGS) start.o testServerBroadcast.o -o testServerBroadcast.coff
	../bin/coff2noff testServerBroadcast.coff testServerBroadcast
	
simpleNetwork.o: simpleNetwork.c
	$(CC) $(CFLAGS) -c simpleNetwork.c
simpleNetwork: simpleNetwork.o start.o
	$(LD) $(LDFLAGS) start.o simpleNetwork.o -o simpleNetwork.coff
	../bin/coff2noff simpleNetwork.coff simpleNetwork  
	
simpleNetwork2.o: simpleNetwork2.c
	$(CC) $(CFLAGS) -c simpleNetwork2.c
simpleNetwork2: simpleNetwork2.o start.o
	$(LD) $(LDFLAGS) start.o simpleNetwork2.o -o simpleNetwork2.coff
	../bin/coff2noff simpleNetwork2.coff simpleNetwork2 

testNetworkLockKill.o: testNetworkLockKill.c
	$(CC) $(CFLAGS) -c testNetworkLockKill.c
testNetworkLockKill: testNetworkLockKill.o start.o
	$(LD) $(LDFLAGS) start.o testNetworkLockKill.o -o testNetworkLockKill.coff
	../bin/coff2noff testNetworkLockKill.coff testNetworkLockKill
	
passenger.o: passenger.c
	$(CC) $(CFLAGS) -c passenger.c
passenger: passenger.o start.o
	$(LD) $(LDFLAGS) start.o passenger.o -o passenger.coff
	../bin/coff2noff passenger.coff passenger
	
conductor.o: conductor.c
	$(CC) $(CFLAGS) -c conductor.c
conductor: conductor.o start.o
	$(LD) $(LDFLAGS) start.o conductor.o -o conductor.coff
	../bin/coff2noff conductor.coff conductor
	
chef.o: chef.c
	$(CC) $(CFLAGS) -c chef.c
chef: chef.o start.o
	$(LD) $(LDFLAGS) start.o chef.o -o chef.coff
	../bin/coff2noff chef.coff chef
	
steward.o: steward.c
	$(CC) $(CFLAGS) -c steward.c
steward: steward.o start.o
	$(LD) $(LDFLAGS) start.o steward.o -o steward.coff
	../bin/coff2noff steward.coff steward
	
porter.o: porter.c
	$(CC) $(CFLAGS) -c porter.c
porter: porter.o start.o
	$(LD) $(LDFLAGS) start.o porter.o -o porter.coff
	../bin/coff2noff porter.coff porter
	
coachAttendant.o: coachAttendant.c
	$(CC) $(CFLAGS) -c coachAttendant.c
coachAttendant: coachAttendant.o start.o
	$(LD) $(LDFLAGS) start.o coachAttendant.o -o coachAttendant.coff
	../bin/coff2noff coachAttendant.coff coachAttendant

waiter.o: waiter.c
	$(CC) $(CFLAGS) -c waiter.c
waiter: waiter.o start.o
	$(LD) $(LDFLAGS) start.o waiter.o -o waiter.coff
	../bin/coff2noff waiter.coff waiter
	
ticketChecker.o: ticketChecker.c
	$(CC) $(CFLAGS) -c ticketChecker.c
ticketChecker: ticketChecker.o start.o
	$(LD) $(LDFLAGS) start.o ticketChecker.o -o ticketChecker.coff
	../bin/coff2noff ticketChecker.coff ticketChecker
	
nettrain1.o: nettrain1.c
	$(CC) $(CFLAGS) -c nettrain1.c
nettrain1: nettrain1.o start.o
	$(LD) $(LDFLAGS) start.o nettrain1.o -o nettrain1.coff
	../bin/coff2noff nettrain1.coff nettrain1
	
nettrain2.o: nettrain2.c
	$(CC) $(CFLAGS) -c nettrain2.c
nettrain2: nettrain2.o start.o
	$(LD) $(LDFLAGS) start.o nettrain2.o -o nettrain2.coff
	../bin/coff2noff nettrain2.coff nettrain2

nettrain3.o: nettrain3.c
	$(CC) $(CFLAGS) -c nettrain3.c
nettrain3: nettrain3.o start.o
	$(LD) $(LDFLAGS) start.o nettrain3.o -o nettrain3.coff
	../bin/coff2noff nettrain3.coff nettrain3

nettrain4.o: nettrain4.c
	$(CC) $(CFLAGS) -c nettrain4.c
nettrain4: nettrain4.o start.o
	$(LD) $(LDFLAGS) start.o nettrain4.o -o nettrain4.coff
	../bin/coff2noff nettrain4.coff nettrain4
	
fullNettrain.o: fullNettrain.c
	$(CC) $(CFLAGS) -c fullNettrain.c
fullNettrain: fullNettrain.o start.o
	$(LD) $(LDFLAGS) start.o fullNettrain.o -o fullNettrain.coff
	../bin/coff2noff fullNettrain.coff fullNettrain	

clean:
	rm -f *.o *.coff
	rm -f halt matmult sort testfiles testYield testFork testExec testPrint\
		testCreateLock testCreateCondition testDestroyLock testDestroyCondition\
		testAcquire testAcquire2 testRelease testRelease2 testSignal testWait\
		testBroadcast trainsim multiTrain multiThread test2Matmult test2Sort\
		test2MatmultExec test2SortExec test2Mixed test2MixedExec testCreateServerLock testCreateServerCondition\
		testDestroyServerLock testDestroyServerCondition testServerAcquire testServerAcquire2\
		testServerRelease testServerRelease2 testCreateServerMV testSetServerMV\
		testGetServerMV testGetServerMV2 testDestroyServerMV testServerWait\
		testServerSignal testServerBroadcast simpleNetwork simpleNetwork2 testNetworkLockKill\
		passenger conductor chef steward porter coachAttendant waiter ticketChecker\
		nettrain1 nettrain2 nettrain3 nettrain4 fullNettrain
