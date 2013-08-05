# COPYRIGHT_BEGIN
#  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
#  
#  Copyright (C) 2008-2013, Cable Television Laboratories, Inc. 
#  
#  This software is available under multiple licenses: 
#  
#  (1) BSD 2-clause 
#   Redistribution and use in source and binary forms, with or without modification, are
#   permitted provided that the following conditions are met:
#        ·Redistributions of source code must retain the above copyright notice, this list 
#             of conditions and the following disclaimer.
#        ·Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
#             and the following disclaimer in the documentation and/or other materials provided with the 
#             distribution.
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
#   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
#   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
#   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
#   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  
#  (2) GPL Version 2
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, version 2. This program is distributed
#   in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
#   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#  
#   You should have received a copy of the GNU General Public License along
#   with this program.If not, see<http:www.gnu.org/licenses/>.
#  
#  (3)CableLabs License
#   If you or the company you represent has a separate agreement with CableLabs
#   concerning the use of this code, your rights and obligations with respect
#   to this code shall be as set forth therein. No license is granted hereunder
#   for any other purpose.
#  
#   Please contact CableLabs if you need additional information or 
#   have any questions.
#  
#       CableLabs
#       858 Coal Creek Cir
#       Louisville, CO 80027-9750
#       303 661-9100
# COPYRIGHT_END
#############################################################################

default: build

include ../common.mk

.PHONY: build clean purge


#
# Directory locations
#
LIB_OBJDIR = ./ifslibobjs
NDX_OBJDIR = ./ifsndxobjs
SIM_OBJDIR = ./ifssimobjs

INCLUDE_DIR = ./include
BIN_DIR     = ../bin
LIB_DIR     = ../lib

#
# Compile options
#
LIB_COPTS = \
	-I/usr/include \
	-I/usr/include/glib-2.0 \
	-I/usr/lib/$(hardware_platform)/glib-2.0/include \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS)

NDX_COPTS = \
	-I../remap \
	-I/usr/include \
	-I/usr/include/glib-2.0 \
	-I/usr/lib/$(hardware_platform)/glib-2.0/include \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS)

SIM_COPTS = \
	-I../remap \
	-I/usr/include \
	-I/usr/include/glib-2.0 \
	-I/usr/lib/$(hardware_platform)/glib-2.0/include \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS)

#
# Linker options
#
LIB_LDOPTS = \
	-L../bin \
	-L../lib \
	-lglib-2.0 \

NDX_LDOPTS = \
	-L../bin \
	-L../lib \
	-lglib-2.0 \
	-lremap

SIM_LDOPTS = \
	-L../bin \
	-L../lib \
	-lglib-2.0 \
	-lremap

#
# Source files
#
LIB_SOURCES = \
	ifs_mpeg2ts_parse.c \
	ifs_mpeg2ps_parse.c \
	ifs_mpeg4_parse.c \
	ifs_h262_parse.c \
	ifs_h264_parse.c \
	ifs_h265_parse.c \
	ifs_file.c \
	ifs_operations.c \
	ifs_utils.c \
	ifs_impl.c

NDX_SOURCES = \
	ifs_mpeg2ts_parse.c \
	ifs_mpeg2ps_parse.c \
	ifs_mpeg4_parse.c \
	ifs_h262_parse.c \
	ifs_h264_parse.c \
	ifs_h265_parse.c \
	ifs_impl.c \
	ifs_file.c \
	ifs_operations.c \
	ifs_utils.c \
	NdxDump.c

SIM_SOURCES = \
	ifs_mpeg2ts_parse.c \
	ifs_mpeg2ps_parse.c \
	ifs_mpeg4_parse.c \
	ifs_h262_parse.c \
	ifs_h264_parse.c \
	ifs_h265_parse.c \
	ifs_impl.c \
	ifs_file.c \
	ifs_operations.c \
	ifs_utils.c \
	IfsSim.c

#
# Intermediate files
#
LIB_OBJS = $(patsubst %.c,$(LIB_OBJDIR)/%.o,$(LIB_SOURCES))
NDX_OBJS = $(patsubst %.c,$(NDX_OBJDIR)/%.o,$(NDX_SOURCES))
SIM_OBJS = $(patsubst %.c,$(SIM_OBJDIR)/%.o,$(SIM_SOURCES))

#
# Include dependency files
#
LIB_DEPENDS = $(patsubst %.c,$(LIB_OBJDIR)/%.d,$(LIB_SOURCES))
NDX_DEPENDS = $(patsubst %.c,$(NDX_OBJDIR)/%.d,$(NDX_SOURCES))
SIM_DEPENDS = $(patsubst %.c,$(SIM_OBJDIR)/%.d,$(SIM_SOURCES))

ifeq ($(strip $(filter clean purge,$(MAKECMDGOALS))),)
-include $(LIB_DEPENDS)
-include $(NDX_DEPENDS)
-include $(SIM_DEPENDS)
endif

#
# Things built by this makefile
#
LIB = $(LIB_DIR)/libifs$(SO_SUFFIX)
NDX = $(BIN_DIR)/NdxDump$(EXE_SUFFIX)
SIM = $(BIN_DIR)/IfsSim$(EXE_SUFFIX)

platform:
	@$(ECHO) "building for $(PLATFORM)"

#
# Build everything
#
build: $(LIB) $(NDX) $(SIM)

#
# Build the library from intermediate files
#
$(LIB): $(LIB_OBJS)
	$(call BUILD_SHARED_LIBRARY,$(LIB_OBJS) $(LIB_LDOPTS))

#
# Build the NdxDump tool
#
$(NDX): $(NDX_OBJS)
	$(call BUILD_EXECUTABLE,$(NDX_OBJS) $(NDX_LDOPTS))

#
# Build the IfsSim tool
#
$(SIM): $(SIM_OBJS)
	$(call BUILD_EXECUTABLE,$(SIM_OBJS) $(SIM_LDOPTS))

#
# Compile source files into intermediate files
#
$(LIB_OBJDIR)/%.o: %.c $(call makefile-list)
	$(call COMPILE,$(LIB_COPTS))

$(NDX_OBJDIR)/%.o: %.c $(call makefile-list)
	$(call COMPILE,$(NDX_COPTS))

$(SIM_OBJDIR)/%.o: %.c $(call makefile-list)
	$(call COMPILE,$(SIM_COPTS))

#
# Bring header file dependencies up to date
#
$(LIB_OBJDIR)/%.d: %.c
	$(call BUILD_DEPENDS,$(LIB_COPTS))

$(NDX_OBJDIR)/%.d: %.c
	$(call BUILD_DEPENDS,$(NDX_COPTS))

$(SIM_OBJDIR)/%.d: %.c
	$(call BUILD_DEPENDS,$(SIM_COPTS))

#
# Clean and purge
#
clean:
	$(RMTREE) $(LIB_OBJDIR)
	$(RMTREE) $(NDX_OBJDIR)
	$(RMTREE) $(SIM_OBJDIR)

purge:
	$(RM) $(LIB) $(NDX) $(SIM)

