
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
#include "ifs_mpeg2ps_parse.h"
#include "ifs_utils.h"


IfsBoolean mpeg2ps_ParsePacket(IfsHandle ifsHandle, IfsPacket * pIfsPacket)
{
    ifsHandle->entry.what = 0;

    switch (ifsHandle->codecType)
    {
        case IfsCodecTypeH261:
        case IfsCodecTypeH262:
        case IfsCodecTypeH263:
            return h262_ParsePacket(ifsHandle, pIfsPacket);

        case IfsCodecTypeH264:
        case IfsCodecTypeH265:
        default:
            printf("IfsReturnCodeBadInputParameter: "
                   "invalid CODEC line %d of %s\n", __LINE__, __FILE__);
            break;
    }

    return ifsHandle->entry.what;
}

// Extract system clock reference from program stream pack header
void extract_SCR(IfsHandle ifsHandle, unsigned char buf[6])
{
    int SCR, SCR1;
    double scr_val;
    //double scr_ext;

    /* System Clock reference
   '01'                                     2 bslbf
    system_clock_reference_base [32..30]    3 bslbf
    marker_bit                              1 bslbf
    system_clock_reference_base [29..15]    15 bslbf
    marker_bit                              1 bslbf
    system_clock_reference_base [14..0]     15 bslbf
    marker_bit                              1 bslbf
    system_clock_reference_extension        9 uimsbf
    marker_bit                              1 bslbf
    */
    // Check for overflow??
    SCR = GETBITS(buf[0], 6, 4);
    SCR = SCR << 30;
    SCR |= GETBITS(buf[0], 2, 1) << 28;
    SCR |= GETBITS(buf[1], 8, 1) << 20;
    SCR |= GETBITS(buf[2], 8, 4) << 15;
    SCR |= GETBITS(buf[2], 2, 1) << 13;
    SCR |= GETBITS(buf[3], 8, 1) << 5;
    SCR |= GETBITS(buf[4], 8, 4);
    //printf(" SCR: 0x%x\n", SCR);
    scr_val = (double)SCR;

    scr_val *= 300.0;
    //printf(" scr_val: %f\n", scr_val);

    SCR1 = 0;
    SCR1 =  GETBITS(buf[4], 2, 1) << 7;
    SCR1 |=  GETBITS(buf[5], 8, 2) ;
    //scr_ext = (double)SCR1;
    //printf(" scr_ext: %f\n", scr_ext);
    //printf(" final scr_val: %f\n", ((scr_val + scr_ext)/27000.0));
    //return (scr_val + scr_ext)/27000.0;
    //printf(" SCR: %llu scr: %f\n", (SCR*300)+SCR1, ((scr_val + scr_ext)/27000.0));
    ifsHandle->ifsScr = SCR*300+SCR1;
    {
        char temp[256];
        IfsLongLongToString(ifsHandle->ifsScr/27000000, temp);
        if (!ifsHandle->begClockPerContainer)
        {
            ifsHandle->begClockPerContainer = ifsHandle->ifsScr;
        }

        //printf("SCR is %s\n",  temp);
    }
}


