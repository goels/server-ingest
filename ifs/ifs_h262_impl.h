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

#ifndef _IFS_H262_IMPL_H
#define _IFS_H262_IMPL_H "$Rev: 141 $"

#include <glib.h>
#include <stdio.h>
#include "IfsIntf.h"
#include "ifs_streamdefs.h"

typedef enum
{
	// **** Bits 0-15 *****are Transport stream header bits, defined in Ifs_streamdef.h

    // Start Code events:

    IfsIndexStartPicture0 = (unsigned) 1 << 16, // PICTURE_START_CODE  00 (bit 0)
    IfsIndexStartPicture1 = (unsigned) 1 << 17, // PICTURE_START_CODE  00 (bit 1)
    IfsIndexStartUserData = (unsigned) 1 << 18, // USER_DATA           B2
    IfsIndexStartSeqHeader = (unsigned) 1 << 19, // SEQUENCE_HEADER     B3
    IfsIndexStartSeqError = (unsigned) 1 << 20, // SEQUENCE_ERROR      B4
    IfsIndexStartSeqEnd = (unsigned) 1 << 21, // SEQUENCE_END        B7
    IfsIndexStartGroup = (unsigned) 1 << 22, // GROUP_START_CODE    B8
    IfsIndexStartExtension = (unsigned) 1 << 23, // EXTENSION_START     B5

    // Extension Events:
    // defined in Ifs_streamdef.h

} IfsH262Index;

#define IfsIndexStartPicture  (IfsIndexStartPicture0|IfsIndexStartPicture1)
#define IfsIndexInfoStructure (IfsIndexInfoStructure0|IfsIndexInfoStructure1)
#define IfsIndexInfoProgRep   (IfsIndexInfoRepeatFirst|IfsIndexInfoTopFirst)


typedef enum
{
	// H.264 bits mask:
	IfsHeaderBitMask_h262 =
            IfsIndexStartPicture |
            IfsIndexStartUserData |
            IfsIndexStartSeqHeader |
            IfsIndexStartSeqError |
            IfsIndexStartExtension |
            IfsIndexStartSeqEnd |
            IfsIndexStartGroup

} IfsHeaderMask_h262;

typedef enum
{

    IfsH262IndexerSettingVerbose = // index all possible events

    		// Adaptation events:
    		IfsHeaderBitMask_Adptation  |
            // Transport header events:
    		IfsHeaderBitMask_Transport	|
    		// Start Code events: H.262
    		IfsHeaderBitMask_h262 |
    		// Extension Events:
            IfsHeaderBitMask_Extension |

#ifdef DEBUG_ALL_PES_CODES
            IfsHeaderBitMask_PES |
#endif

            0,

    IfsH262IndexerSettingUnitest =

    		// Adaptation events:
    		IfsHeaderBitMask_Adptation  |
            // Transport header events:
    		IfsHeaderBitMask_Transport	|
    		// Start Code events: H.262
       		IfsHeaderBitMask_h262 |
            // Extension Events:
            IfsHeaderBitMask_Extension |

#ifdef DEBUG_ALL_PES_CODES
            IfsHeaderBitMask_PES |
#endif

            0,

    IfsH262IndexerSettingDefPlus = // Default plus PCR and PTS indexing

    		IfsIndexAdaptPcreBit |
    		IfsIndexStartPicture |
    		IfsIndexStartSeqHeader |

#ifdef DEBUG_ALL_PES_CODES
            IfsIndexStartVideo |
            IfsIndexInfoContainsPts |
#endif

            0,

    IfsH262IndexerSettingDefault =

    		IfsIndexStartPicture |
    		IfsIndexStartSeqHeader |
    0,

} IfsH262IndexerSetting;


typedef enum // next 00 next 01 next B5 next EX next 1X next 8X   else   where
{ // ------- ------- ------- ------- ------- ------- ------- -------
    IfsStateInitial, // Zero_01                                         Initial buf[ 0]
    IfsStateZero_01, // Zero_02                                         Initial buf[ 1]
    IfsStateZero_02, // Zero_02 Found_1                                 Initial buf[ 2]
    IfsStateFound_1, // GotPic0         GotExt0 GotVid0                 Initial buf[ 3]
    //
    IfsStateGotPic0, //                                                 GotPic1 buf[ 4]
    IfsStateGotPic1, //                                                 Initial buf[ 5]
    //
    IfsStateGotExt0, //                                 GotExt1 GotExt2 Initial buf[ 4]
    IfsStateGotExt1, //                                                 Initial buf[ 5]
    IfsStateGotExt2, //                                                 GotExt3 buf[ 5]
    IfsStateGotExt3, //                                                 GotExt4 buf[ 6]
    IfsStateGotExt4, //                                                 GotExt5 buf[ 7]
    IfsStateGotExt5, //                                                 Initial buf[ 8]

    IfsStateGotVid0, //                                                 GotVid1 buf[ 4]
    IfsStateGotVid1, //                                                 GotVid2 buf[ 5]
    IfsStateGotVid2, //                                                 GotVid3 buf[ 6]
    IfsStateGotVid3, //                                         GotVid4 Initial buf[ 7]
    IfsStateGotVid4, //                                                 GotVid5 buf[ 8]
    IfsStateGotVid5, //                                                 GotVid6 buf[ 9]
    IfsStateGotVid6, //                                                 GotVid7 buf[10]
    IfsStateGotVid7, //                                                 GotVid8 buf[11]
    IfsStateGotVid8, //                                                 GotVid9 buf[12]
    IfsStateGotVid9,
//                                                 Initial buf[13]

} IfsH262State;

typedef struct IfsH262CodecImpl
{
    IfsBoolean (*ParsePacket)(IfsHandle ifsHandle, IfsPacket * pIfsPacket);
    char* (*ParseWhat)(IfsHandle ifsHandle, char * temp,
                const IfsIndexDumpMode ifsIndexDumpMode, const IfsBoolean);
    void (*CountIndexes)(ullong ifsIndex);
    void (*DumpIndexes)(void);
    void (*DumpHandle)(IfsHandle ifsHandle);

    IfsPcr ifsPcr;
    IfsPts ifsPts;
    IfsPid videoPid;
    IfsPid audioPid;
    unsigned char oldEsp;
    unsigned char oldSc;
    unsigned char oldTp;
    unsigned char oldCc;

} IfsH262CodecImpl;


#endif
