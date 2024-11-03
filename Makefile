CFLAGS?=-O2 -g -Wall -W 
CFLAGS+= -I./aisdecoder -I./aisdecoder/lib -I./tcp_listener
LDFLAGS+=-lpthread -lm

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
	CFLAGS += $(shell pkg-config --cflags librtlsdr libusb-1.0)
	LDFLAGS +=$(shell pkg-config --libs librtlsdr libusb-1.0)
endif

CC?=gcc
SOURCES= \
	main.c rtl_ais.c convenience.c \
	./aisdecoder/aisdecoder.c \
	./aisdecoder/sounddecoder.c \
	./aisdecoder/lib/receiver.c \
	./aisdecoder/lib/protodec.c \
	./aisdecoder/lib/hmalloc.c \
	./aisdecoder/lib/filter.c \
	./tcp_listener/tcp_listener.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=rtl_ais

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) -std=gnu89

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

install:
	install -d -m 755 $(DESTDIR)/$(PREFIX)/bin
	install -m 755 $(EXECUTABLE) "$(DESTDIR)/$(PREFIX)/bin/"

