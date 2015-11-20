
#
# Makefile for sjgopencl plug-in for GIMP
#
# $Id: Makefile,v 1.2 2014/11/20 00:39:53 sjg Exp $
#
# (c) Stephen Geary, Oct 2014
#

RM=rm

CC=gcc

CFLAGS=-Wall $(shell gimptool-2.0 --cflags)
#CFLAGS=-Wno-deprecated -Wno-deprecated-declarations $(shell gimptool-2.0 --cflags)

LDFLAGS=$(shell gimptool-2.0 --libs) -lOpenCL -lrt -lm

SOURCES=sjgopencl.c simpleCL.c debugme.c sjgutils.c parser.c expr.c

OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=sjgopencl


install: all
	gimptool-2.0 --install-bin $(EXECUTABLE)

debug:
	$(CC) -g -DDEBUGME $(CFLAGS) $(SOURCES) -o $(EXECUTABLE)  $(LDFLAGS)
	gimptool-2.0 --install-bin $(EXECUTABLE)

all:
	$(CC) -O2 $(CFLAGS) $(SOURCES) -o $(EXECUTABLE)  $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@



