CC      = gcc
CFLAGS  = -lm
CFLAGS_DBG  = $(CFLAGS) -g
CFLAGS_OPTI  = $(CFLAGS) -O3
RM      = rm -f

default: all

all: opti


# DEBUG BINS

moremath_d.o: moremath.c moremath.h
	$(CC) $(CFLAGS_DBG) -c moremath.c moremath.h

raytracing_d.o: raytracing.c raytracing.h
	$(CC) $(CFLAGS_DBG) -c raytracing.c raytracing.h

main_d.o: main.c
	$(CC) $(CFLAGS_DBG) -c main.c


# OPTI BINS

moremath_o.o: moremath.c moremath.h
	$(CC) $(CFLAGS_OPTI) -c moremath.c moremath.h

raytracing_o.o: raytracing.c raytracing.h
	$(CC) $(CFLAGS_OPTI) -c raytracing.c raytracing.h

main_o.o: main.c
	$(CC) $(CFLAGS_OPTI) -c main.c


# BUILD GENERAL BIN

debug: moremath_d.o raytracing_d.o main_d.o
	$(CC) $(CFLAGS_DBG) moremath.o raytracing.o main.o -o rayt.out

opti: moremath_o.o raytracing_o.o main_o.o
	$(CC) $(CFLAGS_OPTI) moremath.o raytracing.o main.o -o rayt.out


# CLEAN

clean:
	$(RM) *.o

veryclean:
	$(RM) *.o *.out