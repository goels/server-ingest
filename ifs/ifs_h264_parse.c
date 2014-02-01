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

#define _IFS_H264_PARSE_C "$Rev: 141 $"

#include <string.h>

#include "ifs_h264_impl.h"
#include "ifs_h264_parse.h"
#include "ifs_mpeg2ts_parse.h"
#include "ifs_utils.h"



extern IfsH264Index indexerSetting;

static unsigned long indexCounts[64] = { 0 };
static unsigned long iPictureCount = 0;
static unsigned long pPictureCount = 0;
static unsigned long bPictureCount = 0;
static unsigned long iSequence = 0;
static unsigned long videoNonePts = 0;
static unsigned long videoWithPts = 0;

enum AVPictureType {
    AV_PICTURE_TYPE_P,     ///< Intra
    AV_PICTURE_TYPE_B,     ///< Predicted
    AV_PICTURE_TYPE_I,     ///< Bi-dir predicted
    AV_PICTURE_TYPE_SP,     ///< S(GMC)-VOP MPEG4
    AV_PICTURE_TYPE_SI,    ///< Switching Intra
};


static const char * golomb_to_pict_type[5] = {
    "AV_PICTURE_TYPE_P", "AV_PICTURE_TYPE_B", "AV_PICTURE_TYPE_I",
    "AV_PICTURE_TYPE_SP", "AV_PICTURE_TYPE_SI"
};

static void ParseElementary(IfsHandle ifsHandle, const unsigned char pdeLen,
        const unsigned char bytes[]);
static void ParseCodeAVC(IfsHandle ifsHandle, const unsigned char code);
static unsigned int DecodeUGolomb(unsigned int * base, unsigned int * offset);




static void ParsePacket(IfsHandle ifsHandle,
        unsigned char bytes[IFS_TRANSPORT_PACKET_SIZE])
{
    const unsigned char tpBit = (bytes[1] >> 5) & 0x01; // Transport Priority
    const unsigned char pusiBit = (bytes[1] >> 6) & 0x01; // Payload Unit Start Indicator
    const unsigned char teiBit = (bytes[1] >> 7) & 0x01; // Transport Error Indicator
    //
    const unsigned char ccBits = (bytes[3] >> 0) & 0x0F; // Continuity counter
    const unsigned char pdeBit = (bytes[3] >> 4) & 0x01; // Payload data exists
    const unsigned char afeBit = (bytes[3] >> 5) & 0x01; // Adaptation field exists
    const unsigned char scBits = (bytes[3] >> 6) & 0x03; // Scrambling control

    // Transport Priority
    if (ifsHandle->codec->h264->oldTp == IFS_UNDEFINED_BYTE)
    {
        ifsHandle->codec->h264->oldTp = tpBit;
    }
    else if (ifsHandle->codec->h264->oldTp != tpBit)
    {
        ifsHandle->entry.what |= IfsIndexHeaderTpChange;
        ifsHandle->codec->h264->oldTp = tpBit;
    }

    // Payload Unit Start Indicator
    if (pusiBit)
    {
        ifsHandle->entry.what |= IfsIndexHeaderPusiBit;
    }

    // Continuity counter
    if ((ifsHandle->codec->h264->oldCc != IFS_UNDEFINED_BYTE) && (((ifsHandle->codec->h264->oldCc + 1)
            & 0x0F) != ccBits))
    {
        ifsHandle->entry.what |= IfsIndexHeaderCcError;
    }
    ifsHandle->codec->h264->oldCc = ccBits;

    // Payload data exists
    if (pdeBit)
    {
        ifsHandle->entry.what |= IfsIndexHeaderPdeBit;
    }

    // Adaptation field exists
    if (afeBit)
    {
        ifsHandle->entry.what |= IfsIndexHeaderAfeBit;
    }

    // Scrambling control
    if (ifsHandle->codec->h264->oldSc == IFS_UNDEFINED_BYTE)
    {
        ifsHandle->codec->h264->oldSc = scBits;
    }
    else if (ifsHandle->codec->h264->oldSc != scBits)
    {
        ifsHandle->entry.what |= IfsIndexHeaderScChange;
        ifsHandle->codec->h264->oldSc = scBits;
    }

    // Transport Error Indicator
    if (teiBit)
    {
        ifsHandle->entry.what |= IfsIndexHeaderTeiBit;

        // Abandon the rest of this packet and Reset the PES state machine

        ifsHandle->ifsState = IfsStateInitial;
    }
    else if (afeBit)
    {
        const unsigned char afLen = bytes[4];

        if (afLen)
            mpeg2ts_ParseAdaptation(ifsHandle, &bytes[5]);

        if (pdeBit)
        {
            ParseElementary(ifsHandle, IFS_TRANSPORT_PACKET_SIZE - (5 + afLen),
                    &bytes[5 + afLen]);
        }
    }
    else if (pdeBit)
    {
        ParseElementary(ifsHandle, IFS_TRANSPORT_PACKET_SIZE - 4, &bytes[4]);
    }
}



static void ParseCodeAVC(IfsHandle ifsHandle, const unsigned char code)
{
	ullong what = 0;

    if (code & 0x80) // The first bit has to be zero for AVC the forbidden_zero_bit
    {
#ifdef DEBUG_ALL_PES_CODES
    	  switch(code)
    	  {
				case 0xB0: what = IfsIndexStartReservedB0; break; // RESERVED
				case 0xB1: what = IfsIndexStartReservedB1; break; // RESERVED
				case 0xB6: what = IfsIndexStartReservedB6; break; // RESERVED
				case 0xB9: what = IfsIndexStartMpegEnd; break; // MPEG_PROGRAM_END_CODE
				case 0xBA: what = IfsIndexStartPack; break; // PACK_START_CODE
				case 0xBB: what = IfsIndexStartSysHeader; break; // SYSTEM_HEADER_START_CODE
				case 0xBC: what = IfsIndexStartProgramMap; break; // PROGRAM_STREAM_MAP
				case 0xBD: what = IfsIndexStartPrivate1; break; // PRIVATE_STREAM_1
				case 0xBE: what = IfsIndexStartPadding; break; // PADDING_STREAM
				case 0xBF: what = IfsIndexStartPrivate2; break; // PRIVATE_STREAM_2
				case 0xF0: what = IfsIndexStartEcm; break; // ECM_STREAM
				case 0xF1: what = IfsIndexStartEmm; break; // EMM_STREAM
				case 0xF2: what = IfsIndexStartDsmCc; break; // DSM_CC_STREAM
				case 0xF3: what = IfsIndexStart13522; break; // ISO_IEC_13522_STREAM
				case 0xF4: what = IfsIndexStartItuTypeA; break; // ITU_T_REC_H_222_1_TYPE_A
				case 0xF5: what = IfsIndexStartItuTypeB; break; // ITU_T_REC_H_222_1_TYPE_B
				case 0xF6: what = IfsIndexStartItuTypeC; break; // ITU_T_REC_H_222_1_TYPE_C
				case 0xF7: what = IfsIndexStartItuTypeD; break; // ITU_T_REC_H_222_1_TYPE_D
				case 0xF8: what = IfsIndexStartItuTypeE; break; // ITU_T_REC_H_222_1_TYPE_E
				case 0xF9: what = IfsIndexStartAncillary; break; // ANCILLARY_STREAM
				case 0xFF: what = IfsIndexStartDirectory; break; // PROGRAM_STREAM_DIRECTORY

				default:

				if ( (0xC0 <= code) && (code <= 0xDF) )
				{   what = IfsIndexStartAudio; break;} // AUDIO

				if ( (0xFA <= code) && (code <= 0xFE) )
				{   what = IfsIndexStartRes_FA_FE; break;} // RESERVED


				if ( (0xE0 <= code) && (code <= 0xEF) )
				{   ifsHandle->extWhat = IfsIndexStartVideo; return;} // VIDEO (added later)
    	  }
#endif
    }
    else
    {
        	int nal_ref_idc = (code >> 5) & 3;
    		int nal_unit_type = code & 0x1f;
    		if( 0)
    			printf("PARSINGAVC: NAL RefId:0x%02X NAL_UNIT_TYPE:0x%02X\n", nal_ref_idc, nal_unit_type);
    		switch (nal_unit_type)
			{
			case 0x00: /* handled ind ParsePict() */
			   what = IfsIndexNalUnitUnspecified;
				break; // PICTURE_START_CODE
			case 0x01:
				what = IfsIndexNalUnitCodedSliceNonIDRPict;
				break; // USER_DATA
			case 0x02:
				what = IfsIndexNalUnitCodedSliceDataPartA;
				break; // SEQUENCE_HEADERNon
			case 0x03:
				what = IfsIndexNalUnitCodedSliceDataPartB;
				break; // SEQUENCE_ERROR
			case 0x04:
				what = IfsIndexNalUnitCodedSliceDataPartitionC;
				break; // EXTENSION_START
			case 0x05:
				what = IfsIndexNalUnitCodedSliceIDRPicture;
				break; // SEQUENCE_END
			case 0x06:
				what = IfsIndexNalUnitSEI;
				break; // GROUP_START_CODE
			case 0x07:
				 what = IfsIndexNalUnitSeqParamSet;
				 break; // GROUP_START_CODE
			case 0x08:
				 what = IfsIndexNalUnitPictParamSet;
				 break; // GROUP_START_CODE
			case 0x09:
				 what = IfsIndexNalUnitAccessUnitDelimiter;
				 break; // GROUP_START_CODE
			case 0x0a:
				  what = IfsIndexNalUnitEndofSeq;
				  break; // GROUP_START_CODE
			case 0x0b:
				  what = IfsIndexNalUnitEndofStream;
				  break; // GROUP_START_CODE
			default:
				  what = IfsIndexNalUnitOther;
				  break;
			}
    }

    ifsHandle->entry.what |= what;
}

static inline unsigned int GET_LE_LONG(unsigned char *temp)
{
    return ( (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3]);
}

static inline void GET_LE_BYTE_ARRAY(unsigned char *temp, unsigned int val)
{
    temp[3] = val & 0xff;
    temp[2] = (val >> 8) & 0xff;
    temp[1] = (val >> 16) & 0xff;
    temp[0] = (val >> 24) & 0xff;
}


static inline unsigned int get_bit(unsigned int * param, unsigned int offset)
{
    int temp = 0;
    unsigned char base[4];

    GET_LE_BYTE_ARRAY(base, *param);

    temp = ((*(base + (offset >> 0x3))) >> (0x7 - (offset & 0x7))) & 0x1;
    return temp;
}

//This function implement decoding of exp-Golomb codes of zero range (used in H.264).

static unsigned int DecodeUGolomb(unsigned int * base, unsigned int * offset)
{
    unsigned int zeros = 0;
    unsigned int info;
    int i = 0;

    // calculate zero bits. Will be optimized.
    while (0 == get_bit(base, (*offset)++)) zeros++;

    // insert first 1 bit
    info = 1 << zeros;
    // (*offset)++;

    for (i = zeros - 1; i >= 0; i--)
    {
        info |= get_bit(base, (*offset)++) << i;
    }

    //printf("Offset is %u\n", *offset);

    return (info - 1);
}



static void ParseElementary(IfsHandle ifsHandle, const unsigned char pdeLen,
        const unsigned char bytes[])
{
    unsigned char i;
	static IfsPts last_pts = 0;
    IfsPts ifsPts = last_pts;

    for (i = 0; i < pdeLen; i++)
    {
        switch (ifsHandle->ifsState)
        {
        case IfsStateInitial:

            // Searching for the first 0x00 byte

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            // Zero_01                                         Initial buf[ 0]

            if (bytes[i] == 0x00)
                ifsHandle->ifsState = IfsStateZero_01;
            break;

        case IfsStateZero_01:

            // Make sure there is a second 0x00 byte

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            // Zero_02                                         Initial buf[ 1]

            ifsHandle->ifsState = bytes[i] ? IfsStateInitial : IfsStateZero_02;
            break;

        case IfsStateZero_02:

            // Searching for the 0x01 byte, skipping any 0x00 bytes

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            // Zero_02 Found_1                                 Initial buf[ 2]

            if (bytes[i])
                ifsHandle->ifsState = bytes[i] == 0x01 ? IfsStateFound_1
                        : IfsStateInitial;
            break;

        case IfsStateFound_1:

            // Found 0x00 .. 0x00 0x01, this is the start code
        	  // CHeck the type of stream and call appropriate parsecode
        	  if (ifsHandle->codecType == IfsCodecTypeH264) // H264/AVC
        	  {
        		  ParseCodeAVC(ifsHandle, bytes[i]);
        		  if ((bytes[i] & 0x80) != 0)
        		  {
/*
        	            if (ifsHandle->entry.what & IfsIndexInfoContainsPts)
        	            {
        	            	ifsPts =  ifsHandle->codec->h264->ifsPts;
        	            }
        	            else
        	            {
        	            	ifsPts = ifsHandle->codec->h264->ifsPcr/300;
        	            }

        	            {
        	            	char temp[33], temp1[33];
        	            	IfsLongLongToString(ifsPts, temp);
        	            	IfsLongLongToString(ifsHandle->begClockPerContainer/300, temp1);
        	            	printf("PTS FOUND %s PCR %s Diff:%lld\n", temp, temp1, (ifsPts - ifsHandle->begClockPerContainer/300)/90000 );
        					ifsHandle->entry.when = ifsHandle->begClock + 100000*(ifsPts - ifsHandle->begClockPerContainer/300)/9;
        	            }
*/

        		  }
        		  else // If it happens to be a picture we need to parse for the picture type
        		  {
        			  if ( ((unsigned int)ifsHandle->entry.what & (unsigned int)IfsIndexNalUnitCodedSliceNonIDRPict) ||
        				   ((unsigned int)ifsHandle->entry.what & (unsigned int)IfsIndexNalUnitCodedSliceIDRPicture))
        			  {

        				  ifsHandle->ifsState = IfsStateGotPic0;
        			  }


        		  }

        		  if (ifsHandle->ifsState != IfsStateGotPic0)
        		  {
        			  ifsHandle->ifsState 	= ((bytes[i] & 0xF0) == 0xE0 ? IfsStateGotVid0
        					  : IfsStateInitial);
        		  }
        	  }

        	  if (ifsHandle->ifsState == IfsStateInitial)
        	  {
      				char temp[33], temp1[33];
      				IfsLongLongToString(ifsPts, temp);
      				IfsLongLongToString(ifsHandle->begClockPerContainer/300, temp1);
      				//printf("initial PTS %s PCR %s Diff:%lld\n", temp, temp1, (ifsPts - ifsHandle->begClockPerContainer/300)/90000 );
      				ifsHandle->entry.when = 100000*(ifsPts - ifsHandle->begClockPerContainer/300)/9;
        	  }

            break;

        case IfsStateGotPic0:

            // Found 0x00 .. 0x00 0x01 0x00, skip this byte

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotPic1 buf[ 4]

        	if (ifsHandle->codecType == IfsCodecTypeH264)
        	{
        		  ifsHandle->codec->h264->last4Bytes = (bytes[i] << 24);
        	}
            ifsHandle->ifsState = IfsStateGotPic1;
            break;

        case IfsStateGotPic1:

            // Found 0x00 .. 0x00 0x01 0x00 0xXX, this byte contains the picture coding type

        	if (ifsHandle->codecType == IfsCodecTypeH264)
        	{
        		unsigned int offset = 0;
				unsigned int first_mb_in_slice = 0;
				unsigned int slice_type = 0;
        		unsigned int processVal;

        		ifsHandle->codec->h264->last4Bytes |= (bytes[i] << 16);
        		processVal = ifsHandle->codec->h264->last4Bytes;
        		// Now process the bytes to determine the frame type
        	   // printf("Processing Value 0x%x\n", processVal);

        	    // Ignore the first mb in slice
        	    first_mb_in_slice= DecodeUGolomb(&processVal, &offset);

        	    if (offset < 32)
        	    {
					processVal = processVal << offset;
					//printf("Processing the next 0x%x\n", processVal);
					offset = 0;
					slice_type = DecodeUGolomb(&processVal, &offset);

					if(0)
						printf("MbinSlice %d slice_type:%d, %s \n", first_mb_in_slice, slice_type, golomb_to_pict_type[slice_type % 5]);

					if ((slice_type % 5) == AV_PICTURE_TYPE_I)
					{
						ifsHandle->entry.what |= IfsIndexPictureI;
						//printf("Adding IfsIndexPictureI\n");
					}
					else if ((slice_type % 5) == AV_PICTURE_TYPE_P)
					{
						ifsHandle->entry.what |= IfsIndexPictureP;
						//printf("Adding IfsIndexPictureP\n");
					}
					else if ((slice_type % 5) == AV_PICTURE_TYPE_B)
					{
						ifsHandle->entry.what |= IfsIndexPictureB;
						//printf("Adding IfsIndexPictureB\n");
					}
				}
            }
        	else
        	{
				ifsHandle->entry.what |= ((bytes[i] & 0x08 ? IfsIndexStartPicture0
						: 0) | (bytes[i] & 0x10 ? IfsIndexStartPicture1 : 0));

				// next 00 next 01 next B5 next EX next 1X next 8X   else   where
				// ------- ------- ------- ------- ------- ------- ------- -------
				//                                                 Initial buf[ 5]
        	}
			// Check if the pts is present then pick it up
			if (ifsHandle->entry.what & IfsIndexInfoContainsPts)
			{
				ifsPts =  ifsHandle->codec->h264->ifsPts;
				//printf("using new pts --");
			}
			else if (last_pts)
			{
				ifsPts = (last_pts + (.033 * 90000));
				//printf("using last pts --");
			}
			else
			{
				ifsPts = ifsHandle->codec->h264->ifsPcr/300;
				//printf("using pcr -- ");
			}

			{
				char temp[33], temp1[33];
				IfsLongLongToString(ifsPts, temp);
				IfsLongLongToString(ifsHandle->begClockPerContainer/300, temp1);
			//	printf("PTS  %s PCR %s Diff:%lld\n", temp, temp1, (ifsPts - ifsHandle->begClockPerContainer/300)/90000 );
				ifsHandle->entry.when = 100000*(ifsPts - ifsHandle->begClockPerContainer/300)/9;
			}
			last_pts = ifsPts;
			ifsHandle->ifsState = IfsStateInitial;
            break;

        case IfsStateGotExt0:

            // Found 0x00 .. 0x00 0x01 0xB5, this byte contains the extension

            // ParseExt(ifsHandle, bytes[i] >> 4);

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                 GotExt1 GotExt2 Initial buf[ 4]

            switch (bytes[i] & 0xF0)
            {
            case 0x10:
                ifsHandle->ifsState = IfsStateGotExt1;
                break;
            case 0x80:
                ifsHandle->ifsState = IfsStateGotExt2;
                break;
            default:
                ifsHandle->ifsState = IfsStateInitial;
                break;
            }
            break;

        case IfsStateGotExt1:

            // Found 0x00 .. 0x00 0x01 0xB5 0x1X, this byte contains the progressive sequence bit

#ifdef DEBUG_ALL_PES_CODES
            ifsHandle->isProgSeq = bytes[i] & 0x08 ? IfsTrue : IfsFalse;
            ifsHandle->extWhat |= ( ifsHandle->isProgSeq ? IfsIndexInfoProgSeq : 0 );
            ifsHandle->entry.what |= ifsHandle->extWhat;
#endif

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 Initial buf[ 5]

            ifsHandle->ifsState = IfsStateInitial;
            break;

        case IfsStateGotExt2:

            // Found 0x00 .. 0x00 0x01 0xB5 0x8X, skip this byte

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotExt3 buf[ 5]

            ifsHandle->ifsState = IfsStateGotExt3;
            break;

        case IfsStateGotExt3:

            // Found 0x00 .. 0x00 0x01 0xB5 0x8X 0x__, this byte contains the picture structure

#ifdef DEBUG_ALL_PES_CODES
            ifsHandle->extWhat |= ( (bytes[i] & 0x01 ? IfsIndexInfoStructure0 : 0) | (bytes[i] & 0x02 ? IfsIndexInfoStructure1 : 0) );
#endif

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotExt4 buf[ 6]

            ifsHandle->ifsState = IfsStateGotExt4;
            break;

        case IfsStateGotExt4:

            // Found 0x00 .. 0x00 0x01 0xB5 0x8X 0x__ 0x__, this byte contains the TFF and RFF bits

#ifdef DEBUG_ALL_PES_CODES
            ifsHandle->extWhat |= ( (bytes[i] & 0x80 ? IfsIndexInfoTopFirst : 0) | (bytes[i] & 0x02 ? IfsIndexInfoRepeatFirst : 0) );
#endif

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotExt5 buf[ 7]

            ifsHandle->ifsState = IfsStateGotExt5;
            break;

        case IfsStateGotExt5:

            // Found 0x00 .. 0x00 0x01 0xB5 0x8X 0x__ 0x__ 0x__, this byte contains the progressive frame bit

#ifdef DEBUG_ALL_PES_CODES
            ifsHandle->extWhat |= ( bytes[i] & 0x80 ? IfsIndexInfoProgFrame : 0 );
            ifsHandle->extWhat |= ( ifsHandle->isProgSeq ? IfsIndexInfoProgSeq : 0 );
            ifsHandle->entry.what |= ifsHandle->extWhat;
#endif

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 Initial buf[ 8]

            ifsHandle->ifsState = IfsStateInitial;
            break;

        case IfsStateGotVid0:

            // Found 0x00 .. 0x00 0x01 0xEX, this byte contains the first half of the PES packet length

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid1 buf[ 4]
            ifsHandle->ifsState = IfsStateGotVid1;
            break;

        case IfsStateGotVid1:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__ this byte contains the second half of the PES packet length

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid2 buf[ 5]
            ifsHandle->ifsState = IfsStateGotVid2;
            break;

        case IfsStateGotVid2:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__ this byte contains:
            //      marker                   2 '10'
            //      PES_scrambling_control   2 bslbf
            //      PES_priority             1 bslbf
            //      data_alignment_indicator 1 bslbf
            //      copyright                1 bslbf
            //      original_or_copy         1 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid3 buf[ 6]
            ifsHandle->ifsState = IfsStateGotVid3;
            break;

        case IfsStateGotVid3:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__ this byte contains:
            //      PTS_DTS_flags             2 bslbf
            //      ESCR_flag                 1 bslbf
            //      ES_rate_flag              1 bslbf
            //      DSM_trick_mode_flag       1 bslbf
            //      additional_copy_info_flag 1 bslbf
            //      PES_CRC_flag              1 bslbf
            //      PES_extension_flag        1 bslbf

            // PTS_DTS_flags -- This is a 2 bit field. If the PTS_DTS_flags field equals '10', the PTS fields shall be present
            // in the PES packet header. If the PTS_DTS_flags field equals '11', both the PTS fields and DTS fields shall be
            // present in the PES packet header. If the PTS_DTS_flags field equals '00' no PTS or DTS fields shall be present
            // in the PES packet header. The value '01' is forbidden.

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                         GotVid4 Initial buf[ 7]

#ifdef DEBUG_ALL_PES_CODES
            ifsHandle->extWhat |= ( bytes[i] & 0x80 ? IfsIndexInfoContainsPts : 0 );
            ifsHandle->entry.what |= ifsHandle->extWhat;
            ifsHandle->entry.pts = IfsTrue;
	    	//printf("IfsIndexInfoContainsPts set\n");
#endif

            ifsHandle->ifsState = bytes[i] & 0x80 ? IfsStateGotVid4
                    : IfsStateInitial;
            break;

        case IfsStateGotVid4:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PES_header_data_length    8 uimsbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid5 buf[ 8]

            //          RILOG_INFO(" 8 0x%02X\n", bytes[i]);
            ifsHandle->ifsState = IfsStateGotVid5;
            break;

        case IfsStateGotVid5:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      '001x'                4 bslbf   buf[9]
            //      PTS [32..30]          3 bslbf
            //      marker_bit            1 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid6 buf[ 9]

            ifsHandle->codec->h264->ifsPts = ((IfsPts)(bytes[i] & 0x0E)) << 29; // 32..30
            //          RILOG_INFO(" 9 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h264->ifsPts>>32),
            //                 (long)ifsHandle->codec->h264->ifsPts, IfsLongLongToString(ifsHandle->codec->h264->ifsPts));
            ifsHandle->ifsState = IfsStateGotVid6;
            break;

        case IfsStateGotVid6:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PTS [29..22]          8 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid7 buf[10]

            ifsHandle->codec->h264->ifsPts |= ((IfsPts) bytes[i]) << 22; // 29..22
            //          RILOG_INFO("10 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h264->ifsPts>>32),
            //                 (long)ifsHandle->codec->h264->ifsPts, IfsLongLongToString(ifsHandle->codec->h264->ifsPts));
            ifsHandle->ifsState = IfsStateGotVid7;
            break;

        case IfsStateGotVid7:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PTS [21..15]          7 bslbf
            //      marker_bit            1 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid8 buf[11]

            ifsHandle->codec->h264->ifsPts |= ((IfsPts)(bytes[i] & 0xFE)) << 14; // 21..15
            //          RILOG_INFO("11 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h264->ifsPts>>32),
            //                 (long)ifsHandle->codec->h264->ifsPts, IfsLongLongToString(ifsHandle->codec->h264->ifsPts));
            ifsHandle->ifsState = IfsStateGotVid8;
            break;

        case IfsStateGotVid8:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PTS [14..7]           8 bslbf
            //      marker_bit            1 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid9 buf[12]

            ifsHandle->codec->h264->ifsPts |= ((IfsPts) bytes[i]) << 7; // 14.. 7
            //          RILOG_INFO("12 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h264->ifsPts>>32),
            //                 (long)ifsHandle->codec->h264->ifsPts, IfsLongLongToString(ifsHandle->codec->h264->ifsPts));
            ifsHandle->ifsState = IfsStateGotVid9;
            break;

        case IfsStateGotVid9:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PTS [6..0]            7 bslbf
            //      marker_bit            1 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 Initial buf[13]

            ifsHandle->codec->h264->ifsPts |= ((IfsPts)(bytes[i] & 0xFE)) >> 1; // 6..0
            //          RILOG_INFO("13 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h264->ifsPts>>32),
            //                 (long)ifsHandle->codec->h264->ifsPts, IfsLongLongToString(ifsHandle->codec->h264->ifsPts));
  			last_pts = ifsHandle->codec->h264->ifsPts;
            ifsHandle->ifsState = IfsStateInitial;
            break;
        }
    }
}

IfsBoolean h264_ParsePacket(IfsHandle ifsHandle, IfsPacket * pIfsPacket)
{

    if (pIfsPacket->bytes[0] == 0x47)
    {
        ifsHandle->entry.what = 0;

        if (mpeg2ts_GetPid(pIfsPacket) == ifsHandle->codec->h264->videoPid)
        {
             //printf("parsing MPEG2TS packet\n");
            ParsePacket(ifsHandle, pIfsPacket->bytes);
        }
    }
    else if(pIfsPacket->bytes[4] == 0x47) // 192 byte packets
    {
        ifsHandle->entry.what = 0;
        //printf("parsing 192-byte MPEG2TS packet\n")
        if (mpeg2ts_GetPid(pIfsPacket) == ifsHandle->codec->h264->videoPid)
        {
            ParsePacket(ifsHandle, (unsigned char*)&pIfsPacket->bytes[4]);
        }
    }
    else  // TODO: this section should parse MPEG PS
    {
        //printf("parsing MPEG2PS packet\n");
        ParseElementary(ifsHandle, ifsHandle->pktSize, pIfsPacket->bytes);
    }

    // any indexed events in this packet?
    return ( ((unsigned long)(ifsHandle->entry.what>>32) & (unsigned long)(indexerSetting>>32))
            || ((unsigned long)ifsHandle->entry.what & indexerSetting) );
}


char * h264_ParseWhat(IfsHandle ifsHandle, char * temp,
        const IfsIndexDumpMode ifsIndexDumpMode, const IfsBoolean dumpPcrAndPts)
{
	long long unsigned int ifsIndex = ifsHandle->entry.what;
     temp[0] = 0;

    if (ifsIndexDumpMode == IfsIndexDumpModeAll)
    {
        if (ifsIndex & IfsIndexHeaderBadSync)
            strcat(temp, "BadSync  ");
        if (ifsIndex & IfsIndexHeaderTpChange)
            strcat(temp, "TpChange ");
        if (ifsIndex & IfsIndexHeaderPusiBit)
            strcat(temp, "PusiBit  ");
        if (ifsIndex & IfsIndexHeaderTeiBit)
            strcat(temp, "TeiBit   ");
        if (ifsIndex & IfsIndexHeaderCcError)
            strcat(temp, "CcError  ");
        if (ifsIndex & IfsIndexHeaderPdeBit)
            strcat(temp, "PdeBit   ");
        if (ifsIndex & IfsIndexHeaderAfeBit)
            strcat(temp, "AfeBit   ");
        if (ifsIndex & IfsIndexHeaderScChange)
            strcat(temp, "ScChange ");

        if (ifsIndex & IfsIndexAdaptAfeeBit)
            strcat(temp, "AdaptAfeeBit    ");
        if (ifsIndex & IfsIndexAdaptTpdeBit)
            strcat(temp, "AdaptTpdeBit    ");
        if (ifsIndex & IfsIndexAdaptSpeBit)
            strcat(temp, "AdaptSpeBit     ");
        if (ifsIndex & IfsIndexAdaptOpcreBit)
            strcat(temp, "AdaptOpcreBit   ");
        if (ifsIndex & IfsIndexAdaptPcreBit)
        {
            if (dumpPcrAndPts)
            {
                strcat(temp, "AdaptPcreBit(");
                (void) IfsLongLongToString(ifsHandle->codec->h264->ifsPcr,
                        &temp[strlen(temp)]);
                strcat(temp, ") ");
            }
            else
                strcat(temp, "AdaptPcreBit ");
        }
        if (ifsIndex & IfsIndexAdaptEspChange)
            strcat(temp, "AdaptEspChange  ");
        if (ifsIndex & IfsIndexAdaptRaiBit)
            strcat(temp, "AdaptRaiBit     ");
        if (ifsIndex & IfsIndexAdaptDiBit)
            strcat(temp, "AdaptDiBit      ");

		if (ifsIndex & IfsIndexNalUnitUnspecified) strcat(temp, "NAL-Unspecified      ");
		if (ifsIndex & IfsIndexNalUnitCodedSliceNonIDRPict) strcat(temp, "NAL-NonIDR      ");
		if (ifsIndex & IfsIndexNalUnitCodedSliceDataPartA) strcat(temp, "NAL-DataPartA     ");
		if (ifsIndex &   IfsIndexNalUnitCodedSliceDataPartB) strcat(temp, "NAL-DataPartB      ");
		if (ifsIndex & IfsIndexNalUnitCodedSliceDataPartitionC) strcat(temp, "NAL-DataPartC      ");
		if (ifsIndex & IfsIndexNalUnitCodedSliceIDRPicture) strcat(temp, "NAL-IDR      ");
		if (ifsIndex & IfsIndexNalUnitSEI) strcat(temp, "NAL-SEI      ");
		if (ifsIndex &  IfsIndexNalUnitSeqParamSet) strcat(temp, "NAL-SeqSet      ");
		if (ifsIndex & IfsIndexNalUnitPictParamSet) strcat(temp, "NAL-PPS      ");
		if (ifsIndex & IfsIndexNalUnitAccessUnitDelimiter) strcat(temp, "NAL-AccessDel      ");
		if (ifsIndex & IfsIndexNalUnitEndofSeq) strcat(temp, "NAL-EndOfSeq      ");
		if (ifsIndex & IfsIndexNalUnitEndofStream) strcat(temp, "NAL-EndofSt      ");
		if (ifsIndex & IfsIndexNalUnitOther) strcat(temp, "NAL-Other      ");
		if (ifsIndex & IfsIndexPictureI) strcat(temp, "StartIpicture   ");
		if (ifsIndex & IfsIndexPictureP) strcat(temp, "StartPpicture   ");
		if (ifsIndex & IfsIndexPictureB) strcat(temp, "StartBpicture   ");
    }
    else if (ifsIndexDumpMode == IfsIndexDumpModeDef)
    {
    		ifsIndex &= indexerSetting; // Clean up the output in this mode

			if (ifsIndex & IfsIndexHeaderBadSync)
				strcat(temp, "BadSync ");
			if (ifsIndex & IfsIndexHeaderTpChange)
				strcat(temp, "TpChange ");
			if (ifsIndex & IfsIndexHeaderPusiBit)
				strcat(temp, "Pusi ");
			if (ifsIndex & IfsIndexHeaderTeiBit)
				strcat(temp, "Tei ");
			if (ifsIndex & IfsIndexHeaderCcError)
				strcat(temp, "CcError ");
			if (ifsIndex & IfsIndexHeaderPdeBit)
				strcat(temp, "Pde ");
			if (ifsIndex & IfsIndexHeaderAfeBit)
				strcat(temp, "Afe ");
			if (ifsIndex & IfsIndexHeaderScChange)
				strcat(temp, "ScChange ");

			if (ifsIndex & IfsIndexAdaptAfeeBit)
				strcat(temp, "Afee ");
			if (ifsIndex & IfsIndexAdaptTpdeBit)
				strcat(temp, "Tpde ");
			if (ifsIndex & IfsIndexAdaptSpeBit)
				strcat(temp, "Spe ");
			if (ifsIndex & IfsIndexAdaptOpcreBit)
				strcat(temp, "Opcre ");
			if (ifsIndex & IfsIndexAdaptPcreBit)
			{
				if (dumpPcrAndPts)
				{
					strcat(temp, "Pcre(");
					(void) IfsLongLongToString(ifsHandle->codec->h264->ifsPcr,
							&temp[strlen(temp)]);
					strcat(temp, ") ");
				}
				else
					strcat(temp, "Pcre ");

			}
			if (ifsIndex & IfsIndexAdaptEspChange)
				strcat(temp, "EspChange ");
			if (ifsIndex & IfsIndexAdaptRaiBit)
				strcat(temp, "Rai ");
			if (ifsIndex & IfsIndexAdaptDiBit)
				strcat(temp, "Di ");

#ifdef DEBUG_ALL_PES_CODES
			if (ifsIndex & IfsIndexPictureI)
			{
				strcat(temp, "I ");
			}
			else if (ifsIndex & IfsIndexPictureP)
			{
				strcat(temp, "P ");
			}
			else if (ifsIndex & IfsIndexPictureB)
			{
				strcat(temp, "B ");
			}

			if (ifsIndex & IfsIndexNalUnitUnspecified) strcat(temp, "NAL-Unspecified ");
			if (ifsIndex & IfsIndexNalUnitCodedSliceNonIDRPict) strcat(temp, "NAL-NonIDR      ");
			if (ifsIndex & IfsIndexNalUnitCodedSliceDataPartA) strcat(temp, "NAL-DataPartA     ");
			if (ifsIndex &   IfsIndexNalUnitCodedSliceDataPartB) strcat(temp, "NAL-DataPartB      ");
			  if (ifsIndex & IfsIndexNalUnitCodedSliceDataPartitionC) strcat(temp, "NAL-DataPartC      ");
			if (ifsIndex & IfsIndexNalUnitCodedSliceIDRPicture) strcat(temp, "NAL-IDR      ");
			if (ifsIndex & IfsIndexNalUnitSEI) strcat(temp, "NAL-SEI      ");
			if (ifsIndex &  IfsIndexNalUnitSeqParamSet) strcat(temp, "NAL-Seq Set      ");
			if (ifsIndex & IfsIndexNalUnitPictParamSet) strcat(temp, "NAL-PPS      ");
			if (ifsIndex & IfsIndexNalUnitAccessUnitDelimiter) strcat(temp, "NAL-AcesssDel      ");
			if (ifsIndex & IfsIndexNalUnitEndofSeq) strcat(temp, "NAL-EndOfSeq      ");
			if (ifsIndex & IfsIndexNalUnitEndofStream) strcat(temp, "NAL-EndofSt      ");
			if (ifsIndex & IfsIndexNalUnitOther) strcat(temp, "NAL-Other      ");

			if (ifsIndex & IfsIndexStartPack ) strcat(temp, "Pack ");
			if (ifsIndex & IfsIndexStartSysHeader ) strcat(temp, "SysHeader ");
			if (ifsIndex & IfsIndexStartProgramMap) strcat(temp, "ProgramMap ");
			if (ifsIndex & IfsIndexStartPrivate1 ) strcat(temp, "Private1 ");
			if (ifsIndex & IfsIndexStartPadding ) strcat(temp, "Padding ");
			if (ifsIndex & IfsIndexStartPrivate2 ) strcat(temp, "Private2 ");
			if (ifsIndex & IfsIndexStartAudio ) strcat(temp, "Audio ");
			if (ifsIndex & IfsIndexStartVideo )
			{
				if (ifsIndex & IfsIndexInfoContainsPts)
				{
					if (dumpPcrAndPts)
					{
						strcat(temp, "Video(");
						(void)IfsLongLongToString(ifsHandle->codec->h264->ifsPts, &temp[strlen(temp)]);
						strcat(temp, ") ");
					}
					else strcat(temp, "Video(PTS) ");
				}
				else strcat(temp, "Video ");
			}
			if (ifsIndex & IfsIndexStartEcm ) strcat(temp, "Ecm ");
			if (ifsIndex & IfsIndexStartEmm ) strcat(temp, "Emm ");
			if (ifsIndex & IfsIndexStartDsmCc ) strcat(temp, "DsmCc ");
			if (ifsIndex & IfsIndexStart13522 ) strcat(temp, "13522 ");
			if (ifsIndex & IfsIndexStartItuTypeA ) strcat(temp, "ItuTypeA ");
			if (ifsIndex & IfsIndexStartItuTypeB ) strcat(temp, "ItuTypeB ");
			if (ifsIndex & IfsIndexStartItuTypeC ) strcat(temp, "ItuTypeC ");
			if (ifsIndex & IfsIndexStartItuTypeD ) strcat(temp, "ItuTypeD ");
			if (ifsIndex & IfsIndexStartItuTypeE ) strcat(temp, "ItuTypeE ");
			if (ifsIndex & IfsIndexStartAncillary ) strcat(temp, "Ancillary ");
			if (ifsIndex & IfsIndexStartRes_FA_FE ) strcat(temp, "Res_FA_FE ");
			if (ifsIndex & IfsIndexStartDirectory ) strcat(temp, "Directory ");
#endif
    }

    return temp;
}



void h264_CountIndexes(ullong ifsIndex)
{
	/*
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
	*/

		int i;

		for (i = 0; i < 64; i++)
		{
			ullong mask = ((ullong) 1) << i;

			if ((mask & IfsIndexPictureI) & ifsIndex )
			{
				iPictureCount++;
			}
			else if ((mask & IfsIndexPictureP) &  ifsIndex)
			{
				pPictureCount++;
			}
			else if ((mask & IfsIndexPictureB) & ifsIndex)
			{
				bPictureCount++;
			}
			else if ((mask & ifsIndex) & IfsIndexNalUnitSeqParamSet)
			{
				iSequence++;
			}
			else if (mask & IfsIndexStartVideo)
			{
				if (ifsIndex & IfsIndexStartVideo)
				{
					switch (ifsIndex & IfsIndexInfoContainsPts)
					{
					case 0:
						videoNonePts++;
						break;
					case IfsIndexInfoContainsPts:
						videoWithPts++;
						break;
					}
				}
			}
			else if (mask & ifsIndex)
				indexCounts[i]++;
		}
}


void h264_DumpIndexes(void)
{
	int i;

	printf("Occurrences  Event\n");
	printf("----------  -----\n");

	for (i = 0; i < 64; i++)
	{
		char temp[256]; // ParseWhat
		IfsHandleImpl tempHandleImpl;
		tempHandleImpl.entry.what = ((ullong) 1) << i;


		if (tempHandleImpl.entry.what & IfsIndexPictureI)
		{
			if (iPictureCount)
			{
				tempHandleImpl.entry.what = IfsIndexPictureI;
				printf("%10ld  %s frames\n", iPictureCount, h264_ParseWhat(
						&tempHandleImpl, temp, IfsIndexDumpModeDef, IfsFalse));
			}
		}
		else if (tempHandleImpl.entry.what & IfsIndexPictureP)
		{
			if (pPictureCount)
			{
				tempHandleImpl.entry.what = IfsIndexPictureP;
				printf("%10ld  %s frames\n", pPictureCount, h264_ParseWhat(
						&tempHandleImpl, temp, IfsIndexDumpModeDef, IfsFalse));
			}
		}
		else if (tempHandleImpl.entry.what & IfsIndexPictureB)
		{
			if (bPictureCount)
			{
				tempHandleImpl.entry.what = IfsIndexPictureB;
				printf("%10ld  %s frames\n", bPictureCount, h264_ParseWhat(
						&tempHandleImpl, temp, IfsIndexDumpModeDef, IfsFalse));
			}
		}
		else if (tempHandleImpl.entry.what & IfsIndexNalUnitSeqParamSet)
		{
			if (iSequence)
			{
				printf("%10ld %s\n", iSequence, h264_ParseWhat(&tempHandleImpl, temp,
						IfsIndexDumpModeDef, IfsFalse));
			}
		}
		else if (tempHandleImpl.entry.what & IfsIndexStartVideo)
		{
			if (videoNonePts)
			{
				printf("%10ld %s\n", videoNonePts, h264_ParseWhat(&tempHandleImpl,
						temp, IfsIndexDumpModeDef, IfsFalse));
			}
			if (videoWithPts)
			{
				tempHandleImpl.entry.what |= IfsIndexInfoContainsPts;
				printf("%10ld %s\n", videoWithPts, h264_ParseWhat(&tempHandleImpl,
						temp, IfsIndexDumpModeDef, IfsFalse));
			}
		}
		else  if (indexCounts[i])
		{
			 if(h264_ParseWhat(&tempHandleImpl,temp, IfsIndexDumpModeDef, IfsFalse))
				 printf("%10ld %s\n", indexCounts[i], temp);
		}
	}
}


void h264_DumpHandle(const IfsHandle ifsHandle)
{
    printf("DUMP ifsHandle->codec->h264\n");
    g_static_mutex_lock(&(ifsHandle->mutex));
    printf("ifsHandle->ifsState         %d\n", ifsHandle->ifsState); // IfsState
    g_static_mutex_unlock(&(ifsHandle->mutex));
}


