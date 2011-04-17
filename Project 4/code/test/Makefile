# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
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

all: halt matmult sort testfiles yieldTest lockTest forkTest execTest CVTest execCVTest multiExecTest\
	destroyCVTest writeTest traceTest randTest PassportOffice ExecPassportOffice ExecTwoPassportOffice execPassportTests\
	passportTest1 passportTest2 passportTest3 execPassportTests2 Client Client1A Client1B Client2A Client2B Client2C ClientBad\
	AppClerk PicClerk PassClerk CashClerk

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

yieldTest.o: yieldTest.c
	$(CC) $(CFLAGS) -c yieldTest.c
yieldTest: yieldTest.o start.o
	$(LD) $(LDFLAGS) start.o yieldTest.o -o yieldTest.coff
	../bin/coff2noff yieldTest.coff yieldTest

lockTest.o: lockTest.c
	$(CC) $(CFLAGS) -c lockTest.c
lockTest: lockTest.o start.o
	$(LD) $(LDFLAGS) start.o lockTest.o -o lockTest.coff
	../bin/coff2noff lockTest.coff lockTest

forkTest.o: forkTest.c
	$(CC) $(CFLAGS) -c forkTest.c
forkTest: forkTest.o start.o
	$(LD) $(LDFLAGS) start.o forkTest.o -o forkTest.coff
	../bin/coff2noff forkTest.coff forkTest

execTest.o: execTest.c
	$(CC) $(CFLAGS) -c execTest.c
execTest: execTest.o start.o
	$(LD) $(LDFLAGS) start.o execTest.o -o execTest.coff
	../bin/coff2noff execTest.coff execTest

CVTest.o: CVTest.c
	$(CC) $(CFLAGS) -c CVTest.c
CVTest: CVTest.o start.o
	$(LD) $(LDFLAGS) start.o CVTest.o -o CVTest.coff
	../bin/coff2noff CVTest.coff CVTest

execCVTest.o: execCVTest.c
	$(CC) $(CFLAGS) -c execCVTest.c
execCVTest: execCVTest.o start.o
	$(LD) $(LDFLAGS) start.o execCVTest.o -o execCVTest.coff
	../bin/coff2noff execCVTest.coff execCVTest

multiExecTest.o: multiExecTest.c
	$(CC) $(CFLAGS) -c multiExecTest.c
multiExecTest: multiExecTest.o start.o
	$(LD) $(LDFLAGS) start.o multiExecTest.o -o multiExecTest.coff
	../bin/coff2noff multiExecTest.coff multiExecTest

destroyCVTest.o: destroyCVTest.c
	$(CC) $(CFLAGS) -c destroyCVTest.c
destroyCVTest: destroyCVTest.o start.o
	$(LD) $(LDFLAGS) start.o destroyCVTest.o -o destroyCVTest.coff
	../bin/coff2noff destroyCVTest.coff destroyCVTest

writeTest.o: writeTest.c
	$(CC) $(CFLAGS) -c writeTest.c
writeTest: writeTest.o start.o
	$(LD) $(LDFLAGS) start.o writeTest.o -o writeTest.coff
	../bin/coff2noff writeTest.coff writeTest

traceTest.o: traceTest.c
	$(CC) $(CFLAGS) -c traceTest.c
traceTest: traceTest.o start.o
	$(LD) $(LDFLAGS) start.o traceTest.o -o traceTest.coff
	../bin/coff2noff traceTest.coff traceTest

randTest.o: randTest.c
	$(CC) $(CFLAGS) -c randTest.c
randTest: randTest.o start.o
	$(LD) $(LDFLAGS) start.o randTest.o -o randTest.coff
	../bin/coff2noff randTest.coff randTest
	
PassportOffice.o: PassportOffice.c
	$(CC) $(CFLAGS) -c PassportOffice.c
PassportOffice: PassportOffice.o start.o
	$(LD) $(LDFLAGS) start.o PassportOffice.o -o PassportOffice.coff
	../bin/coff2noff PassportOffice.coff PassportOffice
	
ExecPassportOffice.o: ExecPassportOffice.c
	$(CC) $(CFLAGS) -c ExecPassportOffice.c
ExecPassportOffice: ExecPassportOffice.o start.o
	$(LD) $(LDFLAGS) start.o ExecPassportOffice.o -o ExecPassportOffice.coff
	../bin/coff2noff ExecPassportOffice.coff ExecPassportOffice
	
ExecTwoPassportOffice.o: ExecTwoPassportOffice.c
	$(CC) $(CFLAGS) -c ExecTwoPassportOffice.c
ExecTwoPassportOffice: ExecTwoPassportOffice.o start.o
	$(LD) $(LDFLAGS) start.o ExecTwoPassportOffice.o -o ExecTwoPassportOffice.coff
	../bin/coff2noff ExecTwoPassportOffice.coff ExecTwoPassportOffice

passportTest1.o: passportTest1.c
	$(CC) $(CFLAGS) -c passportTest1.c
passportTest1: passportTest1.o start.o
	$(LD) $(LDFLAGS) start.o passportTest1.o -o passportTest1.coff
	../bin/coff2noff passportTest1.coff passportTest1

passportTest2.o: passportTest2.c
	$(CC) $(CFLAGS) -c passportTest2.c
passportTest2: passportTest2.o start.o
	$(LD) $(LDFLAGS) start.o passportTest2.o -o passportTest2.coff
	../bin/coff2noff passportTest2.coff passportTest2

execPassportTests.o: execPassportTests.c
	$(CC) $(CFLAGS) -c execPassportTests.c
execPassportTests: execPassportTests.o start.o
	$(LD) $(LDFLAGS) start.o execPassportTests.o -o execPassportTests.coff
	../bin/coff2noff execPassportTests.coff execPassportTests

passportTest3.o: passportTest3.c
	$(CC) $(CFLAGS) -c passportTest3.c
passportTest3: passportTest3.o start.o
	$(LD) $(LDFLAGS) start.o passportTest3.o -o passportTest3.coff
	../bin/coff2noff passportTest3.coff passportTest3

execPassportTests2.o: execPassportTests2.c
	$(CC) $(CFLAGS) -c execPassportTests2.c
execPassportTests2: execPassportTests2.o start.o
	$(LD) $(LDFLAGS) start.o execPassportTests2.o -o execPassportTests2.coff
	../bin/coff2noff execPassportTests2.coff execPassportTests2

Client.o: Client.c
	$(CC) $(CFLAGS) -c Client.c
Client: Client.o start.o
	$(LD) $(LDFLAGS) start.o Client.o -o Client.coff
	../bin/coff2noff Client.coff Client

Client1A.o: Client1A.c
	$(CC) $(CFLAGS) -c Client1A.c
Client1A: Client1A.o start.o
	$(LD) $(LDFLAGS) start.o Client1A.o -o Client1A.coff
	../bin/coff2noff Client1A.coff Client1A

Client1B.o: Client1B.c
	$(CC) $(CFLAGS) -c Client1B.c
Client1B: Client1B.o start.o
	$(LD) $(LDFLAGS) start.o Client1B.o -o Client1B.coff
	../bin/coff2noff Client1B.coff Client1B
	
Client2A.o: Client2A.c
	$(CC) $(CFLAGS) -c Client2A.c
Client2A: Client2A.o start.o
	$(LD) $(LDFLAGS) start.o Client2A.o -o Client2A.coff
	../bin/coff2noff Client2A.coff Client2A

Client2B.o: Client2B.c
	$(CC) $(CFLAGS) -c Client2B.c
Client2B: Client2B.o start.o
	$(LD) $(LDFLAGS) start.o Client2B.o -o Client2B.coff
	../bin/coff2noff Client2B.coff Client2B

Client2C.o: Client2C.c
	$(CC) $(CFLAGS) -c Client2C.c
Client2C: Client2C.o start.o
	$(LD) $(LDFLAGS) start.o Client2C.o -o Client2C.coff
	../bin/coff2noff Client2C.coff Client2C

ClientBad.o: ClientBad.c
	$(CC) $(CFLAGS) -c ClientBad.c
ClientBad: ClientBad.o start.o
	$(LD) $(LDFLAGS) start.o ClientBad.o -o ClientBad.coff
	../bin/coff2noff ClientBad.coff ClientBad

AppClerk.o: AppClerk.c
	$(CC) $(CFLAGS) -c AppClerk.c
AppClerk: AppClerk.o start.o
	$(LD) $(LDFLAGS) start.o AppClerk.o -o AppClerk.coff
	../bin/coff2noff AppClerk.coff AppClerk

PicClerk.o: PicClerk.c
	$(CC) $(CFLAGS) -c PicClerk.c
PicClerk: PicClerk.o start.o
	$(LD) $(LDFLAGS) start.o PicClerk.o -o PicClerk.coff
	../bin/coff2noff PicClerk.coff PicClerk

PassClerk.o: PassClerk.c
	$(CC) $(CFLAGS) -c PassClerk.c
PassClerk: PassClerk.o start.o
	$(LD) $(LDFLAGS) start.o PassClerk.o -o PassClerk.coff
	../bin/coff2noff PassClerk.coff PassClerk

CashClerk.o: CashClerk.c
	$(CC) $(CFLAGS) -c CashClerk.c
CashClerk: CashClerk.o start.o
	$(LD) $(LDFLAGS) start.o CashClerk.o -o CashClerk.coff
	../bin/coff2noff CashClerk.coff CashClerk

clean:
	rm -f *.o *.coff
