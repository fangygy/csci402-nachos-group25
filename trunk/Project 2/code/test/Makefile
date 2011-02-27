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

all: halt matmult sort testfiles yieldTest lockTest forkTest execTest CVTest execCVTest destroyCVTest execDestroyTest writeTest PassportOffice ExecPassportOffice

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

destroyCVTest.o: destroyCVTest.c
	$(CC) $(CFLAGS) -c destroyCVTest.c
destroyCVTest: destroyCVTest.o start.o
	$(LD) $(LDFLAGS) start.o destroyCVTest.o -o destroyCVTest.coff
	../bin/coff2noff destroyCVTest.coff destroyCVTest

execDestroyTest.o: execDestroyTest.c
	$(CC) $(CFLAGS) -c execDestroyTest.c
execDestroyTest: execDestroyTest.o start.o
	$(LD) $(LDFLAGS) start.o execDestroyTest.o -o execDestroyTest.coff
	../bin/coff2noff execDestroyTest.coff execDestroyTest

writeTest.o: writeTest.c
	$(CC) $(CFLAGS) -c writeTest.c
writeTest: writeTest.o start.o
	$(LD) $(LDFLAGS) start.o writeTest.o -o writeTest.coff
	../bin/coff2noff writeTest.coff writeTest
	
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

clean:
	rm -f *.o *.coff
