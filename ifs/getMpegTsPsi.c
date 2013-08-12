
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getMpegTsPsi.h"


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
                printf("ERROR TID %d != %d\r\n", byte, sect->tid);
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
                printf("ERROR Section len = %d\n", sect->len);
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

        if (strm->type == 0x1  ||
            strm->type == 0x2  ||
            strm->type == 0x80 ||
            strm->type == 0x1b ||
            strm->type == 0xea)
        {
            strm->videoPID = (strm->pmt[i + 1] & 0x1f) << 8;
            strm->videoPID |= strm->pmt[i + 2];

            if (strm->firstPMT == TRUE)
            {
                printf("Video PID = %4d <0x%04x>, type = 0x%02x ",
                        strm->videoPID, strm->videoPID, strm->type);
                retVal = TRUE;

                if (strm->type == 0x1)
                {
                    printf("H.261 video\n");
                }
                else if (strm->type == 0x2 ||
                         strm->type == 0x80)
                {
                    printf("H.262 video\n");
                }
                else if (strm->type == 0x1b)
                {
                    printf("H.264 video\n");
                }
                else if (strm->type == 0xea)
                {
                    printf("VC-1 video\n");
                }
            }
        }
        else if (strm->type == 0x3  ||
                 strm->type == 0x4  ||
                 strm->type == 0x81 ||
                 strm->type == 0x6  ||
                 strm->type == 0x82 ||
                 strm->type == 0x83 ||
                 strm->type == 0x84 ||
                 strm->type == 0x85 ||
                 strm->type == 0x86 ||
                 strm->type == 0xa1 ||
                 strm->type == 0xa2 ||
                 strm->type == 0x11)
        {
            strm->audioPID = (strm->pmt[i + 1] & 0x1f) << 8;
            strm->audioPID |= strm->pmt[i + 2];

            if (strm->firstPMT == TRUE)
            {
                printf("Audio PID = %4d <0x%04x>, type = 0x%02x\n",
                        strm->audioPID, strm->audioPID, strm->type);
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

    printf("\nread %d bytes to discover TS packet size...", strm->bufLen);

    for (i = 0; i < strm->bufLen; i++)
    {
        if ((strm->buffer[i + 188] = SYNC_BYTE) &&
            (strm->buffer[i + 376] = SYNC_BYTE))
        {
            strm->tsPktSize = 188;
            break;
        }
        else if ((strm->buffer[i + 192] = SYNC_BYTE) &&
                 (strm->buffer[i + 384] = SYNC_BYTE))
        {
            strm->tsPktSize = 192;
            break;
        }
        else if ((strm->buffer[i + 204] = SYNC_BYTE) &&
                 (strm->buffer[i + 408] = SYNC_BYTE))
        {
            strm->tsPktSize = 204;
            break;
        }
        else if ((strm->buffer[i + 208] = SYNC_BYTE) &&
                 (strm->buffer[i + 416] = SYNC_BYTE))
        {
            strm->tsPktSize = 208;
            break;
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
