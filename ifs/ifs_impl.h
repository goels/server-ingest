// COPYRIGHT_BEGIN
//  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
//  
//  Copyright (C) 2008-2013, Cable Television Laboratories, Inc. 
//  
//  This software is available under multiple licenses: 
//  
//  (1) BSD 2-clause 
//   Redistribution and use in source and binary forms, with or without modification, are
//   permitted provided that the following conditions are met:
//        ·Redistributions of source code must retain the above copyright notice, this list 
//             of conditions and the following disclaimer.
//        ·Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
//             and the following disclaimer in the documentation and/or other materials provided with the 
//             distribution.
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
//   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
//   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
//   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
//   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
//   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  
//  (2) GPL Version 2
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, version 2. This program is distributed
//   in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
//   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//   PURPOSE. See the GNU General Public License for more details.
//  
//   You should have received a copy of the GNU General Public License along
//   with this program.If not, see<http:www.gnu.org/licenses/>.
//  
//  (3)CableLabs License
//   If you or the company you represent has a separate agreement with CableLabs
//   concerning the use of this code, your rights and obligations with respect
//   to this code shall be as set forth therein. No license is granted hereunder
//   for any other purpose.
//  
//   Please contact CableLabs if you need additional information or 
//   have any questions.
//  
//       CableLabs
//       858 Coal Creek Cir
//       Louisville, CO 80027-9750
//       303 661-9100
// COPYRIGHT_END

// Private IFS Definitions

#ifndef _IFS_IMPL_H
#define _IFS_IMPL_H "$Rev: 141 $"

#include <glib.h>
#include <stdio.h>
#include "IfsIntf.h"
#include "ifs_mpeg2_impl.h"

typedef unsigned long FileNumber;

typedef enum
{

    IfsIndexDumpModeOff = 0, IfsIndexDumpModeDef = 1, IfsIndexDumpModeAll = 2,

} IfsIndexDumpMode;

typedef enum
{
    IfsReadTypePrevious = -1, IfsReadTypeNearest = 0, IfsReadTypeNext = +1,

} IfsReadType;

typedef struct IfsIndexEntry
{
    IfsClock when; // nanoseconds
    IfsIndex what;
    NumPackets realWhere;
    NumPackets virtWhere;

#ifndef DEBUG_ALL_PES_CODES
    unsigned long pad;
#endif

} IfsIndexEntry;

typedef struct IfsHandleImpl
{
    // The input parameters
    char * path;
    char * name;

    // Current logical file information
    NumBytes mpegSize; // current total number of MPEG bytes
    NumBytes ndexSize; // current total number of NDEX bytes
    char * both; // path + name
    char * mpeg; // path + name + filename.mpg
    char * ndex; // path + name + filename.ndx
    FileNumber begFileNumber; // lowest file name/number
    FileNumber endFileNumber; // highest file name/number
    FILE * pMpeg; // current MPEG file
    FILE * pNdex; // current NDEX file
    NumPackets realLoc; // current real location in current file, offset in packets
    NumPackets virtLoc; // current virtual location in current file, offset in packets
    IfsBoolean isReading; // true if this a reader, false if it is a writer
    IfsClock begClock; // clock at beg of recording, in nanoseconds
    IfsClock endClock; // clock at end of recording, in nanoseconds
    IfsClock nxtClock; // clock at next file change, in nanoseconds

    // Indexer settings and state
    IfsIndexEntry entry;
#ifdef DEBUG_ALL_PES_CODES
    IfsIndex extWhat;
    IfsBoolean isProgSeq;
    long pad1;
#endif

    // Video codec-specific handle params
    IfsCodec* codec;

    // Video parsing state
    IfsState ifsState;

    // Seek state, all real, no virtual numbers
    NumPackets maxPacket;
    FileNumber curFileNumber;
    NumEntries entryNum;
    NumEntries maxEntry;

    // Append state, all real, no virtual numbers
    FileNumber appendFileNumber;
    NumPackets appendPacketNum;
    NumEntries appendEntryNum;
    NumPackets appendIndexShift;
    NumPackets appendPrevFiles;
    long pad2;

    IfsTime maxSize; // in seconds, 0 = value not used

    int numEmptyFreads;  // the number of times we performed an fread and got 0

    GStaticMutex mutex;  // TSB thread versus IFS thread protection

} IfsHandleImpl;

#endif
