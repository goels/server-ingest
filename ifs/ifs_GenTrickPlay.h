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

// Private IFS Trick Play Generation  Definitions

#ifndef __IFS_GENTRICKPLAY_H__
#define __IFS_GENTRICKPLAY_H__

#include "getMpegTsPsi.h"

#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

#define MAX_I_FRAMES_PER_SEC		2
#define MAX_FRAMES_PER_SEC_LIMIT	8

//#define DEBUG_TEST	1
#define IFS_FLAG_H264_AUD		IfsIndexNalUnitAccessUnitDelimiter
#define IFS_FLAG_H264_SPS		IfsIndexNalUnitSeqParamSet
#define IFS_FLAG_H264_PIC_I		IfsIndexPictureI
#define IFS_FLAG_H264_PIC_P		IfsIndexPictureP
#define IFS_FLAG_H264_PIC_B		IfsIndexPictureB
#define IFS_FLAG_H264_PIC_IEnd	IfsIndexNalUnitAccessUnitDelimiter

#define IFS_FLAG_H262_GOP		IfsIndexStartGroup
#define IFS_FLAG_H262_SEQ		IfsIndexStartSeqHeader
#define IFS_FLAG_H262_PIC_I		IfsIndexStartPicture0
#define IFS_FLAG_H262_PIC_P		IfsIndexStartPicture1
#define IFS_FLAG_H262_PIC_B		IfsIndexStartPicture
#define IFS_FLAG_H262_PIC_IEnd	(IfsIndexStartSeqHeader	|	\
								 IfsIndexStartPicture | IfsIndexStartPicture0 | IfsIndexStartPicture1 )

#ifndef MAX_PATH
#define MAX_PATH 260
#endif


typedef struct _entrySet
{
	IfsBoolean		valid;			// valid flag
	IfsCodecType 	codecType;		// 2 for 262, 4 for 264
	IfsIndexEntry 	entry_AUD; 		// entry for AUD for h.264 or GOP for h.262
	IfsIndexEntry 	entry_SPS;		// entry for SPS for h.264 or SEQ for h.262
	IfsIndexEntry 	entry_IframeB; 	// entry for I-frame start
	IfsIndexEntry	entry_IframeE;	// entry for I-frame end
} entrySet;


typedef struct _refIframeEntery
{
	unsigned long index;
	unsigned long uIframeNum;
	IfsIndexEntry entry;
	IfsIndexEntry prev_entry;
	unsigned	  pktCount;
	unsigned 	  pktSize;
	IfsClock	  timeToNextI;
	unsigned	  speed;
} refIframeEntry;



// trick file generation stuff
typedef struct _trickInfo
{
	IfsHandle ifsHandle;
	IfsBoolean newTrickFile;
	IfsBoolean firstRefIframe;
	unsigned long uIframeNum;
	unsigned trick_speed;
	int		 trick_direction;
    IfsCodecType codecType;
	long total_frame_count;
	char ts_filename[MAX_PATH];
	char trick_filename[MAX_PATH];
	FILE *pFile_ts, *pFile_tm, *pFile_ndx;
	char* (*parseWhat)(IfsHandle ifsHandle, char * temp,
						const IfsIndexDumpMode ifsIndexDumpMode, const IfsBoolean);
	entrySet   *trick_entryset;
    refIframeEntry *refIframe;
    int  patByteCount, pmtByteCount;
    char *patPackets;
    char *pmtPackets;
} trickInfo;



typedef struct _streamInfo
{
	IfsContainerType containerType;
	unsigned short program;
    IfsCodecType codecType;
    unsigned int tsPktSize;
    unsigned short patPID;
    unsigned short pmtPID;
    unsigned short videoPID;
    unsigned short audioPID;
    char *stream_filename;
    unsigned int pat_pkt_nums[MAX_PAT_PKT_COUNT];
    unsigned int pmt_pkt_nums[MAX_PMT_PKT_COUNT];	// PMT could have more than one packet
} streamInfo;



IfsBoolean generate_trickfile(char *indexfilename, streamInfo *strmInfo, int trickspeed, char * destdir);



#endif
