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

#define _IFS_PARSE_C "$Rev: 141 $"

#include <string.h>

#include "ifs_h262_impl.h"
#include "ifs_h262_parse.h"
#include "ifs_utils.h"

extern ullong indexerSetting;

static IfsPid GetPid(IfsPacket * pIfsPacket)
{
    return (((IfsPid)(pIfsPacket->bytes[1] & 0x1F)) << 8)
            | pIfsPacket->bytes[2];
}

static void ParseAdaptation(IfsHandle ifsHandle, unsigned char bytes[7])
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
    //  PCR(i) = PCR_base(i) � 300 + PCR_ext(i)

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

static void ParseCode(IfsHandle ifsHandle, const unsigned char code)
{
    IfsH262Index what = 0;

    //                                     PTS?
    //
    // SLICE                     01 - AF    NO
    // RESERVED                  B0         NO
    // RESERVED                  B1         NO
    // RESERVED                  B6         NO
    // MPEG_PROGRAM_END_CODE     B9         NO
    // PACK_START_CODE           BA         NO
    // SYSTEM_HEADER_START_CODE  BB         NO
    // PROGRAM_STREAM_MAP        BC         NO
    // PRIVATE_STREAM_1          BD        Maybe
    // PADDING_STREAM            BE         NO
    // PRIVATE_STREAM_2          BF         NO
    // AUDIO                     C0 - DF   Maybe
    // VIDEO                     E0 - EF   Maybe
    // ECM_STREAM                F0         NO
    // EMM_STREAM                F1         NO
    // DSM_CC_STREAM             F2         NO
    // ISO_IEC_13522_STREAM      F3        Maybe
    // ITU_T_REC_H_222_1_TYPE_A  F4        Maybe
    // ITU_T_REC_H_222_1_TYPE_B  F5        Maybe
    // ITU_T_REC_H_222_1_TYPE_C  F6        Maybe
    // ITU_T_REC_H_222_1_TYPE_D  F7        Maybe
    // ITU_T_REC_H_222_1_TYPE_E  F8         NO
    // ANCILLARY_STREAM          F9        Maybe
    // RESERVED                  FA - FE   Maybe
    // PROGRAM_STREAM_DIRECTORY  FF         NO

    switch (code)
    {
    case 0x00: /* handled ind ParsePict() */
        break; // PICTURE_START_CODE
    case 0xB2:
        what = IfsIndexStartUserData;
        break; // USER_DATA
    case 0xB3:
        what = IfsIndexStartSeqHeader;
        break; // SEQUENCE_HEADER
    case 0xB4:
        what = IfsIndexStartSeqError;
        break; // SEQUENCE_ERROR
    case 0xB5:
        what = IfsIndexStartExtension;
        break; // EXTENSION_START
    case 0xB7:
        what = IfsIndexStartSeqEnd;
        break; // SEQUENCE_END
    case 0xB8:
        what = IfsIndexStartGroup;
        break; // GROUP_START_CODE

#ifdef DEBUG_ALL_PES_CODES

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

        if ( (0x01 <= code) && (code <= 0xAF) )
        {   what = IfsIndexStartSlice; break;} // SLICE
        if ( (0xC0 <= code) && (code <= 0xDF) )
        {   what = IfsIndexStartAudio; break;} // AUDIO
        if ( (0xFA <= code) && (code <= 0xFE) )
        {   what = IfsIndexStartRes_FA_FE; break;} // RESERVED

        if ( (0xE0 <= code) && (code <= 0xEF) )
        {   ifsHandle->extWhat = IfsIndexStartVideo; return;} // VIDEO (added later)

#endif
    }

    ifsHandle->entry.what |= what;
}

static void ParseExt(IfsHandle ifsHandle, const unsigned char ext)
{
    IfsH262Index what = IfsIndexExtReserved; // 0, 6 and B-F are RESERVED

    switch (ext)
    {
#ifdef DEBUG_ALL_PES_CODES
    case 0x1: ifsHandle->extWhat = IfsIndexExtSequence; return; // SEQUENCE_EXTENSION_ID
    case 0x8: ifsHandle->extWhat = IfsIndexExtPictCode; return; // PICTURE_CODING_EXTENSION
#else
    case 0x1:
        what = IfsIndexExtSequence;
        break; // SEQUENCE_EXTENSION_ID
    case 0x8:
        what = IfsIndexExtPictCode;
        break; // PICTURE_CODING_EXTENSION
#endif
    case 0x2:
        what = IfsIndexExtDisplay;
        break; // SEQUENCE_DISPLAY_EXTENSION_ID
    case 0x3:
        what = IfsIndexExtQuantMat;
        break; // QUANT_MATRIX_EXTENSION_ID
    case 0x4:
        what = IfsIndexExtCopyright;
        break; // COPYRIGHT_EXTENSION_ID
    case 0x5:
        what = IfsIndexExtScalable;
        break; // SEQUENCE_SCALABLE_EXTENSION_ID
    case 0x7:
    case 0x9:
    case 0xA:
        what = IfsIndexExtPictOther;
        break; // Other PICTURE_EXTENSION_IDs
    }

    ifsHandle->entry.what |= what;
}

static void ParseElementary(IfsHandle ifsHandle, const unsigned char pdeLen,
        const unsigned char bytes[])
{
    unsigned char i;

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

            ParseCode(ifsHandle, bytes[i]);

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            // GotPic0         GotExt0 GotVid0                 Initial buf[ 3]

            switch (bytes[i])
            {
            case 0x00:
                ifsHandle->ifsState = IfsStateGotPic0;
                break;
            case 0xB5:
                ifsHandle->ifsState = IfsStateGotExt0;
                break;
            default:
                ifsHandle->ifsState
                        = ((bytes[i] & 0xF0) == 0xE0 ? IfsStateGotVid0
                                : IfsStateInitial);
                break;
            }
            break;

        case IfsStateGotPic0:

            // Found 0x00 .. 0x00 0x01 0x00, skip this byte

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotPic1 buf[ 4]

            ifsHandle->ifsState = IfsStateGotPic1;
            break;

        case IfsStateGotPic1:

            // Found 0x00 .. 0x00 0x01 0x00 0xXX, this byte contains the picture coding type

            ifsHandle->entry.what |= ((bytes[i] & 0x08 ? IfsIndexStartPicture0
                    : 0) | (bytes[i] & 0x10 ? IfsIndexStartPicture1 : 0));

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 Initial buf[ 5]

            ifsHandle->ifsState = IfsStateInitial;
            break;

        case IfsStateGotExt0:

            // Found 0x00 .. 0x00 0x01 0xB5, this byte contains the extension

            ParseExt(ifsHandle, bytes[i] >> 4);

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

            ifsHandle->codec->h262->ifsPts = ((IfsPts)(bytes[i] & 0x0E)) << 29; // 32..30
            //          RILOG_INFO(" 9 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h262->ifsPts>>32),
            //                 (long)ifsHandle->codec->h262->ifsPts, IfsLongLongToString(ifsHandle->codec->h262->ifsPts));
            ifsHandle->ifsState = IfsStateGotVid6;
            break;

        case IfsStateGotVid6:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PTS [29..22]          8 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid7 buf[10]

            ifsHandle->codec->h262->ifsPts |= ((IfsPts) bytes[i]) << 22; // 29..22
            //          RILOG_INFO("10 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h262->ifsPts>>32),
            //                 (long)ifsHandle->codec->h262->ifsPts, IfsLongLongToString(ifsHandle->codec->h262->ifsPts));
            ifsHandle->ifsState = IfsStateGotVid7;
            break;

        case IfsStateGotVid7:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PTS [21..15]          7 bslbf
            //      marker_bit            1 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid8 buf[11]

            ifsHandle->codec->h262->ifsPts |= ((IfsPts)(bytes[i] & 0xFE)) << 14; // 21..15
            //          RILOG_INFO("11 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h262->ifsPts>>32),
            //                 (long)ifsHandle->codec->h262->ifsPts, IfsLongLongToString(ifsHandle->codec->h262->ifsPts));
            ifsHandle->ifsState = IfsStateGotVid8;
            break;

        case IfsStateGotVid8:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PTS [14..7]           8 bslbf
            //      marker_bit            1 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 GotVid9 buf[12]

            ifsHandle->codec->h262->ifsPts |= ((IfsPts) bytes[i]) << 7; // 14.. 7
            //          RILOG_INFO("12 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h262->ifsPts>>32),
            //                 (long)ifsHandle->codec->h262->ifsPts, IfsLongLongToString(ifsHandle->codec->h262->ifsPts));
            ifsHandle->ifsState = IfsStateGotVid9;
            break;

        case IfsStateGotVid9:

            // Found 0x00 .. 0x00 0x01 0xEX, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ this byte contains:
            //      PTS [6..0]            7 bslbf
            //      marker_bit            1 bslbf

            // next 00 next 01 next B5 next EX next 1X next 8X   else   where
            // ------- ------- ------- ------- ------- ------- ------- -------
            //                                                 Initial buf[13]

            ifsHandle->codec->h262->ifsPts |= ((IfsPts)(bytes[i] & 0xFE)) >> 1; // 6..0
            //          RILOG_INFO("13 0x%02X %08lX%08lX %s\n", bytes[i], (long)(ifsHandle->codec->h262->ifsPts>>32),
            //                 (long)ifsHandle->codec->h262->ifsPts, IfsLongLongToString(ifsHandle->codec->h262->ifsPts));
            ifsHandle->ifsState = IfsStateInitial;
            break;
        }
    }
}

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
    if (ifsHandle->codec->h262->oldTp == IFS_UNDEFINED_BYTE)
    {
        ifsHandle->codec->h262->oldTp = tpBit;
    }
    else if (ifsHandle->codec->h262->oldTp != tpBit)
    {
        ifsHandle->entry.what |= IfsIndexHeaderTpChange;
        ifsHandle->codec->h262->oldTp = tpBit;
    }

    // Payload Unit Start Indicator
    if (pusiBit)
    {
        ifsHandle->entry.what |= IfsIndexHeaderPusiBit;
    }

    // Continuity counter
    if ((ifsHandle->codec->h262->oldCc != IFS_UNDEFINED_BYTE) && (((ifsHandle->codec->h262->oldCc + 1)
            & 0x0F) != ccBits))
    {
        ifsHandle->entry.what |= IfsIndexHeaderCcError;
    }
    ifsHandle->codec->h262->oldCc = ccBits;

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
    if (ifsHandle->codec->h262->oldSc == IFS_UNDEFINED_BYTE)
    {
        ifsHandle->codec->h262->oldSc = scBits;
    }
    else if (ifsHandle->codec->h262->oldSc != scBits)
    {
        ifsHandle->entry.what |= IfsIndexHeaderScChange;
        ifsHandle->codec->h262->oldSc = scBits;
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
            ParseAdaptation(ifsHandle, &bytes[5]);

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

IfsBoolean h262_ParsePacket(IfsHandle ifsHandle, IfsPacket * pIfsPacket)
{
    if (pIfsPacket->bytes[0] == 0x47)
    {
        ifsHandle->entry.what = 0;

        if (GetPid(pIfsPacket) == ifsHandle->codec->h262->videoPid)
        {
            ParsePacket(ifsHandle, pIfsPacket->bytes);
        }
    }
    else
    {
        ifsHandle->entry.what = IfsIndexHeaderBadSync;

        // Abandon this entire packet and Reset the PES state machine

        ifsHandle->ifsState = IfsStateInitial;
    }

    return ifsHandle->entry.what & indexerSetting; // any indexed events in this packet?
}

#ifdef DEBUG_ALL_PES_CODES
static void DumpExt(IfsH262Index ifsIndex, char * temp)
{
    int extCase = ((ifsIndex & IfsIndexInfoProgSeq ? 32 : 0) |
            (ifsIndex & IfsIndexInfoStructure1 ? 16 : 0) |
            (ifsIndex & IfsIndexInfoStructure0 ? 8 : 0) |
            (ifsIndex & IfsIndexInfoProgFrame ? 4 : 0) |
            (ifsIndex & IfsIndexInfoTopFirst ? 2 : 0) |
            (ifsIndex & IfsIndexInfoRepeatFirst ? 1 : 0));

    switch (extCase)
    {
        case 8: strcat(temp, "iT" ); break;
        case 16: strcat(temp, "iB" ); break;
        case 24: strcat(temp, "iBT" ); break;
        case 26: strcat(temp, "iTB" ); break;
        case 28: strcat(temp, "pBT" ); break;
        case 29: strcat(temp, "pBTB"); break;
        case 30: strcat(temp, "pTB" ); break;
        case 31: strcat(temp, "pTBT"); break;
        case 60: strcat(temp, "P" ); break;
        case 61: strcat(temp, "PP" ); break;
        case 63: strcat(temp, "PPP" ); break;
        default: sprintf(&temp[strlen(temp)], "BAD %2d", extCase);
    }
}
#endif

char * h262_ParseWhat(IfsHandle ifsHandle, char * temp,
        const IfsIndexDumpMode ifsIndexDumpMode, const IfsBoolean dumpPcrAndPts)
{
    IfsH262Index ifsIndex = ifsHandle->entry.what;

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
                (void) IfsLongLongToString(ifsHandle->codec->h262->ifsPcr,
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

        //  if (ifsIndex & IfsIndexStartPicture   ) strcat(temp, "StartPicture    "); logs I, P or B instead (see below)
        if (ifsIndex & IfsIndexStartUserData)
            strcat(temp, "StartUserData   ");
        if (ifsIndex & IfsIndexStartSeqHeader)
            strcat(temp, "StartSeqHeader  ");
        if (ifsIndex & IfsIndexStartSeqError)
            strcat(temp, "StartSeqError   ");
        if (ifsIndex & IfsIndexStartExtension)
            strcat(temp, "StartExtension  ");
        if (ifsIndex & IfsIndexStartSeqEnd)
            strcat(temp, "StartSeqEnd     ");
        if (ifsIndex & IfsIndexStartGroup)
            strcat(temp, "StartGroup      ");

        if (ifsIndex & IfsIndexExtReserved)
            strcat(temp, "ExtReserved   ");
        if (ifsIndex & IfsIndexExtSequence)
        {
            strcat(temp, "ExtSequence   ");
#ifdef DEBUG_ALL_PES_CODES
            strcat(temp, ifsIndex & IfsIndexInfoProgSeq ? "p" : "i");
#endif
        }
        if (ifsIndex & IfsIndexExtDisplay)
            strcat(temp, "ExtDisplay    ");
        if (ifsIndex & IfsIndexExtQuantMat)
            strcat(temp, "ExtQuantMat   ");
        if (ifsIndex & IfsIndexExtCopyright)
            strcat(temp, "ExtCopyright  ");
        if (ifsIndex & IfsIndexExtScalable)
            strcat(temp, "ExtScalable   ");
        if (ifsIndex & IfsIndexExtPictOther)
            strcat(temp, "ExtPictOther  ");
        if (ifsIndex & IfsIndexExtPictCode)
        {
            strcat(temp, "ExtPictCode   ");
#ifdef DEBUG_ALL_PES_CODES
            DumpExt(ifsIndex, temp);
#endif
        }

#ifdef DEBUG_ALL_PES_CODES

        if (ifsIndex & IfsIndexStartSlice ) strcat(temp, "StartSlice      ");
        if (ifsIndex & IfsIndexStartReservedB0) strcat(temp, "StartReservedB0 ");
        if (ifsIndex & IfsIndexStartReservedB1) strcat(temp, "StartReservedB1 ");
        if (ifsIndex & IfsIndexStartReservedB6) strcat(temp, "StartReservedB6 ");
        if (ifsIndex & IfsIndexStartMpegEnd ) strcat(temp, "StartMpegEnd    ");
        if (ifsIndex & IfsIndexStartPack ) strcat(temp, "StartPack       ");
        if (ifsIndex & IfsIndexStartSysHeader ) strcat(temp, "StartSysHeader  ");
        if (ifsIndex & IfsIndexStartProgramMap) strcat(temp, "StartProgramMap ");
        if (ifsIndex & IfsIndexStartPrivate1 ) strcat(temp, "StartPrivate1   ");
        if (ifsIndex & IfsIndexStartPadding ) strcat(temp, "StartPadding    ");
        if (ifsIndex & IfsIndexStartPrivate2 ) strcat(temp, "StartPrivate2   ");
        if (ifsIndex & IfsIndexStartAudio ) strcat(temp, "StartAudio      ");
        if (ifsIndex & IfsIndexStartVideo )
        {
            if (ifsIndex & IfsIndexInfoContainsPts)
            {
                if (dumpPcrAndPts)
                {
                    strcat(temp, "StartVideo(");
                    (void)IfsLongLongToString(ifsHandle->codec->h262->ifsPts, &temp[strlen(temp)]);
                    strcat(temp, ") ");
                }
                else strcat(temp, "StartVideo(PTS) ");
            }
            else strcat(temp, "StartVideo      ");
        }
        if (ifsIndex & IfsIndexStartEcm ) strcat(temp, "StartEcm        ");
        if (ifsIndex & IfsIndexStartEmm ) strcat(temp, "StartEmm        ");
        if (ifsIndex & IfsIndexStartDsmCc ) strcat(temp, "StartDsmCc      ");
        if (ifsIndex & IfsIndexStart13522 ) strcat(temp, "Start13522      ");
        if (ifsIndex & IfsIndexStartItuTypeA ) strcat(temp, "StartItuTypeA   ");
        if (ifsIndex & IfsIndexStartItuTypeB ) strcat(temp, "StartItuTypeB   ");
        if (ifsIndex & IfsIndexStartItuTypeC ) strcat(temp, "StartItuTypeC   ");
        if (ifsIndex & IfsIndexStartItuTypeD ) strcat(temp, "StartItuTypeD   ");
        if (ifsIndex & IfsIndexStartItuTypeE ) strcat(temp, "StartItuTypeE   ");
        if (ifsIndex & IfsIndexStartAncillary ) strcat(temp, "StartAncillary  ");
        if (ifsIndex & IfsIndexStartRes_FA_FE ) strcat(temp, "StartRes_FA_FE  ");
        if (ifsIndex & IfsIndexStartDirectory ) strcat(temp, "StartDirectory  ");

#endif

        switch (ifsIndex & IfsIndexStartPicture)
        {
        case IfsIndexStartPicture0:
            strcat(temp, "StartIpicture   ");
            break;
        case IfsIndexStartPicture1:
            strcat(temp, "StartPpicture   ");
            break;
        case IfsIndexStartPicture:
            strcat(temp, "StartBpicture   ");
            break;
        }
    }
    else if (ifsIndexDumpMode == IfsIndexDumpModeDef)
    {
        //ifsIndex &= indexerSetting; // Clean up the output in this mode

        switch (ifsIndex & IfsIndexStartPicture)
        {
        case IfsIndexStartPicture0:
            strcat(temp, "I ");
            break;
        case IfsIndexStartPicture1:
            strcat(temp, "P ");
            break;
        case IfsIndexStartPicture:
            strcat(temp, "B ");
            break;
        default:
            strcat(temp, "  ");
            break;
        }

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
                (void) IfsLongLongToString(ifsHandle->codec->h262->ifsPcr,
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

        //  if (ifsIndex & IfsIndexStartPicture   ) strcat(temp, "StartPicture    "); logs I, P or B instead (see above)
        if (ifsIndex & IfsIndexStartUserData)
            strcat(temp, "UserData ");
        if (ifsIndex & IfsIndexStartSeqHeader)
            strcat(temp, "SeqHeader ");
        if (ifsIndex & IfsIndexStartSeqError)
            strcat(temp, "SeqError ");
        if (ifsIndex & IfsIndexStartExtension)
            strcat(temp, "Ext ");
        if (ifsIndex & IfsIndexStartSeqEnd)
            strcat(temp, "SeqEnd ");
        if (ifsIndex & IfsIndexStartGroup)
            strcat(temp, "Gop ");

        if (ifsIndex & IfsIndexExtReserved)
            strcat(temp, "Reserved ");
        if (ifsIndex & IfsIndexExtSequence)
        {
#ifdef DEBUG_ALL_PES_CODES
            strcat(temp, ifsIndex & IfsIndexInfoProgSeq ? "Seq(p) " : "Seq(i) ");
#else
            strcat(temp, "Seq ");
#endif
        }
        if (ifsIndex & IfsIndexExtDisplay)
            strcat(temp, "Display ");
        if (ifsIndex & IfsIndexExtQuantMat)
            strcat(temp, "QuantMat ");
        if (ifsIndex & IfsIndexExtCopyright)
            strcat(temp, "Copyright ");
        if (ifsIndex & IfsIndexExtScalable)
            strcat(temp, "Scalable ");
        if (ifsIndex & IfsIndexExtPictCode)
        {
#ifdef DEBUG_ALL_PES_CODES
            strcat(temp, "PictCode(");
            DumpExt(ifsIndex, temp);
            strcat(temp, ") ");
#else
            strcat(temp, "PictCode ");
#endif
        }

        if (ifsIndex & IfsIndexExtPictOther)
            strcat(temp, "PictOther ");

#ifdef DEBUG_ALL_PES_CODES

        if (ifsIndex & IfsIndexStartSlice ) strcat(temp, "Slice ");
        if (ifsIndex & IfsIndexStartReservedB0) strcat(temp, "ReservedB0 ");
        if (ifsIndex & IfsIndexStartReservedB1) strcat(temp, "ReservedB1 ");
        if (ifsIndex & IfsIndexStartReservedB6) strcat(temp, "ReservedB6 ");
        if (ifsIndex & IfsIndexStartMpegEnd ) strcat(temp, "MpegEnd ");
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
                    (void)IfsLongLongToString(ifsHandle->codec->h262->ifsPts, &temp[strlen(temp)]);
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

static unsigned long indexCounts[64] = { 0 };
static unsigned long iPictureCount = 0;
static unsigned long pPictureCount = 0;
static unsigned long bPictureCount = 0;
static unsigned long iSequence = 0;
static unsigned long pSequence = 0;
static unsigned long pictCodeCounts[64] = { 0 };
static unsigned long videoNonePts = 0;
static unsigned long videoWithPts = 0;

void h262_CountIndexes(ullong index)
{
    IfsH262Index ifsIndex = (IfsH262Index)index;
    // Special cases:
    //
    //  IfsIndexStartPicture
    //
    //  16  0000000000018000 = PdeBit   StartIpicture
    //  17  0000000000028000 = PdeBit   StartPpicture
    //  18  0000000000038000 = PdeBit   StartBpicture
    //
    //  IfsIndexExtSequence
    //      IfsIndexInfoProgSeq
    //
    //  25  0000000002808000 = PdeBit   StartExtension  ExtSequence   i
    //  26  0400000002808000 = PdeBit   StartExtension  ExtSequence   p
    //
    //  IfsIndexExtPictCode
    //      IfsIndexInfoProgFrame
    //      IfsIndexInfoStructure
    //      IfsIndexInfoProgRep
    //
    //  32  0000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  0
    //  33  0800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  1
    //  34  1000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  2
    //  35  1800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  3
    //  36  2000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  4
    //  37  2800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  5
    //  38  3000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  6
    //  39  3800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  7
    //  40  4000000080808000 = PdeBit   StartExtension  ExtPictCode   iT
    //  41  4800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD  9
    //  42  5000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 10
    //  43  5800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 11
    //  44  6000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 12
    //  45  6800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 13
    //  46  7000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 14
    //  47  7800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 15
    //  48  8000000080808000 = PdeBit   StartExtension  ExtPictCode   iB
    //  49  8800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 17
    //  50  9000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 18
    //  51  9800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 19
    //  52  A000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 20
    //  53  A800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 21
    //  54  B000000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 22
    //  55  B800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 23
    //  56  C000000080808000 = PdeBit   StartExtension  ExtPictCode   iBT
    //  57  C800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 25
    //  58  D000000080808000 = PdeBit   StartExtension  ExtPictCode   iTB
    //  59  D800000080808000 = PdeBit   StartExtension  ExtPictCode   BAD 27
    //  60  E000000080808000 = PdeBit   StartExtension  ExtPictCode   pBT
    //  61  E800000080808000 = PdeBit   StartExtension  ExtPictCode   pBTB
    //  62  F000000080808000 = PdeBit   StartExtension  ExtPictCode   pTB
    //  63  F800000080808000 = PdeBit   StartExtension  ExtPictCode   pTBT
    //
    //  IfsIndexStartVideo
    //      IfsIndexInfoContainsPts
    //
    //  76  0000100000008000 = PdeBit   StartVideo
    //  77  0200100000008000 = PdeBit   StartVideo(0)
    //
    // Normal cases:
    //
    //   0  0000000000000100 = BadSync
    //   1  0000000000000200 = TpChange
    //   2  0000000000000400 = PusiBit
    //   3  0000000000000800 = TeiBit
    //
    //   4  0000000000001000 = CcError
    //   5  0000000000002000 = ScChange
    //   6  0000000000004000 = AfeBit
    //  15  0000000000008000 = PdeBit
    //
    //   7  0000000000004001 = AfeBit   AdaptAfeeBit
    //   8  0000000000004002 = AfeBit   AdaptTpdeBit
    //   9  0000000000004004 = AfeBit   AdaptSpeBit
    //  10  0000000000004008 = AfeBit   AdaptOpcreBit
    //
    //  11  0000000000004010 = AfeBit   AdaptPcreBit(0)
    //  12  0000000000004020 = AfeBit   AdaptEspChange
    //  13  0000000000004040 = AfeBit   AdaptRaiBit
    //  14  0000000000004080 = AfeBit   AdaptDiBit
    //
    //  19  0000000000048000 = PdeBit   StartUserData
    //  20  0000000000088000 = PdeBit   StartSeqHeader
    //
    //  21  0000000000108000 = PdeBit   StartSeqError
    //  22  0000000000208000 = PdeBit   StartSeqEnd
    //  23  0000000000408000 = PdeBit   StartGroup
    //  24  0000000001808000 = PdeBit   StartExtension  ExtReserved
    //  27  0000000004808000 = PdeBit   StartExtension  ExtDisplay
    //  28  0000000008808000 = PdeBit   StartExtension  ExtQuantMat
    //  29  0000000010808000 = PdeBit   StartExtension  ExtCopyright
    //  30  0000000020808000 = PdeBit   StartExtension  ExtScalable
    //  31  0000000040808000 = PdeBit   StartExtension  ExtPictOther
    //
    //  64  0000000100008000 = PdeBit   StartSlice
    //  65  0000000200008000 = PdeBit   StartReservedB0
    //  66  0000000400008000 = PdeBit   StartReservedB1
    //  67  0000000800008000 = PdeBit   StartReservedB6
    //
    //  68  0000001000008000 = PdeBit   StartMpegEnd
    //  69  0000002000008000 = PdeBit   StartPack
    //  70  0000004000008000 = PdeBit   StartSysHeader
    //  71  0000008000008000 = PdeBit   StartProgramMap
    //
    //  72  0000010000008000 = PdeBit   StartPrivate1
    //  73  0000020000008000 = PdeBit   StartPadding
    //  74  0000040000008000 = PdeBit   StartPrivate2
    //  75  0000080000008000 = PdeBit   StartAudio
    //
    //  78  0000200000008000 = PdeBit   StartEcm
    //  79  0000400000008000 = PdeBit   StartEmm
    //  80  0000800000008000 = PdeBit   StartDsmCc
    //
    //  81  0001000000008000 = PdeBit   Start13522
    //  82  0002000000008000 = PdeBit   StartItuTypeA
    //  83  0004000000008000 = PdeBit   StartItuTypeB
    //  84  0008000000008000 = PdeBit   StartItuTypeC
    //
    //  85  0010000000008000 = PdeBit   StartItuTypeD
    //  86  0020000000008000 = PdeBit   StartItuTypeE
    //  87  0040000000008000 = PdeBit   StartAncillary
    //  88  0080000000008000 = PdeBit   StartRes_FA_FE
    //
    //  89  0100000000008000 = PdeBit   StartDirectory

    int i;

    for (i = 0; i < 64; i++)
    {
        IfsH262Index mask = ((IfsH262Index) 1) << i;

        if (mask & IfsIndexStartPicture0)
        {
            switch (ifsIndex & IfsIndexStartPicture)
            {
            case IfsIndexStartPicture0:
                iPictureCount++;
                break;
            case IfsIndexStartPicture1:
                pPictureCount++;
                break;
            case IfsIndexStartPicture:
                bPictureCount++;
                break;
            }
        }
        else if (mask & IfsIndexExtSequence)
        {
            if (ifsIndex & IfsIndexExtSequence)
            {
                switch (ifsIndex & IfsIndexInfoProgSeq)
                {
                case 0:
                    iSequence++;
                    break;
                case IfsIndexInfoProgSeq:
                    pSequence++;
                    break;
                }
            }
        }
        else if (mask & IfsIndexExtPictCode)
        {
            if (ifsIndex & IfsIndexExtPictCode)
            {
                size_t code = ((ifsIndex & IfsIndexInfoProgSeq ? 32 : 0)
                        | (ifsIndex & IfsIndexInfoStructure1 ? 16 : 0)
                        | (ifsIndex & IfsIndexInfoStructure0 ? 8 : 0)
                        | (ifsIndex & IfsIndexInfoProgFrame ? 4 : 0)
                        | (ifsIndex & IfsIndexInfoTopFirst ? 2 : 0) | (ifsIndex
                        & IfsIndexInfoRepeatFirst ? 1 : 0));
                pictCodeCounts[code]++;
            }
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
        else if (mask & (IfsIndexStartPicture1 | IfsIndexInfoProgSeq
                | IfsIndexInfoProgFrame | IfsIndexInfoStructure
                | IfsIndexInfoProgRep | IfsIndexInfoContainsPts))
        {
            // Do nothing
        }
        else if (mask & ifsIndex)
            indexCounts[i]++;
    }
}

void h262_DumpIndexes(void)
{
    int i;

    printf("Occurances  Event\n");
    printf("----------  -----\n");

    for (i = 0; i < 64; i++)
    {
        char temp[256] = { 0 }; // ParseWhat
        IfsH262CodecImpl localH262Codec = { 0 };
        IfsHandleImpl tempHandleImpl;

        tempHandleImpl.codec = (IfsCodec*)&localH262Codec;
        g_static_mutex_init(&(tempHandleImpl.mutex));
        tempHandleImpl.entry.what = ((IfsH262Index) 1) << i;

        if (IfsSetCodec(&tempHandleImpl, IfsCodecTypeH262)
                != IfsReturnCodeNoErrorReported)
        {
            printf("Problems setting ifs codec\n");
            return;
        }

        if (tempHandleImpl.entry.what & IfsIndexStartPicture0)
        {
            if (iPictureCount)
            {
                tempHandleImpl.entry.what = IfsIndexStartPicture0;
                printf("%10ld  %s frame\n", iPictureCount,
                    h262_ParseWhat(
                        &tempHandleImpl, temp, IfsIndexDumpModeDef, IfsFalse));
            }
            if (pPictureCount)
            {
                tempHandleImpl.entry.what = IfsIndexStartPicture1;
                printf("%10ld  %s frame\n", pPictureCount,
                    h262_ParseWhat(
                        &tempHandleImpl, temp, IfsIndexDumpModeDef, IfsFalse));
            }
            if (bPictureCount)
            {
                tempHandleImpl.entry.what = IfsIndexStartPicture;
                printf("%10ld  %s frame\n", bPictureCount,
                    h262_ParseWhat(
                        &tempHandleImpl, temp, IfsIndexDumpModeDef, IfsFalse));
            }
        }
        else if (tempHandleImpl.entry.what & IfsIndexExtSequence)
        {
            if (iSequence)
            {
                printf("%10ld%s\n", iSequence,
                    h262_ParseWhat(&tempHandleImpl, temp,
                        IfsIndexDumpModeDef, IfsFalse));
            }
            if (pSequence)
            {
                tempHandleImpl.entry.what |= IfsIndexInfoProgSeq;
                printf("%10ld%s\n", pSequence,
                    h262_ParseWhat(&tempHandleImpl, temp,
                        IfsIndexDumpModeDef, IfsFalse));
            }
        }
        else if (tempHandleImpl.entry.what & IfsIndexExtPictCode)
        {
            IfsH262Index j;

            for (j = 0; j < 64; j++)
            {
                if (pictCodeCounts[j])
                {
                    tempHandleImpl.entry.what = (IfsIndexExtPictCode
                            | (j & 32 ? IfsIndexInfoProgSeq : 0)
                            | (j & 16 ? IfsIndexInfoStructure1 : 0)
                            | (j & 8 ? IfsIndexInfoStructure0 : 0)
                            | (j & 4 ? IfsIndexInfoProgFrame : 0)
                            | (j & 2 ? IfsIndexInfoTopFirst : 0)
                            | (j & 1 ? IfsIndexInfoRepeatFirst : 0));

                    printf("%10ld%s\n", pictCodeCounts[j],
                        h262_ParseWhat(
                            &tempHandleImpl, temp, IfsIndexDumpModeDef,
                            IfsFalse));
                }
            }
        }
        else if (tempHandleImpl.entry.what & IfsIndexStartVideo)
        {
            if (videoNonePts)
            {
                printf("%10ld%s\n", videoNonePts,
                    h262_ParseWhat(&tempHandleImpl,
                        temp, IfsIndexDumpModeDef, IfsFalse));
            }
            if (videoWithPts)
            {
                tempHandleImpl.entry.what |= IfsIndexInfoContainsPts;
                printf("%10ld%s\n", videoWithPts,
                    h262_ParseWhat(&tempHandleImpl,
                        temp, IfsIndexDumpModeDef, IfsFalse));
            }
        }
        else if (tempHandleImpl.entry.what & (IfsIndexStartPicture1
                | IfsIndexInfoProgSeq | IfsIndexInfoProgFrame
                | IfsIndexInfoStructure | IfsIndexInfoProgRep
                | IfsIndexInfoContainsPts))
        {
            // Do nothing
        }
        else if (indexCounts[i])
            printf("%10ld%s\n", indexCounts[i],
                h262_ParseWhat(&tempHandleImpl,
                    temp, IfsIndexDumpModeDef, IfsFalse));
    }
}

void h262_DumpHandle(const IfsHandle ifsHandle)
{
    char temp[32];

    printf("DUMP ifsHandle->codec->h262\n");
    g_static_mutex_lock(&(ifsHandle->mutex));
    printf("ifsHandle->codec->ifsPcr     %s\n", IfsLongLongToString //
            (ifsHandle->codec->h262->ifsPcr, temp)); // IfsPcr
    printf("ifsHandle->codec->ifsPts     %s\n", IfsLongLongToString //
            (ifsHandle->codec->h262->ifsPts, temp)); // IfsPts
    printf("ifsHandle->codec->videoPid         %d\n", ifsHandle->codec->h262->videoPid); // IfsPid
    printf("ifsHandle->codec->audioPid         %d\n", ifsHandle->codec->h262->audioPid); // IfsPid
    printf("ifsHandle->ifsState         %d\n", ifsHandle->ifsState); // IfsState
    printf("ifsHandle->codec->oldEsp    %d\n", ifsHandle->codec->h262->oldEsp); // unsigned char
    printf("ifsHandle->codec->oldSc     %d\n", ifsHandle->codec->h262->oldSc); // unsigned char
    printf("ifsHandle->codec->oldTp     %d\n", ifsHandle->codec->h262->oldTp); // unsigned char
    printf("ifsHandle->codec->oldCc     %d\n", ifsHandle->codec->h262->oldCc); // unsigned char
    g_static_mutex_unlock(&(ifsHandle->mutex));
}

