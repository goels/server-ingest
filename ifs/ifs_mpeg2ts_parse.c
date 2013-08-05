
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
//        Â·Redistributions of source code must retain the above copyright notice, this list 
//             of conditions and the following disclaimer.
//        Â·Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#define _IFS_MPEG2TS_PARSE_C "$Rev: 141 $"

#include <string.h>

#include "ifs_impl.h"
#include "ifs_h262_parse.h"
#include "ifs_h264_parse.h"
#include "ifs_h265_parse.h"
#include "ifs_mpeg2ts_parse.h"
#include "ifs_utils.h"

IfsPid mpeg2ts_GetPid(IfsPacket * pIfsPacket)
{
    return (((IfsPid)(pIfsPacket->bytes[1] & 0x1F)) << 8)
            | pIfsPacket->bytes[2];
}

void mpeg2ts_ParseAdaptation(IfsHandle ifsHandle, unsigned char bytes[7])
{
    unsigned char espBit = (bytes[0] >> 5) & 0x01;
    unsigned char what = bytes[0] & ~(1 << 5);

    if (ifsHandle->codec->h262->oldEsp == IFS_UNDEFINED_BYTE)
    {
        ifsHandle->codec->h262->oldEsp = espBit;
    }
    else if (ifsHandle->codec->h262->oldEsp != espBit)
    {
        what |= (1 << 5);
        ifsHandle->codec->h262->oldEsp = espBit;
    }

    ifsHandle->entry.what |= what;

    //  if (PCR_flag == '1') {
    //      program_clock_reference_base     33 uimsbf
    //      reserved                          6 bslbf
    //      program_clock_reference_extension 9 uimsbf
    //
    //  PCR(i) = PCR_base(i) × 300 + PCR_ext(i)

    if (what & IfsIndexAdaptPcreBit)
    {
        ifsHandle->codec->h262->ifsPcr = (((IfsPcr) bytes[1]) << 25); // 33-25
        ifsHandle->codec->h262->ifsPcr |= (((IfsPcr) bytes[2]) << 17); // 24-17
        ifsHandle->codec->h262->ifsPcr |= (((IfsPcr) bytes[3]) << 9); // 16- 9
        ifsHandle->codec->h262->ifsPcr |= (((IfsPcr) bytes[4]) << 1); //  8- 1
        ifsHandle->codec->h262->ifsPcr |= (((IfsPcr) bytes[5]) >> 7); //     0

        ifsHandle->codec->h262->ifsPcr = (ifsHandle->codec->h262->ifsPcr * 300 + (((((IfsPcr) bytes[5])
                & 1) << 8) | bytes[6]));
    }
}

IfsBoolean mpeg2ts_ParsePacket(IfsHandle ifsHandle, IfsPacket * pIfsPacket)
{
    if (pIfsPacket->bytes[0] == 0x47)
    {
        ifsHandle->entry.what = 0;

        switch (ifsHandle->codecType)
        {
            case IfsCodecTypeH261:
            case IfsCodecTypeH262:
            case IfsCodecTypeH263:
                if (mpeg2ts_GetPid(pIfsPacket) ==
                    ifsHandle->codec->h262->videoPid)
                {
                    h262_ParsePacket(ifsHandle, pIfsPacket);
                }
                break;

            case IfsCodecTypeH264:
                h264_ParsePacket(ifsHandle, pIfsPacket);
                break;

            case IfsCodecTypeH265:
                h265_ParsePacket(ifsHandle, pIfsPacket);
                break;

            default:
                printf("IfsReturnCodeBadInputParameter: "
                       "invalid CODEC line %d of %s\n", __LINE__, __FILE__);
                break;
        }

    }
    else
    {
        ifsHandle->entry.what = IfsIndexHeaderBadSync;

        // Abandon this entire packet and Reset the PES state machine

        ifsHandle->ifsState = IfsStateInitial;
    }

    return ifsHandle->entry.what;
}

