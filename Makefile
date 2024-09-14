CC      = gcc
CFLAGS  = -lm
CFLAGS_DBG  = $(CFLAGS) -g
CFLAGS_OPTI  = $(CFLAGS) -O3
RM      = rm -f


default: all

all: opti

debug: main.c
	$(CC) $(CFLAGS_DBG) -o rayt.o main.c

opti: main.c
	$(CC) $(CFLAGS_OPTI) -o rayt.o main.c

clean veryclean:
	$(RM) rayt.o