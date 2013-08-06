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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include "RemapImpl.h"

#include "ifs_file.h"
#include "ifs_impl.h"
#include "ifs_h262_parse.h"
#include "ifs_h264_parse.h"
#include "ifs_h265_parse.h"
#include "ifs_utils.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

static char inputFile[MAX_PATH];
static RemapPid thePmtPid = REMAP_UNDEFINED_PID;
static RemapPid theVideoPid = REMAP_UNDEFINED_PID;
static IfsHandle ifsHandle;
static IfsBoolean isAnMpegFile = IfsFalse;
static IfsCodecType gCodecType = IfsCodecTypeError;

void DumpBadCodes(void);
void DumpBadCodes(void)
{
    printf("                                        top  repeat  Reasons\n");
    printf("BAD  progressive  picture  progressive field  first   it is \n");
    printf("case   sequence  structure    frame    first  field    BAD  \n");
    printf("---- ----------- --------- ----------- ----- ------  -------\n");
    printf("   0      0          0          0        0      0    3      \n");
    printf("   1      0          0          0        0      1    3, 7   \n");
    printf("   2      0          0          0        1      0    3      \n");
    printf("   3      0          0          0        1      1    3, 7   \n");
    printf("   4      0          0          1        0      0    3      \n");
    printf("   5      0          0          1        0      1    3      \n");
    printf("   6      0          0          1        1      0    3      \n");
    printf("   7      0          0          1        1      1    3      \n");
    printf("   9      0          1          0        0      1    6, 7   \n");
    printf("  10      0          1          0        1      0    4      \n");
    printf("  11      0          1          0        1      1    4, 6, 7\n");
    printf("  12      0          1          1        0      0    2      \n");
    printf("  13      0          1          1        0      1    2, 6   \n");
    printf("  14      0          1          1        1      0    2, 4   \n");
    printf("  15      0          1          1        1      1    2, 4, 6\n");
    printf("  17      0          2          0        0      1    6, 7   \n");
    printf("  18      0          2          0        1      0    4      \n");
    printf("  19      0          2          0        1      1    4, 6, 7\n");
    printf("  20      0          2          1        0      0    2      \n");
    printf("  21      0          2          1        0      1    2, 6   \n");
    printf("  22      0          2          1        1      0    2, 4   \n");
    printf("  23      0          2          1        1      1    2, 4, 6\n");
    printf("  25      0          3          0        0      1    7      \n");
    printf("  27      0          3          0        1      1    7      \n");
    printf("  32      1          0          0        0      0    1, 3   \n");
    printf("  33      1          0          0        0      1    1, 3   \n");
    printf("  34      1          0          0        1      0    1, 3   \n");
    printf("  35      1          0          0        1      1    1, 3   \n");
    printf("  36      1          0          1        0      0    1, 3   \n");
    printf("  37      1          0          1        0      1    1, 3   \n");
    printf("  38      1          0          1        1      0    1, 3   \n");
    printf("  39      1          0          1        1      1    1, 3   \n");
    printf("  40      1          1          0        0      0    1      \n");
    printf("  41      1          1          0        0      1    1, 6   \n");
    printf("  42      1          1          0        1      0    1      \n");
    printf("  43      1          1          0        1      1    1, 6   \n");
    printf("  44      1          1          1        0      0    1      \n");
    printf("  45      1          1          1        0      1    1, 6   \n");
    printf("  46      1          1          1        1      0    1      \n");
    printf("  47      1          1          1        1      1    1, 6   \n");
    printf("  48      1          2          0        0      0    1      \n");
    printf("  49      1          2          0        0      1    1, 6   \n");
    printf("  50      1          2          0        1      0    1      \n");
    printf("  51      1          2          0        1      1    1, 6   \n");
    printf("  52      1          2          1        0      0    1      \n");
    printf("  53      1          2          1        0      1    1, 6   \n");
    printf("  54      1          2          1        1      0    1      \n");
    printf("  55      1          2          1        1      1    1, 6   \n");
    printf("  56      1          3          0        0      0    1      \n");
    printf("  57      1          3          0        0      1    1      \n");
    printf("  58      1          3          0        1      0    1      \n");
    printf("  59      1          3          0        1      1    1      \n");
    printf("  62      1          3          1        1      0    5      \n");
    printf("---- ----------- --------- ----------- ----- ------  -------\n");
    printf("\n");
    printf("Reasons:\n");
    printf("\n");
    printf(" 1   When progressive_sequence is 1 the coded video sequence\n");
    printf("     contains only progressive frame pictures\n");
    printf("\n");
    printf(" 2   When progressive_sequence is 0 field pictures must be\n");
    printf("     interlaced\n");
    printf("\n");
    printf(" 3   Reserved picture_structure value\n");
    printf("\n");
    printf(" 4   If progressive_sequence is 0 then top_field_first must\n");
    printf("     be 0 for all field encoded pictures\n");
    printf("\n");
    printf(" 5   If progressive_sequence is 1 then top_field_first 1,\n");
    printf("     repeat_first_field 0 is not a legal combination\n");
    printf("\n");
    printf(" 6   repeat_first_field shall be 0 for all field encoded\n");
    printf("     pictures\n");
    printf("\n");
    printf(" 7   If progressive_sequence and progressive_frame are 0 then\n");
    printf("     repeat_first_frame shall be 0\n");
}

static IfsBoolean ProcessArguments(int argc, char *argv[]) // returns IfsTrue = failure, IfsFalse = success
{
    const time_t now = time(NULL);

    printf("\nNdxDump.exe, version %d, at %ld %s\n", INTF_RELEASE_VERSION, now,
            asctime(gmtime(&now)));

    if (argc == 2)
    {
        printf("Processing two args...\n");
        strcpy(inputFile, argv[1]);
        if (strstr(argv[1], ".mpg"))
        {
            // .mpg or .ts means we're looking at MPEG2 PS or TS encapsulated
            gCodecType = IfsCodecTypeH262;
            printf("Opening mpeg file...\n");
            FILE * const pInFile = fopen(inputFile, "rb");

            size_t i = 2;
            RemapHandle remapHandle;
            NumPackets nextNumPackets;
            RemapPacket packets[2][16];
            RemapPacket * pointers[2][16];
            NumPackets prevNumPackets;
            RemapPacket ** ppPrevPointers;

            if (pInFile == NULL)
            {
                printf("Unable to open input file: %s\n", inputFile);
                return IfsTrue;
            }

            isAnMpegFile = IfsTrue;

            if (RemapOpen(&remapHandle) != RemapReturnCodeNoErrorReported)
            {
                printf("Problems remapping mpeg file\n");
                return IfsTrue;
            }

            while ((nextNumPackets = fread(&packets[i & 1][0],
                    IFS_TRANSPORT_PACKET_SIZE, 16, pInFile)) != 0)
            {
                if (RemapAndFilter(remapHandle, NULL, nextNumPackets,
                        &packets[i & 1][0], &pointers[i & 1][0],
                        &prevNumPackets, &ppPrevPointers)
                        != RemapReturnCodeNoErrorReported)
                {
                    printf("Problems remapping and filtering mpeg packets\n");
                    return IfsTrue;
                }
                i++;
            }
            if (RemapAndFilter(remapHandle, NULL, 0, NULL, NULL,
                    &prevNumPackets, &ppPrevPointers)
                    != RemapReturnCodeNoErrorReported)
            {
                printf("Problems remapping and filtering prev mpeg file\n");
                return IfsTrue;
            }
            if (remapHandle->nextPat == 0)
                printf("Could not find a valid PAT in the file\n");
            else if (remapHandle->nextPat == 1)
                printf("PMT PID (%4d) found\n", thePmtPid
                        = remapHandle->pSavePat[0].pid);
            else
            {
                int selection;
                printf(
                        "This is an MPTS, please select the desired PMT PID:\n\n");
                for (i = 0; i < remapHandle->nextPat; i++)
                    printf(
                            "   Selection %d will select program %d, PMT PID %d (0x%X)\n",
                            i, remapHandle->pSavePat[i].prog,
                            remapHandle->pSavePat[i].pid,
                            remapHandle->pSavePat[i].pid);
                printf("\n   Selection ");
                if (scanf("%d", &selection) != 1)
                {
                    printf("ERROR -- scanf returned wrong number of parsed items!\n");
                }
                else
                {
                    thePmtPid = remapHandle->pSavePat[selection].pid; 
                    printf("PMT PID set to %4d\n", thePmtPid);
                }
            }

            if (RemapClose(remapHandle, &prevNumPackets, &ppPrevPointers)
                    != RemapReturnCodeNoErrorReported)
            {
                printf("Problems closing file after remapping\n");
                return IfsTrue;
            }

            if (thePmtPid != REMAP_UNDEFINED_PID)
            {
                rewind(pInFile);

                if (RemapOpen(&remapHandle) != RemapReturnCodeNoErrorReported)
                {
                    printf("Remap after rewind failed\n");
                    return IfsTrue;
                }
                if (RemapPmts(remapHandle, thePmtPid)
                        != RemapReturnCodeNoErrorReported)
                {
                    printf("Remap of PMTs failed after rewind\n");
                    return IfsTrue;
                }
                while ((nextNumPackets = fread(&packets[i & 1][0],
                        IFS_TRANSPORT_PACKET_SIZE, 16, pInFile)) != 0)
                {
                    if (RemapAndFilter(remapHandle, NULL, nextNumPackets,
                            &packets[i & 1][0], &pointers[i & 1][0],
                            &prevNumPackets, &ppPrevPointers)
                            != RemapReturnCodeNoErrorReported)
                    {
                        printf("Remap and filtering of rewind packets failed\n");
                        return IfsTrue;
                    }
                    i++;
                }
                if (RemapAndFilter(remapHandle, NULL, 0, NULL, NULL,
                        &prevNumPackets, &ppPrevPointers)
                        != RemapReturnCodeNoErrorReported)
                {
                    printf("Remap and filter of rewind file failed\n");
                    return IfsTrue;
                }

                theVideoPid = remapHandle->videoPid;

                if (RemapClose(remapHandle, &prevNumPackets, &ppPrevPointers)
                        != RemapReturnCodeNoErrorReported)
                {
                    printf("Problems closing file after rewind remap\n");
                    return IfsTrue;
                }
                if (theVideoPid == REMAP_UNDEFINED_PID)
                    printf("Could not find a valid PMT in the file\n");
                else
                {
                    NumPackets numPackets;
                    IfsPacket ifsPacket;

                    printf("Vid PID (%4d) found\n", theVideoPid);

                    rewind(pInFile);

                    if (IfsOpenWriter(".", NULL, 0, &ifsHandle)
                            != IfsReturnCodeNoErrorReported)
                    {
                        printf("Problems opening ifs writer\n");
                        return IfsTrue;
                    }

                    // TODO: set the correct container!
                    if (IfsSetContainer(ifsHandle, IfsContainerTypeMpeg2Ts)
                            != IfsReturnCodeNoErrorReported)
                    {
                        printf("Problems setting ifs container\n");
                        return IfsTrue;
                    }

                    if (IfsSetCodec(ifsHandle, gCodecType)
                            != IfsReturnCodeNoErrorReported)
                    {
                        printf("Problems setting ifs codec\n");
                        return IfsTrue;
                    }

                    IfsSetMode(IfsIndexDumpModeOff, IfsH262IndexerSettingDefault);

                    if (IfsStart(ifsHandle, theVideoPid, 0)
                            != IfsReturnCodeNoErrorReported)
                    {
                        printf("Problems starting ifs writer\n");
                        return IfsTrue;
                    }
                    i = 0;

                    // Pretend each packet contains 1 second of data...
                    while ((numPackets = fread(&ifsPacket,
                            IFS_TRANSPORT_PACKET_SIZE, 1, pInFile)) != 0)
                        if (IfsWrite(ifsHandle, ++i * NSEC_PER_SEC, numPackets,
                                &ifsPacket) != IfsReturnCodeNoErrorReported)
                        {
                            printf("Problems writing packet\n");
                            return IfsTrue;
                        }
                    fclose(pInFile);
                    if (IfsSeekToPacket(ifsHandle, 0, NULL)
                            != IfsReturnCodeNoErrorReported)
                    {
                        printf("Problems seeking to packet\n");
                        return IfsTrue;
                    }
                    if (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry),
                            ifsHandle->pNdex) != sizeof(IfsIndexEntry))
                    {
                        printf("Problems reading entry\n");
                        return IfsTrue;
                    }
                    printf("Successfully opened file...\n");
                    return IfsFalse;
                }
            }

            fclose(pInFile);
        }
        else if (strstr(argv[1], ".ndx"))
        {
            printf("Processing index file...");
            // Simulate (part of) an IfsOpen command...

            ifsHandle = g_try_malloc0(sizeof(IfsHandleImpl));
            if (ifsHandle == NULL)
            {
                printf("Could not allocate memory\n");
                return IfsTrue;
            }

            ifsHandle->pNdex = fopen(inputFile, "rb");

            if (ifsHandle->pNdex)
            {
                printf("The input file name and path will be '%s'\n\n",
                        inputFile);
                if (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry),
                        ifsHandle->pNdex) == sizeof(IfsIndexEntry))
                {
                    return IfsFalse;
                }
                else
                    printf("Could not read '%s', errno %d\n\n", inputFile,
                            errno);
            }
            else
                printf("Could not open '%s', errno %d\n\n", inputFile, errno);
        }
        else
        {
            // TODO: discover the CODEC from the file!
            gCodecType = IfsCodecTypeH262;
            printf("Processing one arg...");
            IfsReturnCode ifsReturnCode;
            size_t length = strlen(inputFile);
            while (length && (inputFile[length] != '\\') && (inputFile[length]
                    != '/'))
                length--;
            if (length)
            {
                inputFile[length] = 0;
                ifsReturnCode = IfsOpenReader(inputFile,
                        &inputFile[length + 1], &ifsHandle);
            }
            else
                ifsReturnCode = IfsOpenReader(".", inputFile, &ifsHandle);

            if (ifsReturnCode == IfsReturnCodeNoErrorReported)
            {
                //IfsDumpHandle(ifsHandle);
                printf("Opened file for processing...");
                return IfsFalse;
}
            else
                printf("IfsPathNameInfo failed with error \"%s\"\n\n",
                        IfsReturnCodeToString(ifsReturnCode));
        }
    }
    else
    {
        //      DumpBadCodes();
        printf("Usage:  NdxDump <input>\n");
        printf("        Where <input> is one of the following three cases:\n");
        printf("          1) an IFS index file such as 00000000.ndx\n");
        printf("          2) an IFS directory containing .mpg and .ndx files\n");
        printf("             such as 4B59D994_0029\n");
        printf("          3) an MPEG file such as plain.mpg\n");
    }

    printf("Returning true...");
    return IfsTrue;
}

int main(int argc, char *argv[])
{
    IfsClock duration;
    time_t seconds;
    IfsIndexEntry firstEntry, lastEntry;
    NumEntries numEntries = 0;
    void (*countIndexes)(ullong ifsIndex) = NULL;
    void (*dumpIndexes)(void) = NULL;
    char* (*parseWhat)(IfsHandle ifsHandle, char * temp,
        const IfsIndexDumpMode ifsIndexDumpMode, const IfsBoolean) = NULL;

    printf("processing cmd line args...\n");

    if (ProcessArguments(argc, argv))
        return 0;

    printf("starting analysis...\n");

    switch (ifsHandle->codecType)
    {
        case IfsCodecTypeH261:
        case IfsCodecTypeH262:
        case IfsCodecTypeH263:
            IfsSetMode(IfsIndexDumpModeDef, IfsH262IndexerSettingVerbose);
            parseWhat = ifsHandle->codec->h262->ParseWhat;
            countIndexes = ifsHandle->codec->h262->CountIndexes;
            dumpIndexes = ifsHandle->codec->h262->DumpIndexes;
            break;
        case IfsCodecTypeH264:
            IfsSetMode(IfsIndexDumpModeDef, IfsH264IndexerSettingVerbose);
            parseWhat = ifsHandle->codec->h264->ParseWhat;
            countIndexes = ifsHandle->codec->h264->CountIndexes;
            dumpIndexes = ifsHandle->codec->h264->DumpIndexes;
            break;
        case IfsCodecTypeH265:
            IfsSetMode(IfsIndexDumpModeDef, IfsH265IndexerSettingVerbose);
            parseWhat = ifsHandle->codec->h265->ParseWhat;
            countIndexes = ifsHandle->codec->h265->CountIndexes;
            dumpIndexes = ifsHandle->codec->h265->DumpIndexes;
            break;
        default:
            printf("IfsReturnCodeBadInputParameter: ifsHandle->codec "
                   "not set in line %d of %s\n", __LINE__, __FILE__);
            exit(0);
    }

    firstEntry = ifsHandle->entry;

    while (1)
    {
        if (ifsHandle->ndex)
            printf("\n%s:\n\n", ifsHandle->ndex);

        if (isAnMpegFile)
        {
            printf(
                    "Packet(where)  Index Information (what happened at that time and location)\n");
            printf(
                    "-------------  -----------------------------------------------------------\n");
            do
            {
                char temp[256]; // ParseWhat
                countIndexes(ifsHandle->entry.what);
                printf("%6ld/%6ld  %s\n", ifsHandle->entry.realWhere,
                        ifsHandle->entry.virtWhere, parseWhat(ifsHandle, temp,
                                IfsIndexDumpModeDef, IfsFalse));
                numEntries++;

            } while (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry),
                    ifsHandle->pNdex) == sizeof(IfsIndexEntry));

            printf(
                    "-------------  -----------------------------------------------------------\n");
            printf(
                    "Packet(where)  Index Information (what happened at that time and location)\n");
        }
        else
        {
            printf(
                    "Packet(where)  Date/TimeStamp(when)  Index Information (what happened at that time and location)\n");
            printf(
                    "-------------  --------------------  -----------------------------------------------------------\n");
            do
            {
                char temp1[32]; // IfsToSecs only
                char temp2[256]; // ParseWhat
                countIndexes(ifsHandle->entry.what);
                printf("%6ld/%6ld  %s  %s\n", ifsHandle->entry.realWhere,
                        ifsHandle->entry.virtWhere, IfsToSecs(
                                ifsHandle->entry.when, temp1),
                        parseWhat(ifsHandle, temp2, IfsIndexDumpModeDef,
                                IfsFalse));
                numEntries++;

            } while (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry),
                    ifsHandle->pNdex) == sizeof(IfsIndexEntry));

            printf(
                    "-------------  --------------------  -----------------------------------------------------------\n");
            printf(
                    "Packet(where)  Date/TimeStamp(when)  Index Information (what happened at that time and location)\n");
        }

        if (!ifsHandle->curFileNumber)
            break;

        if (ifsHandle->curFileNumber >= ifsHandle->endFileNumber)
            break;

        if (IfsOpenActualFiles(ifsHandle, ifsHandle->curFileNumber + 1, "rb+"))
            break;

        if (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry), ifsHandle->pNdex)
                != sizeof(IfsIndexEntry))
            break;
    }

    lastEntry = ifsHandle->entry;
    duration = lastEntry.when - firstEntry.when;

    printf("\n");
    dumpIndexes();
    printf("\n");

    if (isAnMpegFile)
    {
        printf("First packet indexed %7ld/%7ld\n", firstEntry.realWhere,
                firstEntry.virtWhere);
        printf("Last packet indexed  %7ld/%7ld\n", lastEntry.realWhere,
                lastEntry.virtWhere);
        printf("Number of entries %10ld\n", numEntries);
        printf("Rate of indexing  %20.9f packets/entry\n",
                ((lastEntry.virtWhere - firstEntry.virtWhere) * 1.0)
                        / (numEntries * 1.0));
#if 0
        printf("I frame rate      %20.9f frames/I-frame\n",
                (iPictureCount ? ((iPictureCount + pPictureCount
                        + bPictureCount) * 1.0) / (iPictureCount * 1.0) : 0.0));
#endif
    }
    else
    {
        char temp[32]; // IfsToSecs only
        seconds = firstEntry.when / NSEC_PER_SEC;
        printf("Date/Time Start   %s %s", IfsToSecs(firstEntry.when, temp),
                asctime(gmtime(&seconds)));
        seconds = lastEntry.when / NSEC_PER_SEC;
        printf("Date/Time Ended   %s %s", IfsToSecs(lastEntry.when, temp),
                asctime(gmtime(&seconds)));
        printf("Recording Lasted  %s seconds\n", IfsToSecs(duration, temp));
        printf("First packet indexed %7ld/%7ld\n", firstEntry.realWhere,
                firstEntry.virtWhere);
        printf("Last packet indexed  %7ld/%7ld\n", lastEntry.realWhere,
                lastEntry.virtWhere);
        printf("Number of entries %10ld\n", numEntries);
        printf("Rate of indexing  %20.9f entries/second\n", (numEntries * 1.0)
                / ((duration * 1.0) / (NSEC_PER_SEC * 1.0)));
        printf("                  %20.9f packets/entry\n",
                ((lastEntry.virtWhere - firstEntry.virtWhere) * 1.0)
                        / (numEntries * 1.0));
#if 0
        printf("I frame rate      %20.9f frames/I-frame\n",
                (iPictureCount ? ((iPictureCount + pPictureCount
                        + bPictureCount) * 1.0) / (iPictureCount * 1.0) : 0.0));
#endif
    }
    return IfsClose(ifsHandle);
}
