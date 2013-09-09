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

#ifndef _IFS_STREAMDEFS_H
#define _IFS_STREAMDEFS_H "$Rev: 141 $"

#include <glib.h>
#include <stdio.h>
#include "IfsIntf.h"

typedef enum
{
    // Adaptation events:

    IfsIndexAdaptAfeeBit = (unsigned) 1 << 0, // Adaptation field extension exists
    IfsIndexAdaptTpdeBit = (unsigned) 1 << 1, // Transport private data exists
    IfsIndexAdaptSpeBit = (unsigned) 1 << 2, // Splicing point exists
    IfsIndexAdaptOpcreBit = (unsigned) 1 << 3, // Old PCR exists
    IfsIndexAdaptPcreBit = (unsigned) 1 << 4, // PCR exists
    IfsIndexAdaptEspChange = (unsigned) 1 << 5, // Elementary stream priority CHANGED
    IfsIndexAdaptRaiBit = (unsigned) 1 << 6, // Random Access indicator
    IfsIndexAdaptDiBit = (unsigned) 1 << 7, // Discontinuity indicator

    // Transport header events:

    IfsIndexHeaderBadSync = (unsigned) 1 << 8, // Sync byte was not 0x47
    IfsIndexHeaderTpChange = (unsigned) 1 << 9, // Transport Priority CHANGED
    IfsIndexHeaderPusiBit = (unsigned) 1 << 10, // Payload Unit Start Indicator
    IfsIndexHeaderTeiBit = (unsigned) 1 << 11, // Transport Error Indicator
    IfsIndexHeaderCcError = (unsigned) 1 << 12, // Continuity counter ERROR
    IfsIndexHeaderScChange = (unsigned) 1 << 13, // Scrambling control CHANGED
    IfsIndexHeaderAfeBit = (unsigned) 1 << 14, // Adaptation field exists
    IfsIndexHeaderPdeBit = (unsigned) 1 << 15, // Payload data exists

} IfsHeader_TS;


typedef enum {

    IfsIndexStartSlice = 1uLL << 32, // SLICE                     01 - AF
    IfsIndexStartReservedB0 = 1uLL << 33, // RESERVED                  B0
    IfsIndexStartReservedB1 = 1uLL << 34, // RESERVED                  B1
    IfsIndexStartReservedB6 = 1uLL << 35, // RESERVED                  B6
	IfsIndexStartMpegEnd = 1uLL << 36, // MPEG_PROGRAM_END_CODE     B9
	IfsIndexStartPack = 1uLL << 37, // PACK_START_CODE           BA
	IfsIndexStartSysHeader = 1uLL << 38, // SYSTEM_HEADER_START_CODE  BB
	IfsIndexStartProgramMap = 1uLL << 39, // PROGRAM_STREAM_MAP        BC
	IfsIndexStartPrivate1 = 1uLL << 40, // PRIVATE_STREAM_1          BD
	IfsIndexStartPadding = 1uLL << 41, // PADDING_STREAM            BE
	IfsIndexStartPrivate2 = 1uLL << 42, // PRIVATE_STREAM_2          BF
	IfsIndexStartAudio = 1uLL << 43, // AUDIO                     C0 - DF
	IfsIndexStartVideo = 1uLL << 44, // VIDEO                     E0 - EF
	IfsIndexStartEcm = 1uLL << 45, // ECM_STREAM                F0
	IfsIndexStartEmm = 1uLL << 46, // EMM_STREAM                F1
	IfsIndexStartDsmCc = 1uLL << 47, // DSM_CC_STREAM             F2
	IfsIndexStart13522 = 1uLL << 48, // ISO_IEC_13522_STREAM      F3
	IfsIndexStartItuTypeA = 1uLL << 49, // ITU_T_REC_H_222_1_TYPE_A  F4
	IfsIndexStartItuTypeB = 1uLL << 50, // ITU_T_REC_H_222_1_TYPE_B  F5
	IfsIndexStartItuTypeC = 1uLL << 51, // ITU_T_REC_H_222_1_TYPE_C  F6
	IfsIndexStartItuTypeD = 1uLL << 52, // ITU_T_REC_H_222_1_TYPE_D  F7
	IfsIndexStartItuTypeE = 1uLL << 53, // ITU_T_REC_H_222_1_TYPE_E  F8
	IfsIndexStartAncillary = 1uLL << 54, // ANCILLARY_STREAM          F9
	IfsIndexStartRes_FA_FE = 1uLL << 55, // RESERVED                  FA - FE
	IfsIndexStartDirectory = 1uLL << 56, // PROGRAM_STREAM_DIRECTORY  FF

	IfsIndexInfoContainsPts = 1uLL << 57,
	IfsIndexInfoProgSeq = 1uLL << 58,
	IfsIndexInfoRepeatFirst = 1uLL << 59,
	IfsIndexInfoTopFirst = 1uLL << 60,
	IfsIndexInfoProgFrame = 1uLL << 61,
	IfsIndexInfoStructure0 = 1uLL << 62,
	IfsIndexInfoStructure1 = 1uLL << 63,

	// Extension Events:

	IfsIndexExtReserved = (unsigned) 1 << 24, // RESERVED            0, 6 and B-F
	IfsIndexExtSequence = (unsigned) 1 << 25, // SEQUENCE_EXTENSION_ID           1
	IfsIndexExtDisplay = (unsigned) 1 << 26, // SEQUENCE_DISPLAY_EXTENSION_ID   2
	IfsIndexExtQuantMat = (unsigned) 1 << 27, // QUANT_MATRIX_EXTENSION_ID       3
	IfsIndexExtCopyright = (unsigned) 1 << 28, // COPYRIGHT_EXTENSION_ID          4
	IfsIndexExtScalable = (unsigned) 1 << 29, // SEQUENCE_SCALABLE_EXTENSION_ID  5
	IfsIndexExtPictOther = (unsigned) 1 << 30, // Other PICTURE_EXTENSION_IDs     7 and 9-A
	IfsIndexExtPictCode = (unsigned) 1 << 31,
	// PICTURE_CODING_EXTENSION_ID     8

} IfsHeader_PES;



typedef enum
{
	IfsHeaderBitMask_Adptation =
			  IfsIndexAdaptAfeeBit | IfsIndexAdaptTpdeBit | IfsIndexAdaptSpeBit
            | IfsIndexAdaptOpcreBit | IfsIndexAdaptPcreBit
            | IfsIndexAdaptEspChange | IfsIndexAdaptRaiBit | IfsIndexAdaptDiBit

} IfsHeaderMask_Adaptation;


typedef enum
{
	IfsHeaderBitMask_Transport =
            IfsIndexHeaderBadSync | IfsIndexHeaderTpChange
            | IfsIndexHeaderPusiBit | IfsIndexHeaderTeiBit
            | IfsIndexHeaderCcError | IfsIndexHeaderPdeBit
            | IfsIndexHeaderAfeBit | IfsIndexHeaderScChange

} IfsHeaderMask_Transport;


typedef enum
{
	// PES bits mask:

	IfsHeaderBitMask_PES =
		IfsIndexStartSlice |
		IfsIndexStartReservedB0 |
		IfsIndexStartReservedB1 |
		IfsIndexStartReservedB6 |
		IfsIndexStartMpegEnd |
		IfsIndexStartPack |
		IfsIndexStartSysHeader |
		IfsIndexStartProgramMap |
		IfsIndexStartPrivate1 |
		IfsIndexStartPadding |
		IfsIndexStartPrivate2 |
		IfsIndexStartAudio |
		IfsIndexStartVideo |
		IfsIndexStartEcm |
		IfsIndexStartEmm |
		IfsIndexStartDsmCc |
		IfsIndexStart13522 |
		IfsIndexStartItuTypeA |
		IfsIndexStartItuTypeB |
		IfsIndexStartItuTypeC |
		IfsIndexStartItuTypeD |
		IfsIndexStartItuTypeE |
		IfsIndexStartAncillary |
		IfsIndexStartRes_FA_FE |
		IfsIndexStartDirectory |
		IfsIndexInfoContainsPts |
		IfsIndexInfoProgSeq |
		IfsIndexInfoRepeatFirst |
		IfsIndexInfoTopFirst |
		IfsIndexInfoProgFrame |
		IfsIndexInfoStructure0 |
		IfsIndexInfoStructure1

} IfsHeaderMask_PES;


typedef enum
{
	// extension bits mask
	IfsHeaderBitMask_Extension =

	IfsIndexExtReserved | IfsIndexExtSequence | IfsIndexExtDisplay
    | IfsIndexExtQuantMat | IfsIndexExtCopyright | IfsIndexExtScalable
    | IfsIndexExtPictOther | IfsIndexExtPictCode

} IfsHeaderMask_Extensions;


#endif
