
/*
 * Copyright (C) 2013  Cable Television Laboratories, Inc.
 * Contact: http://www.cablelabs.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getMpegTsPsi.h"
#undef DEBUG_LOGS
#define OTHER   0
#define VIDEO   1
#define AUDIO   2




int
getPmtStreamType(struct stream* strm)
{
    switch(strm->type)
    {
        case 0x00: return OTHER;     // "Reserved";
        case 0x01:
            strm->codecType = IfsCodecTypeH261;
            return VIDEO;            // "MPEG-1 Video";
        case 0x02:
            strm->codecType = IfsCodecTypeH262;
            return VIDEO;            // "MPEG-2 Video";
        case 0x03: return AUDIO;     // "MPEG-1 Audio";
        case 0x04: return AUDIO;     // "MPEG-2 Audio";
        case 0x05: return OTHER;     // "ISO 13818-1 private sections";
        case 0x06: return OTHER;     // "ISO 13818-1 PES private data";
        case 0x07: return OTHER;     // "ISO 13522 MHEG";
        case 0x08: return OTHER;     // "ISO 13818-1 DSM-CC";
        case 0x09: return OTHER;     // "ISO 13818-1 auxiliary";
        case 0x0A: return OTHER;     // "ISO 13818-6 multi-protocol encap";
        case 0x0B: return OTHER;     // "ISO 13818-6 DSM-CC U-N msgs";
        case 0x0C: return OTHER;     // "ISO 13818-6 stream descriptors";
        case 0x0D: return OTHER;     // "ISO 13818-6 sections";
        case 0x0E: return OTHER;     // "ISO 13818-1 auxiliary";
        case 0x0F: return AUDIO;     // "MPEG-2 AAC Audio";
        case 0x10:
            strm->codecType = IfsCodecTypeH264;
            return VIDEO;            // "MPEG-4 Video";
        case 0x11: return AUDIO;     // "MPEG-4 LATM AAC Audio";
        case 0x12: return AUDIO;     // "MPEG-4 generic";
        case 0x13: return OTHER;     // "ISO 14496-1 SL-packetized";
        case 0x14: return OTHER;     // "ISO 13818-6 Synch'd Download Protocol";
        case 0x1b:
            strm->codecType = IfsCodecTypeH264;
            return VIDEO;            // "H.264 Video";
        case 0x1c: return AUDIO;     // "MPEG-4 Audio";
        case 0x80:
            strm->codecType = IfsCodecTypeH262;
            return VIDEO;            // "DigiCipher II Video";
        case 0x81: return AUDIO;     // "A52/AC-3 Audio";
        case 0x82: return AUDIO;     // "HDMV DTS Audio";
        case 0x83: return AUDIO;     // "LPCM Audio";
        case 0x84: return AUDIO;     // "SDDS Audio";
        case 0x85: return OTHER;     // "ATSC Program ID";
        case 0x86: return AUDIO;     // "DTS-HD Audio";
        case 0x87: return AUDIO;     // "E-AC-3 Audio";
        case 0x8a: return AUDIO;     // "DTS Audio";
        case 0x91: return AUDIO;     // "A52b/AC-3 Audio";
        case 0x92: return OTHER;     // "DVD_SPU vls Subtitle";
        case 0x94: return AUDIO;     // "SDDS Audio";
        case 0xa0:
            strm->codecType = IfsCodecTypeError;
            return VIDEO;            // "MSCODEC Video";
        case 0xa1: return AUDIO;     // "MSCODEC Audio";
        case 0xa2: return AUDIO;     // "Generic Audio";
        case 0xdb:
            strm->codecType = IfsCodecTypeH264;
            return VIDEO;            // "iOS H.264 Video";
        case 0xea:
            strm->codecType = IfsCodecTypeError;
            return VIDEO;            // "Private ES (VC-1)";
        default:
            // 0x80-0xFF: return "User Private";
            // 0x15-0x7F: return "ISO 13818-1 Reserved";
            return OTHER;
    }
}

static const char*
getPmtStreamTypeStr(unsigned char type)
{
    switch(type)
    {
        case 0x00: return "Reserved";
        case 0x01: return "MPEG-1 Video";
        case 0x02: return "MPEG-2 Video";
        case 0x03: return "MPEG-1 Audio";
        case 0x04: return "MPEG-2 Audio";
        case 0x05: return "ISO 13818-1 private sections";
        case 0x06: return "ISO 13818-1 PES private data";
        case 0x07: return "ISO 13522 MHEG";
        case 0x08: return "ISO 13818-1 DSM-CC";
        case 0x09: return "ISO 13818-1 auxiliary";
        case 0x0A: return "ISO 13818-6 multi-protocol encap";
        case 0x0B: return "ISO 13818-6 DSM-CC U-N msgs";
        case 0x0C: return "ISO 13818-6 stream descriptors";
        case 0x0D: return "ISO 13818-6 sections";
        case 0x0E: return "ISO 13818-1 auxiliary";
        case 0x0F: return "MPEG-2 AAC Audio";
        case 0x10: return "MPEG-4 Video";
        case 0x11: return "MPEG-4 LATM AAC Audio";
        case 0x12: return "MPEG-4 generic";
        case 0x13: return "ISO 14496-1 SL-packetized";
        case 0x14: return "ISO 13818-6 Synchronized Download Protocol";
        case 0x1b: return "H.264 Video";
        case 0x1c: return "MPEG-4 Audio";
        case 0x80: return "DigiCipher II Video";
        case 0x81: return "A52/AC-3 Audio";
        case 0x82: return "HDMV DTS Audio";
        case 0x83: return "LPCM Audio";
        case 0x84: return "SDDS Audio";
        case 0x85: return "ATSC Program ID";
        case 0x86: return "DTS-HD Audio";
        case 0x87: return "E-AC-3 Audio";
        case 0x8a: return "DTS Audio";
        case 0x91: return "A52b/AC-3 Audio";
        case 0x92: return "DVD_SPU vls Subtitle";
        case 0x94: return "SDDS Audio";
        case 0xa0: return "MSCODEC Video";
        case 0xa1: return "MSCODEC Audio";
        case 0xa2: return "Generic Audio";
        case 0xdb: return "iOS H.264 Video";
        case 0xea: return "Private ES (VC-1)";
        default:
            if (0x80 <= type && type <= 0xFF)
                return "User Private";
            if (0x15 <= type && type <= 0x7F)
                return "ISO 13818-1 Reserved";
            else
                return "Unknown";
    }
}

static void
handleSectionStart(struct section *sect, struct packet *pkt, unsigned char byte)
{
    pkt->pointer = byte;
    sect->start = FALSE;
    sect->tid = 0;
    sect->number = 0;
    sect->lastNumber = 0;

    if (pkt->pointer == 0)
        sect->lengthState = SECT_LEN_STATES;
}

static int
getSectionLength(struct section *sect, unsigned char byte)
{
    --sect->lengthState;

    switch (sect->lengthState)
    {
        case 2:
            if (byte != sect->tid)   // verify Table ID
            {
                // some streams have unknown tables, so just ignore silently...
#ifdef DEBUG_LOGS
                printf("\nERROR TID %d != %d", byte, sect->tid);
#endif
                sect->lengthState = 0;
            }
            break;

        case 1:
            sect->len = (byte & 0xf) << 8;
            break;

        case 0:
            sect->len |= byte;
#ifdef DEBUG_LOGS
            printf("\nSection len = %d\n", sect->len);
#endif
            if (sect->len > 1021)
            {
                printf("\nERROR Section len = %d\n", sect->len);
                sect->lengthState = 0;
            }

            return TRUE;
    }

    return FALSE;
}

static void
handlePmtSectionState(struct section *sect, unsigned char byte)
{
    --sect->len;
    if(sect->prog_info_len && (sect->state == 1))
    	;//printf("program sec length: %d\n", sect->prog_info_len);
    else
    --sect->state;

    switch (sect->state)
    {
        case 10:     // Prog Num high 8 bits
            break;
        case 9:     // Prog Num low 8 bits
            break;
        case 8:     // RES / VER / CNI
            break;
        case 7:     // Sect Num
            sect->number = byte;

            if (sect->number == 0)
                sect->offset = 0;
            break;
        case 6:     // Last Sect Num
            sect->lastNumber = byte;
            break;
        case 5:     // PCR PID high 5 bits
            break;
        case 4:     // PCR PID low 8 bits
            break;
        case 3:     // Program Info Len high 4 bits
        	sect->prog_info_len = (byte &0xf) << 8;
            break;
        case 2:     // Program Info Len low 8 bits
        	sect->prog_info_len |= byte;
           	if(sect->prog_info_len)
            	sect->prog_info_len--;
           	else
           	{
           		sect->state = 0;
           		sect->read = TRUE;
           	}
           	break;
        case 1:
          	if(sect->prog_info_len)
           		sect->prog_info_len--;
             break;
        case 0:
    		sect->read = TRUE;
    		break;
    }
}

static void
handlePatSectionState(struct section *sect, unsigned char byte)
{
    --sect->len;
    --sect->state;

    switch (sect->state)
    {
        case 4:     // TSID HIGH
            break;
        case 3:     // TSID LOW
            break;
        case 2:     // RES / VER / CNI
            break;
        case 1:     // Sect Num
            sect->number = byte;

            if (sect->number == 0)
            {
                sect->offset = 0;
            }
            break;
        case 0:     // Last Sect Num
            sect->lastNumber = byte;
            sect->read = TRUE;
            break;
    }
}

static int
parsePatSection(struct stream *strm, struct section *sect) 
{
    int i, retVal = FALSE;

    for (i = 0; i < (sect->offset - 4); i += 4)
    {
        strm->program = strm->pat[i] << 8;
        strm->program |= strm->pat[i + 1];

        if (strm->firstPAT == TRUE)
        {
            strm->pmtPID = (strm->pat[i + 2] & 0x1f) << 8;
            strm->pmtPID |= strm->pat[i + 3];
            printf("Program Number = %d (0x%04x), PMT PID = %d (0x%04x)\n",
                    strm->program, strm->program, strm->pmtPID, strm->pmtPID);
            retVal = TRUE;
        }
    }

    strm->firstPAT = FALSE;
    //printf("PAT found done !\n");
    strm->flags &= ~(FLAGS_FOUND_PAT_SEC);	// clear the section flag
    strm->flags |= FLAGS_FOUND_PAT;			// set the pat found flag
    return retVal;
}

static int
handlePatSection(struct stream *strm, struct section *sect, struct packet *pkt) 
{
    int retVal = FALSE;
    unsigned short i, bytesLeft = (strm->bufLen - strm->pos);
    unsigned short sectBytes = (bytesLeft >= sect->len)? sect->len : bytesLeft;

#ifdef DEBUG_LOGS
    printf("bytesLeft:%d = (strm->bufLen:%d - strm->pos:%d);\n",
           bytesLeft, strm->bufLen, strm->pos);
    printf("sectBytes(%d) = (bytesLeft >= sect->len:%d)? sect->len:bytesLeft;\n",
            sectBytes, sect->len);
#endif
    // if the TS packet doesn't have the required section bytes...
    if (pkt->length <= sectBytes)
    {
        sectBytes = pkt->length;      // only read what's left
#ifdef DEBUG_LOGS
        printf("sectBytes:%d = pkt->length:%d\n",
               sectBytes, pkt->length);
#endif
    }

    for (i = 0; i < sectBytes; i++)
    {
#ifdef DEBUG_LOGS
        printf("PAT[%d] = 0x%02x\n", sect->offset, strm->buffer[strm->pos+i]);
#endif
        strm->pat[sect->offset++] = strm->buffer[strm->pos + i];
    }

    // Adjust indexes...
    strm->pos += sectBytes;
    sect->len -= sectBytes;
    pkt->length -= sectBytes;

    // we've completed reading...
    if (sect->len <= 0)
    {
        sect->read = FALSE;

        if (sect->number == sect->lastNumber)
        {
            retVal = parsePatSection(strm, sect);
        }
        else
        {
#ifdef DEBUG_LOGS
            printf("sect->number:%d != sect->lastNumber:%d\n",
                   sect->number, sect->lastNumber);
#endif
        }
    }

    return retVal;
}

static int
parsePmtSection(struct stream *strm, struct section *sect) 
{
    int i, retVal = FALSE;
    int es_info_length = 0;

    for (i = 0; i < (sect->offset - 4); )
    {
        strm->type = strm->pmt[i];
        retVal = getPmtStreamType(strm);

        if (VIDEO == retVal)
        {
            strm->videoPID = (strm->pmt[i + 1] & 0x1f) << 8;
            strm->videoPID |= strm->pmt[i + 2];

#ifndef DEBUG_LOGS
            if (strm->firstPMT)
#endif
            {
                printf("\nVideo PID = %4d <0x%04x>, type = 0x%02x %s\n",
                       strm->videoPID, strm->videoPID, strm->type,
                       getPmtStreamTypeStr(strm->type));
            }
        }
        else if (AUDIO == retVal)
        {
            strm->audioPID = (strm->pmt[i + 1] & 0x1f) << 8;
            strm->audioPID |= strm->pmt[i + 2];

#ifndef DEBUG_LOGS
            if (strm->firstPMT)
#endif
            {
                printf("Audio PID = %4d <0x%04x>, type = 0x%02x %s\n",
                       strm->audioPID, strm->audioPID, strm->type,
                       getPmtStreamTypeStr(strm->type));
            }
        }
        else
        {
#ifndef DEBUG_LOGS
            if (strm->firstPMT)
#endif
            {
                printf("\n      PID = <0x%04x>, type = 0x%02x %s\n",
                      ((strm->pmt[i+1] & 0x1f) << 8) | strm->pmt[i+2], strm->type,
                       getPmtStreamTypeStr(strm->type));
            }
        }

        //Now read the es_info_length and skip over
        es_info_length = (strm->pmt[i+3] & 0xf << 8) | (strm->pmt[i+4]);
        i += (es_info_length + 5); // Skip over 4 bytes of stream type, pid and length and the length itself
    }

    strm->firstPMT = FALSE;
    //printf("PAT found done !\n");
    strm->flags &= ~(FLAGS_FOUND_PMT_SEC);	// clear the section flag
    strm->flags |= FLAGS_FOUND_PMT;			// set the pmt found flag
    return retVal;
}

static int
handlePmtSection(struct stream *strm, struct section *sect, struct packet *pkt) 
{
    int retVal = FALSE;
    unsigned short i, bytesLeft = (strm->bufLen - strm->pos);
    unsigned short sectBytes = (bytesLeft >= sect->len)? sect->len : bytesLeft;

    // if the TS packet doesn't have the required section bytes...
    if (pkt->length <= sectBytes)
        sectBytes = pkt->length;      // only read what's left

    for (i = 0; i < sectBytes; i++)
    {
#ifdef DEBUG_LOGS
        printf("PMT[%d]: 0x%02x\n", sect->offset, strm->buffer[strm->pos+i]);
#endif
        strm->pmt[sect->offset++] = strm->buffer[strm->pos + i];
    }

    // Adjust indexes...
    strm->pos += sectBytes;
    sect->len -= sectBytes;
    pkt->length -= sectBytes;

    if (sect->len <= 0)
    {
        sect->read = FALSE;

        if (sect->number == sect->lastNumber)
            retVal = parsePmtSection(strm, sect);
    }

    return retVal;
}

static void
handlePktHeader(struct stream *strm, struct section *sect, struct packet *pkt) 
{
    unsigned char temp;

    --pkt->headerState;

    switch (pkt->headerState)
    {
        case 2:     // Flags and high 5 bits of PID
            temp = strm->buffer[strm->pos];
            pkt->pusi = (temp >> 6) & 0x1;
            pkt->pid = (temp & 0x1f) << 8;
            break;
        case 1:     // low 8 bits of PID
            temp = strm->buffer[strm->pos];
            pkt->pid |= temp;
#ifdef DEBUG_LOGS
            printf("\nPID = %4x", pkt->pid);
#endif
            break;
        case 0:     // TS header complete, next: process section...
            if (pkt->pusi == 1)
            {
                sect->lengthState = 0;
                sect->state = 0;
                sect->read = FALSE;
                sect->start = ((pkt->pid == strm->patPID) ||
                               (pkt->pid == strm->pmtPID));
            }
            break;
    }
}

void
decode_mp2ts(struct stream* strm, unsigned char **pat, unsigned char **pmt) 
{
    struct packet packet = { 0 };
    struct section section = { 0 };

    if (strm->offset > 0) // 192 byte packets
    {
        packet.length = strm->tsPktSize-1;      // TS_PKT_SIZE - sync byte
        packet.headerState = 3;                 // TS_HDR_SIZE - sync byte
    }
    else if (strm->buffer[0] == SYNC_BYTE || strm->buffer[4] == SYNC_BYTE)
    {
        packet.length = strm->tsPktSize-1;      // TS_PKT_SIZE - sync byte
        packet.headerState = 3;                 // TS_HDR_SIZE - sync byte
    }
    else
    {
        fprintf(stderr, "SYNC ERROR\n");
        printf("\n\n");
        exit(-5);
    }

    for (strm->pos = 1; strm->pos < strm->bufLen; strm->pos++)
    {
        if (packet.headerState != 0)
        {
            --packet.length;
            handlePktHeader(strm, &section, &packet);
        }
        else if (packet.pid == strm->patPID)
        {
            if (section.read == TRUE)
            {
                if (handlePatSection(strm, &section, &packet))
                {
                    //printf("PAT section: %d\n", section.lastNumber);
                    strm->flags |= FLAGS_FOUND_PAT_SEC;
                    if (NULL != pat)
                    {
                        int len = (strm->tsPktSize * (1 + section.lastNumber));

                        *pat = (unsigned char*)malloc(len);
                        memcpy(*pat, strm->buffer, len);
                    }
                }
            }
            else
            {
                --packet.length;

                if (section.start == TRUE)
                {
                    // initialize the section data
                    handleSectionStart(&section, &packet,
                                       strm->buffer[strm->pos]);
                }
                else if (packet.pointer != 0)
                {
                    // if there is a pointer field value, add one to the length
                    // state machine to account for the pointer field itself...
                    if (--packet.pointer == 0)
                        section.lengthState = SECT_LEN_STATES+1;
                }
                else if (section.lengthState != 0)
                {
                    // if we retrieved the length, prepare for the next state...
                    if (getSectionLength(&section, strm->buffer[strm->pos]))
                         section.state = PAT_SECT_STATES;
                }
                else if (section.state != 0)
                {
                    handlePatSectionState(&section, strm->buffer[strm->pos]);
                }
            }
        }
        else if (packet.pid == strm->pmtPID)
        {
            if (section.read == TRUE)
            {
                if (handlePmtSection(strm, &section, &packet))
                {
                    //printf("PMT section: %d\n", section.lastNumber);
                    strm->flags |= FLAGS_FOUND_PMT_SEC;

                    if (NULL != pmt)
                    {
                        int len = (strm->tsPktSize * (1 + section.lastNumber));

                        *pmt = (unsigned char*)malloc(len);
                        memcpy(*pmt, strm->buffer, len);
                    }
                }
            }
            else
            {
                --packet.length;

                if (section.start == TRUE)
                {
                    // initialize the section data
                    handleSectionStart(&section, &packet,
                                       strm->buffer[strm->pos]);
                    section.tid = PMT_TABLE_ID;
                }
                else if (packet.pointer != 0)
                {
                    // if there is a pointer field value, add one to the length
                    // state machine to account for the pointer field itself...
                    if (--packet.pointer == 0)
                        section.lengthState = SECT_LEN_STATES+1;
                }
                else if (section.lengthState != 0)
                {
                    // if we retrieved the length, prepare for the next state...
                    if (getSectionLength(&section, strm->buffer[strm->pos]))
                         section.state = PMT_SECT_STATES;
                }
                else if (section.state != 0)
                {
                    handlePmtSectionState(&section, strm->buffer[strm->pos]);
                }
            }
        }
        else
        {
            // we don't currently handle this OID or Table ID
            --packet.length;
        }
    }
}

void
discoverPktSize(struct stream* strm)
{
    unsigned int i;

    printf("\nread %d bytes to discover TS packet size...\n ", strm->bufLen);
    strm->tsPktSize = 0;

    for (i = 0; i < strm->bufLen; i++)
    {
        if (strm->buffer[i] == SYNC_BYTE)
        {
		if ((strm->buffer[i + 188] == SYNC_BYTE) &&
		    (strm->buffer[i + 376] == SYNC_BYTE))
		{
		    //printf("\nfound 3 consecutive SYNCs at 188 byte separation");
		    strm->tsPktSize = 188;
		    break;
		}
		else if ((strm->buffer[i + 192] == SYNC_BYTE) &&
		         (strm->buffer[i + 384] == SYNC_BYTE))
		{
		    //printf("\nfound 3 consecutive SYNCs at 192 byte separation");
		    strm->tsPktSize = 192;
		    break;
		}
		else if ((strm->buffer[i + 204] == SYNC_BYTE) &&
		        (strm->buffer[i + 408] == SYNC_BYTE))
		{
		    //printf("\nfound 3 consecutive SYNCs at 204 byte separation");
		    strm->tsPktSize = 204;
		    break;
		}
		else if ((strm->buffer[i + 208] == SYNC_BYTE) &&
		        (strm->buffer[i + 416] == SYNC_BYTE))
		{
		    //printf("\nfound 3 consecutive SYNCs at 208 byte separation");
		    strm->tsPktSize = 208;
		    break;
		}
        }
        else if ((strm->buffer[i+0] == 0) &&
                 (strm->buffer[i+1] == 0) &&
                 (strm->buffer[i+2] == 1))
        {
            int leave = TRUE;
            switch (strm->buffer[i+3])
            {
                case 0xb0:
                case 0xb1:
                    printf("\nfound reserved start code");
                    break;
                case 0xb2:
                    printf("\nfound user data start code");
                    break;
                case 0xb3:
                case 0xb4:
                case 0xb5:
                case 0xb7:
                    printf("\nfound sequence start code");
                    break;
                case 0xb8:
                    printf("\nfound GOP start code");
                    break;
                case 0xb9:
                    printf("\nfound program end start code");
                    break;
                case 0xba:
                    printf("\nfound pack header start code");
                    break;
                case 0xbb:
                    printf("\nfound system header start code");
                    break;
                case 0xbc:
                    printf("\nfound PS map start code");
                    break;
                case 0xbd:
                case 0xbf:
                    printf("\nfound private start code");
                    break;
                case 0xbe:
                    printf("\nfound padding start code");
                    break;
                default:
                    leave = FALSE;
                    break;
            }

            if (TRUE == leave)
            {
                printf(" at offset[%d]\n", strm->offset);
                break;
            }
        }

        strm->offset++;
    }
    giStreamPacketSize = strm->tsPktSize;
}

#ifdef PSI_TEST_STAND_ALONE
static void
usage(char *pgm)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s <infile> <prog #> \n", pgm);
}

int
main(int argc, char **argv)
{
    struct stream stream = { { 0 } };
    FILE *fp = NULL;

    if (argc != 2)
    {
        usage(argv[0]);
        exit(-1);
    }

    // open the MPEG file to parse...
    if (0 == (fp = fopen(argv[1], "rb")))
    {
        fprintf(stderr, "Cannot open mpeg file <%s>\n", argv[1]);
        exit(-2);
    }

    printf("\n%s:\n\n", argv[1]);
    stream.firstPAT = TRUE;
    stream.firstPMT = TRUE;
    stream.patPID = 0;
    stream.pmtPID = 0x1FFF;
    stream.videoPID = 0x1FFF;
    stream.audioPID = 0x1FFF;

    if (NULL == (stream.buffer = (unsigned char*)malloc(4*MAX_TS_PKT_SIZE)))
    {
        fclose(fp);
        fprintf(stderr, "could not allocate memory for stream\n");
        exit(-3);
    }

    stream.bufLen = fread(stream.buffer, 1, 4 * MAX_TS_PKT_SIZE, fp);
    stream.offset = 0;
    discoverPktSize(&stream);

    if (0 == stream.tsPktSize)
    {
        fclose(fp);
        fprintf(stderr, "mpeg file <%s> is not a transport stream\n", argv[1]);
        free(stream.buffer);
        exit(-4);
    }

    printf("\npacket size: %d, offset: %d\n", stream.tsPktSize, stream.offset);
    rewind(fp);

    // read and toss the bytes preceeding the SYNC
    if (stream.offset > 0)
        fread(stream.buffer, 1, stream.offset, fp);

    while (!feof(fp))
    {
        stream.bufLen = fread(stream.buffer, 1, 4*stream.tsPktSize, fp);
        decode_mp2ts(&stream, NULL, NULL);
    }

    printf("\n\n");
    free(stream.buffer);
    fclose(fp);
    return 0;
}
#endif
