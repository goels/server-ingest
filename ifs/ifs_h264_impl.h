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

#ifndef _IFS_H264_IMPL_H
#define _IFS_H264_IMPL_H "$Rev: 141 $"

#include <glib.h>
#include <stdio.h>
#include "IfsIntf.h"
#include "ifs_streamdefs.h"

// H.264 elementary stream header bit defines
typedef enum
{
    // Start Code events:
    IfsIndexNalUnitUnspecified = (unsigned) 1 << 16, // Unspecified NAL Unit
    IfsIndexNalUnitCodedSliceNonIDRPict = (unsigned) 1 << 17, //Coded SLice of a non-IDR Picture
    IfsIndexNalUnitCodedSliceDataPartA = (unsigned) 1 << 18,
    IfsIndexNalUnitCodedSliceDataPartB = (unsigned) 1 << 19,
    IfsIndexNalUnitCodedSliceDataPartitionC = (unsigned) 1 << 20,
    IfsIndexNalUnitCodedSliceIDRPicture = (unsigned) 1 << 21,
    IfsIndexNalUnitSEI = (unsigned) 1 << 22,
    IfsIndexNalUnitSeqParamSet = (unsigned) 1 << 23,
    IfsIndexNalUnitPictParamSet = (unsigned) 1 << 24,
    IfsIndexNalUnitAccessUnitDelimiter = (unsigned) 1 << 25,
    IfsIndexNalUnitEndofSeq = (unsigned) 1 << 26,
    IfsIndexNalUnitEndofStream = (unsigned) 1 << 27,
    IfsIndexNalUnitOther = (unsigned) 1 << 28,
    IfsIndexPictureI = (unsigned) 1 << 29,
    IfsIndexPictureP = (unsigned) 1 << 30,
    IfsIndexPictureB = (unsigned) 1 << 31,
} IfsIndexAVC;


typedef enum
{
	// H.264 bits mask:
	IfsHeaderBitMask_h264 =
		IfsIndexNalUnitUnspecified |
		IfsIndexNalUnitCodedSliceNonIDRPict |
		IfsIndexNalUnitCodedSliceDataPartA	|
		IfsIndexNalUnitCodedSliceDataPartB |
		IfsIndexNalUnitCodedSliceDataPartitionC |
		IfsIndexNalUnitCodedSliceIDRPicture |
		IfsIndexNalUnitSEI |
		IfsIndexNalUnitSeqParamSet |
		IfsIndexNalUnitPictParamSet |
		IfsIndexNalUnitAccessUnitDelimiter |
		IfsIndexNalUnitEndofSeq|
		IfsIndexNalUnitEndofStream |
		IfsIndexNalUnitOther
} IfsHeaderMask_h264;


typedef enum
{
    IfsIndexStartH264Audio = 1uLL << 0,  // AUDIO
    IfsIndexStartH264Video = 1uLL << 63, // VIDEO

} IfsH264Index;

typedef enum
{

	IfsH264IndexerSettingVerbose =
    		// Adaptation events:
    		IfsHeaderBitMask_Adptation  |
            // Transport header events:
    		IfsHeaderBitMask_Transport	|
    		// H.264 bits
    		IfsHeaderBitMask_h264 |
            // Extension Events:
            IfsHeaderBitMask_Extension |

#ifdef DEBUG_ALL_PES_CODES
            IfsHeaderBitMask_PES |
#endif
            0,

    IfsH264IndexerSettingDefault =
            IfsHeaderBitMask_h264 |
            0,

} IfsH264IndexerSetting;


typedef enum
{ // ------- ------- ------- ------- ------- ------- ------- -------
    IfsH264StateInitial,

} IfsH264State;

typedef struct IfsH264CodecImpl
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
    unsigned int last4Bytes;

} IfsH264CodecImpl;


#endif
