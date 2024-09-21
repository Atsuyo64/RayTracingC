CC      = gcc
CFLAGS  = -lm
CFLAGS_DBG  = $(CFLAGS) -g
CFLAGS_OPTI  = $(CFLAGS) -O3
RM      = rm -f

default: all

all: opti


# DEBUG BINS

objloader_d.o: objloader.c objloader.h
	$(CC) $(CFLAGS_DBG) -c objloader.c

moremath_d.o: moremath.c moremath.h scene.h objloader.h
	$(CC) $(CFLAGS_DBG) -c moremath.c

raytracing_d.o: raytracing.c raytracing.h scene.h
	$(CC) $(CFLAGS_DBG) -c raytracing.c

main_d.o: main.c scene.h
	$(CC) $(CFLAGS_DBG) -c main.c


# OPTI BINS

objloader.o: objloader.c objloader.h
	$(CC) $(CFLAGS_OPTI) -c objloader.c

moremath.o: moremath.c moremath.h scene.h
	$(CC) $(CFLAGS_OPTI) -c moremath.c

raytracing.o: raytracing.c raytracing.h scene.h
	$(CC) $(CFLAGS_OPTI) -c raytracing.c

main.o: main.c scene.h
	$(CC) $(CFLAGS_OPTI) -c main.c


# BUILD GENERAL BIN

debug: moremath_d.o raytracing_d.o main_d.o objloader_d.o
	$(CC) $(CFLAGS_DBG) moremath.o raytracing.o main.o objloader.o -o rayt.out

opti: moremath.o raytracing.o main.o objloader.o
	$(CC) $(CFLAGS_OPTI) moremath.o raytracing.o main.o objloader.o -o rayt.out


# OBJ Loader & Tester

obj: objloader.c objtest.c
	$(CC) $(CFLAGS_OPTI) objloader.c objtest.c -o obj.out

# CLEAN

clean:
	$(RM) *.o

veryclean:
	$(RM) *.o *.out
