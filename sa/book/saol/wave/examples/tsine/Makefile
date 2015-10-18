
#   Sfront, a SAOL to C translator
#   Copyright (C) 1998 John Lazzaro
#   Maintainers's address: lazzaro@cs.berkeley.edu;
#
#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation (Version 1, 1989).
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; see the file COPYING.  If not, write to
#the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

FILENAME = tsine

SAOLFILE = $(FILENAME).saol
SASLFILE = $(FILENAME).sasl
MP4FILE  = $(FILENAME).mp4

INFILE = 
OUTFILE = output.wav

CC = gcc
OPT = -O2
CFLAGS = $(OPT)
SFRONT = sfront
WAVEPLAYER   = play

OUTMODE = -aout $(OUTFILE)
INMODE = 

## for INFILE/OUTFILE selections that need libraries

IOLINK = 

## for INFILE/OUTFILE std selection:  > foo < bar

REDIRECT = 

##
## makes a raw 16-bit signed integer audio file
##
 
$(OUTFILE): $(SAOLFILE) $(SASLFILE)
	$(SFRONT) $(OUTMODE) $(INMODE) -orc $(SAOLFILE) -sco $(SASLFILE)
	$(CC) $(CFLAGS) sa.c -lm $(IOLINK) -o sa
	./sa $(REDIRECT)

## tests bitstream creation code, by creating an MP4 file, decoding
## it, and comparing audio out with audio created by original ASCII
## files. doesn't work with stdin/stdout.

mp4test	: $(OUTFILE)
	rm -rf $(MP4FILE) sa.c
	mv $(OUTFILE) safe
	$(SFRONT) -symtab -orc $(SAOLFILE) -sco $(SASLFILE) -bitout $(MP4FILE)
	$(SFRONT) $(OUTMODE) $(INMODE) -bit $(MP4FILE)
	$(CC) $(CFLAGS) sa.c -lm $(IOLINK) -o sa
	./sa $(REDIRECT)
	cmp $(OUTFILE) safe

## tests a new sfront against an old one. assume safe is the audio
## created by old sfront. 

compare	: $(OUTFILE)
	cmp $(OUTFILE) safe

safe	: $(OUTFILE)
	cp $(OUTFILE) safe

## for performance testing
##

timing	: 
	./sa $(REDIRECT)

##
## to listen to audio output
##

play  : $(OUTFILE)
	$(WAVEPLAYER) $(OUTFILE)

clean: 
	rm -rf sa.c sa audio a*.wav $(MP4FILE) $(OUTFILE) *.info *~ safe



