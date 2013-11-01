CFLAGS=-O2 -g -Wall

LIBS=-lpthread

OBJS=gpio.o dom.o gpsPosition.o rtty.o

all: dom16 dom16_2

dom16: gpio.o dom16.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@ $^

dom16_2: gpio.o dom16_2.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@ $^

%.o: %.c %.h
	$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm *.o dom16 dom16_2
