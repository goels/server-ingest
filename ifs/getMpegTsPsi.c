
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getMpegTsPsi.h"

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
            strm->codecType = IfsCodecTypeH263;
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

static char*
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
                //printf("\nERROR TID %d != %d", byte, sect->tid);
                sect->lengthState = 0;
            }
            break;

        case 1:
            sect->len = (byte & 0xf) << 8;
            break;

        case 0:
            sect->len |= byte;
            // printf("Section len = %d\n", sect->len);

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
    --sect->state;

    switch (sect->state)
    {
        case 8:     // Prog Num high 8 bits
            break;
        case 7:     // Prog Num low 8 bits
            break;
        case 6:     // RES / VER / CNI
            break;
        case 5:     // Sect Num
            sect->number = byte;

            if (sect->number == 0)
                sect->offset = 0;
            break;
        case 4:     // Last Sect Num
            sect->lastNumber = byte;
            break;
        case 3:     // PCR PID high 5 bits
            break;
        case 2:     // PCR PID low 8 bits
            break;
        case 1:     // Program Info Len high 4 bits
            break;
        case 0:     // Program Info Len low 8 bits
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
    return retVal;
}

static int
handlePatSection(struct stream *strm, struct section *sect, struct packet *pkt) 
{
    int retVal = FALSE;
    unsigned short i, bytesLeft = (strm->bufLen - strm->pos);
    unsigned short sectBytes = (bytesLeft >= sect->len)? sect->len : bytesLeft;

    // if the TS packet doesn't have the required section bytes...
    if (pkt->length <= sectBytes)
        sectBytes = pkt->length;      // only read what's left

    for (i = 0; i < sectBytes; i++)
    {
        //printf("PAT[%d] = 0x%02x\n", sect->offset, strm->buffer[strm->pos+i]);
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
            retVal = parsePatSection(strm, sect);
    }

    return retVal;
}

static int
parsePmtSection(struct stream *strm, struct section *sect) 
{
    int i, retVal = FALSE;

    for (i = 0; i < (sect->offset - 4); i += 5)
    {
        strm->type = strm->pmt[i];
        retVal = getPmtStreamType(strm);

        if (VIDEO == retVal)
        {
            strm->videoPID = (strm->pmt[i + 1] & 0x1f) << 8;
            strm->videoPID |= strm->pmt[i + 2];

            if (strm->firstPMT)
            {
                printf("\nVideo PID = %4d <0x%04x>, type = 0x%02x %s",
                       strm->videoPID, strm->videoPID, strm->type,
                       getPmtStreamTypeStr(strm->type));
            }
        }
        else if (AUDIO == retVal)
        {
            strm->audioPID = (strm->pmt[i + 1] & 0x1f) << 8;
            strm->audioPID |= strm->pmt[i + 2];

            if (strm->firstPMT)
            {
                printf("\nAudio PID = %4d <0x%04x>, type = 0x%02x %s",
                       strm->audioPID, strm->audioPID, strm->type,
                       getPmtStreamTypeStr(strm->type));
            }
        }
        else
        {
            if (strm->firstPMT)
            {
                printf("\n      PID = <0x%04x>, type = 0x%02x %s",
                      ((strm->pmt[i+1] & 0x1f) << 8) | strm->pmt[i+2], strm->type,
                       getPmtStreamTypeStr(strm->type));
            }
        }
    }

    strm->firstPMT = FALSE;
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
        //printf("PMT[%d]: 0x%02x\n", sect->offset, strm->buffer[strm->pos+i]);
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
            //printf(" PID=%4x", pkt->pid);
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

    if (strm->buffer[0] == SYNC_BYTE)
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
                    if (--packet.pointer == 0)
                        section.lengthState = SECT_LEN_STATES;
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
                    if (--packet.pointer == 0)
                        section.lengthState = SECT_LEN_STATES;
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

    printf("\nread %d bytes to discover TS packet size...   ", strm->bufLen);
    strm->tsPktSize = 0;

    for (i = 0; i < strm->bufLen; i++)
    {
        if (strm->buffer[i] == SYNC_BYTE)
        {
            if ((strm->buffer[i + 188] = SYNC_BYTE) &&
                (strm->buffer[i + 376] = SYNC_BYTE))
            {
                printf("\nfound 3 consecutive SYNCs at 188 byte separation");
                strm->tsPktSize = 188;
                break;
            }
            else if ((strm->buffer[i + 192] = SYNC_BYTE) &&
                     (strm->buffer[i + 384] = SYNC_BYTE))
            {
                printf("\nfound 3 consecutive SYNCs at 192 byte separation");
                strm->tsPktSize = 192;
                break;
            }
            else if ((strm->buffer[i + 204] = SYNC_BYTE) &&
                     (strm->buffer[i + 408] = SYNC_BYTE))
            {
                printf("\nfound 3 consecutive SYNCs at 204 byte separation");
                strm->tsPktSize = 204;
                break;
            }
            else if ((strm->buffer[i + 208] = SYNC_BYTE) &&
                     (strm->buffer[i + 416] = SYNC_BYTE))
            {
                printf("\nfound 3 consecutive SYNCs at 208 byte separation");
                strm->tsPktSize = 208;
                break;
            }
        }

        strm->offset++;
    }
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