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

#define _IFS_H265_PARSE_C "$Rev: 141 $"

#include <string.h>

#include "ifs_h265_impl.h"
#include "ifs_h265_parse.h"
#include "ifs_utils.h"

extern IfsH265Index indexerSetting;

IfsBoolean h265_ParsePacket(IfsHandle ifsHandle, IfsPacket * pIfsPacket)
{
    ifsHandle->ifsState = IfsStateInitial;

    //return ifsHandle->entry.what & indexerSetting; // any indexed events in this packet?
    return ( ((unsigned long)(ifsHandle->entry.what>>32) & (unsigned long)(indexerSetting>>32))
            || ((unsigned long)ifsHandle->entry.what & indexerSetting) );
}

char * h265_ParseWhat(IfsHandle ifsHandle, char * temp,
        const IfsIndexDumpMode ifsIndexDumpMode, const IfsBoolean flags)
{
    //IfsH265Index ifsIndex = ifsHandle->entry.what;

    temp[0] = 0;

    if (ifsIndexDumpMode == IfsIndexDumpModeAll)
    {
        // TODO:
    }

    return temp;
}

static unsigned long indexCounts[64] = { 0 };

void h265_CountIndexes(ullong index)
{
    IfsH265Index ifsIndex = (IfsH265Index)index;
    int i;

    for (i = 0; i < 64; i++)
    {
        IfsH265Index mask = ((IfsH265Index) 1) << i;

        // TODO: change to codec-specific counts here...
        if (0)
        {
            // Do nothing
        }
        else if (mask & ifsIndex)
            indexCounts[i]++;
    }
}

void h265_DumpIndexes(void)
{
    int i;

    printf("Occurances  Event\n");
    printf("----------  -----\n");

    for (i = 0; i < 64; i++)
    {
        char temp[256]; // ParseWhat
        IfsH265CodecImpl localH265Codec = { 0 };
        IfsHandleImpl tempHandleImpl;
        NumBytes pktSize = IFS_TRANSPORT_PACKET_SIZE;

        tempHandleImpl.codec = (IfsCodec*)&localH265Codec;
        g_static_mutex_init(&(tempHandleImpl.mutex));
        tempHandleImpl.entry.what = ((IfsH265Index) 1) << i;

        // TODO: set the correct container!
        if (IfsSetContainer(&tempHandleImpl, IfsContainerTypeMpeg2Ts, pktSize)
                != IfsReturnCodeNoErrorReported)
        {
            printf("Problems setting ifs codec\n");
            return;
        }

        if (IfsSetCodec(&tempHandleImpl, IfsCodecTypeH265)
                != IfsReturnCodeNoErrorReported)
        {
            printf("Problems setting ifs codec\n");
            return;
        }

        // TODO: change to codec-specific dump here...
        if (0)
        {
            // Do nothing
        }
        else if (indexCounts[i])
            printf("%10ld%s\n", indexCounts[i], tempHandleImpl.codec->h265->ParseWhat(&tempHandleImpl,
                    temp, IfsIndexDumpModeDef, IfsFalse));
    }
}

void h265_DumpHandle(const IfsHandle ifsHandle)
{
    printf("DUMP ifsHandle->codec->h265\n");
    g_static_mutex_lock(&(ifsHandle->mutex));
    printf("ifsHandle->ifsState         %d\n", ifsHandle->ifsState); // IfsState
    g_static_mutex_unlock(&(ifsHandle->mutex));
}

