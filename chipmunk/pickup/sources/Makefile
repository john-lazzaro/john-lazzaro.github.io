
# System Makefile for the Chipmunk Tools
#
#   Copyright (C) 1983, 1990, 1993 California Institute of Technology.
#   Authors: Dave Gillespie, Marwan Jabri
#   Maintainers: John Lazzaro and Dave Gillespie
#   Maintainers's address: lazzaro@cs.berkeley.edu;
#                          EECS/571 Evans Hall/UCB/Berkeley CA 94720
#                          daveg@synaptics.com;
#                          Send questions, bug fixes, to these addresses.
#
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
#the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

# This makefile allows you to make and install the entire
# chipmunk toolset in one command. It assume you have all of the
# tools in the distribution; if not, remove the tools you
# do not have from the line below.

PACKAGES = psys/src log/src util netcmp mosis wol wolcomp until/V1.2 view
 
makeonly: lib bin
	for i in $(PACKAGES) ;\
	do (cd $$i ; echo "Making in $$i..."; make $(MFLAGS)); done
 
World: lib bin clean install

lib:
	-mkdir lib

bin:
	-mkdir bin

clean:
	for i in $(PACKAGES) ;\
	do (if (cd $$i) then (cd $$i; echo "Making clean in $$i..."; \
	make clean) fi); done

install:
	for i in $(PACKAGES) ;\
	do (if (cd $$i) then (cd $$i; echo "Making install in $$i..."; \
	make install) fi); done

create:
	for i in $(PACKAGES) ;\
	do (if (cd $$i) then (cd $$i; echo "Making create in $$i..."; \
	make create) fi); done

 
