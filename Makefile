CFLAGS=-O2 -g -Wall
CC=g++

#LDFLAGS=-L/usr/local/lib -R/usr/local/lib
LIBS=-lpthread

OBJS=crc.o dominoex.o gpio.o gps.o gpsPosition.o rtty.o sandals.o
TARGETS=sandals

all: $(TARGETS)

sandals: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@ $^

%.o: %.cxx %.h  Makefile
	$(CXX) -c $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm *.o $(TARGETS)
