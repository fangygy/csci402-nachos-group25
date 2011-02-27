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

all: halt matmult sort testfiles yieldTest lockTest1 forkTest execTest CVTest1 execCVTest

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

lockTest1.o: lockTest1.c
	$(CC) $(CFLAGS) -c lockTest1.c
lockTest1: lockTest1.o start.o
	$(LD) $(LDFLAGS) start.o lockTest1.o -o lockTest1.coff
	../bin/coff2noff lockTest1.coff lockTest1

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

CVTest1.o: CVTest1.c
	$(CC) $(CFLAGS) -c CVTest1.c
CVTest1: CVTest1.o start.o
	$(LD) $(LDFLAGS) start.o CVTest1.o -o CVTest1.coff
	../bin/coff2noff CVTest1.coff CVTest1

execCVTest.o: execCVTest.c
	$(CC) $(CFLAGS) -c execCVTest.c
execCVTest: execCVTest.o start.o
	$(LD) $(LDFLAGS) start.o execCVTest.o -o execCVTest.coff
	../bin/coff2noff execCVTest.coff execCVTest

clean:
	rm -f *.o *.coff
