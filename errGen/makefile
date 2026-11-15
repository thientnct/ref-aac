# Makefile for MPEG4 test procedure error generator

CC = gcc

ifndef OPTIM
OPTIM = 0
endif

ifeq "$(OPTIM)" "1"
CFLAGS = -O3
else
CFLAGS = -g -Wall
endif

all: erand gilbert packetloss

clean:
	rm -f *.o erand gilbert packetloss

erand: erand.o rand.o
	$(CC) -o erand erand.o rand.o $(CFLAGS)

gilbert: gilbert.o rand.o
	$(CC) -o gilbert gilbert.o rand.o $(CFLAGS)

packetloss: packetloss.o rand.o
	$(CC) -o packetloss packetloss.o rand.o $(CFLAGS)
