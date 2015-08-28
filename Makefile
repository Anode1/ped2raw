#
# Copyright (C) 2001 Vasili Gavrilov
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

SHELL=/bin/sh

BIN=ped2raw

OS=$(shell uname)
ifeq ($(findstring CYGWIN,$(OS)),CYGWIN)
	OS=Cygwin
endif
ifeq ($(findstring WINNT,$(OS)),WINNT)
	OS=Cygwin
endif

ifeq ($(OS), Cygwin)
	NOCYGWIN= -mno-cygwin
	#fix for recently introduced problem 
	SHELL=c:/cygwin/bin/bash.exe
endif

#consider all *.c as sources  
SOURCES.c := $(wildcard *.c)


CFLAGS= $(NOCYGWIN) -ansi -W -Wall
CC=gcc
OBJS=$(SOURCES.c:.c=.o)
LINK=gcc $(CFLAGS)
LFLAGS=-lm $()

debug : CFLAGS = $(NOCYGWIN) -ansi -W -Wall -g -Wundef
pedantic : CFLAGS = $(NOCYGWIN) -ansi -W -Wall -g -Wundef -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
release : CFLAGS = $(NOCYGWIN) -ansi -W -Wall -O2

.SUFFIXES:
.SUFFIXES: .d .o .h .c
.c.o: ; $(CC) $(CFLAGS) -MMD -c $*.c 

.PHONY: clean

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

all release debug pedantic ut ut1: $(BIN)

$(BIN): $(OBJS)
	$(LINK) $(FLAGS) -o $(BIN) $(OBJS) $(LFLAGS)

clean:
	rm -f $(BIN) $(OBJS) *.d *~


include $(sources:.c=.d)
