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

#define _IFS_OPERATIONS_C "$Rev: 141 $"

#define FLUSH_ALL_WRITES
//#define DEBUG_CONVERT_APPEND

#ifndef WIN32
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <glib.h>
#ifdef STAND_ALONE
#include "my_ri_log.h"
#else
#include "ri_log.h"
#include "ri_config.h"
#endif

#include "ifs_file.h"
#include "ifs_parse.h"
#include "ifs_utils.h"

extern log4c_category_t * ifs_RILogCategory;
#define RILOG_CATEGORY ifs_RILogCategory

extern IfsIndexDumpMode indexDumpMode;
extern IfsIndex whatAll;
extern unsigned indexCase;
extern unsigned ifsFileChunkSize;

// IfsHandle operations:

IfsReturnCode IfsWrite(IfsHandle ifsHandle, // Input (must be a writer)
        IfsClock ifsClock, // Input, in nanoseconds
        NumPackets numPackets, // Input
        IfsPacket * pData // Input
)
{
    NumPackets i;

    if (ifsHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ifsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    g_static_mutex_lock(&(ifsHandle->mutex));

    if (ifsHandle->isReading)
    {
        RILOG_ERROR("IfsReturnCodeMustBeAnIfsWriter: in line %d of %s\n",
                __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeMustBeAnIfsWriter;
    }
    if (numPackets && (pData == NULL))
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: numPackets && (pData == NULL) in line %d of %s\n",
                __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeBadInputParameter;
    }

    ifsHandle->entry.when = ifsClock; // nanoseconds

    if (ifsHandle->maxSize) // in seconds, not 0 then this is a circular buffer
    {
        if (!ifsHandle->begClock) // beg not set yet (first write)
        {
            ifsHandle->begClock = ifsClock; // set beg clock in nanoseconds
            char temp[32];
            RILOG_INFO( "set begClock to %s at line %d of %s\n",
                IfsToSecs(ifsClock, temp), __LINE__, __FILE__);
            ifsHandle->nxtClock = ifsClock + (ifsFileChunkSize * NSEC_PER_SEC); // in nanoseconds
        }
        else // not first write
        {
            if (ifsClock >= ifsHandle->nxtClock) // Need to move to the next file
            {
                char tmp[32];
                RILOG_INFO("move to next file (beg:%lu, nxt:%lu) at %s"
                    "at line %d of %s\n",
                    ifsHandle->begFileNumber, ifsHandle->endFileNumber,
                    IfsToSecs(ifsClock, tmp), __LINE__, __FILE__);
                IfsReturnCode ifsReturnCode;

                char * mpeg = NULL;
                char * ndex = NULL;

                ifsHandle->nxtClock += (ifsFileChunkSize * NSEC_PER_SEC); // in nanoseconds
                ifsHandle->realLoc = 0;

                ifsHandle->endFileNumber++;

                g_static_mutex_unlock(&(ifsHandle->mutex));
                ifsReturnCode = IfsOpenActualFiles(ifsHandle,
                        ifsHandle->endFileNumber, "wb+");
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                    return ifsReturnCode;

                g_static_mutex_lock(&(ifsHandle->mutex));

                ifsReturnCode = GenerateFileNames(ifsHandle,
                        ifsHandle->begFileNumber, &mpeg, &ndex);
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                {
                    g_static_mutex_unlock(&(ifsHandle->mutex));
                    return ifsReturnCode;
                }

                while (((ifsClock - ifsHandle->begClock) / NSEC_PER_SEC)
                        >= (ifsHandle->maxSize + ifsFileChunkSize))
                {
                    FILE * pNdex;
                    struct stat statBuffer;
                    IfsIndexEntry ifsIndexEntry;

                    // Delete the oldest files
                    RILOG_INFO("deleting begFileNumber:%lu at line %d of %s\n",
                        ifsHandle->begFileNumber, __LINE__, __FILE__);

                    if (stat(mpeg, &statBuffer))
                    {
                        RILOG_ERROR(
                                "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                                mpeg, errno, __LINE__, __FILE__);
                        ifsReturnCode = IfsReturnCodeStatError;
                        break;
                    }
                    //                  RILOG_INFO("Deleting %s %s bytes\n", mpeg, IfsLongLongToString((size_t)statBuffer.st_size));
                    ifsHandle->mpegSize -= (size_t) statBuffer.st_size;

                    if (stat(ndex, &statBuffer))
                    {
                        RILOG_ERROR(
                                "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                                ndex, errno, __LINE__, __FILE__);
                        ifsReturnCode = IfsReturnCodeStatError;
                        break;
                    }
                    //                  RILOG_INFO("Deleting %s %s bytes\n", ndex, IfsLongLongToString((size_t)statBuffer.st_size));
                    ifsHandle->ndexSize -= (size_t) statBuffer.st_size;

                    if (remove(mpeg))
                    {
                        RILOG_ERROR(
                                "IfsReturnCodeFileDeletingError: remove(%s) failed (%d) in line %d of %s\n",
                                mpeg, errno, __LINE__, __FILE__);
                        ifsReturnCode = IfsReturnCodeFileDeletingError;
                    }
                    if (remove(ndex))
                    {
                        RILOG_ERROR(
                                "IfsReturnCodeFileDeletingError: remove(%s) failed (%d) in line %d of %s\n",
                                ndex, errno, __LINE__, __FILE__);
                        ifsReturnCode = IfsReturnCodeFileDeletingError;
                    }
                    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                        break;

                    ifsHandle->begFileNumber++;

                    // Set the begClock to the new value

                    ifsReturnCode = GenerateFileNames(ifsHandle,
                            ifsHandle->begFileNumber, &mpeg, &ndex);
                    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                        break;

                    pNdex = fopen(ndex, "rb+"); // close here [error checked]
                    if (pNdex == NULL)
                    {
                        RILOG_ERROR(
                                "IfsReturnCodeFileOpeningError: fopen(%s, \"rb+\") failed (%d) in line %d of %s\n",
                                ndex, errno, __LINE__, __FILE__);
                        ifsReturnCode = IfsReturnCodeFileOpeningError;
                        break;
                    }

                    rewind(pNdex);

                    if (fread(&ifsIndexEntry, 1, sizeof(IfsIndexEntry), pNdex)
                            != sizeof(IfsIndexEntry))
                    {
                        RILOG_ERROR(
                                "IfsReturnCodeFileReadingError: fread(%p, 1, %d, %s) failed (%d) in line %d of %s\n",
                                &ifsIndexEntry, sizeof(IfsIndexEntry), ndex,
                                errno, __LINE__, __FILE__);
                        ifsReturnCode = IfsReturnCodeFileReadingError;
                    }
                    else
                    {
                        ifsHandle->begClock = ifsIndexEntry.when; // nanoseconds
                        char temp[32];
                        RILOG_INFO( "set begClock to %s at line %d of %s\n",
                            IfsToSecs(ifsIndexEntry.when, temp), __LINE__, __FILE__);
                    }

                    if (pNdex)
                    {
                        if (fflush(pNdex))
                        {
                            RILOG_ERROR(
                                    "IfsReturnCodeFileFlushingError: fflush(%s) failed (%d) in line %d of %s\n",
                                    ifsHandle->ndex, errno, __LINE__, __FILE__);
                            ifsReturnCode = IfsReturnCodeFileFlushingError;
                        }
                        fclose(pNdex);
                    }

                    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                        break;
                }

                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                {
                    if (mpeg)
                        g_free(mpeg);
                    if (ndex)
                        g_free(ndex);

                    g_static_mutex_unlock(&(ifsHandle->mutex));
                    return ifsReturnCode;
                }

                if (indexDumpMode == IfsIndexDumpModeDef)
                    RILOG_INFO("--------- --------- -\n");
            }
        }
        ifsHandle->endClock = ifsClock; // move end clock (in nanoseconds)
    }
    else // This is NOT a circular buffer
    {
        if (!ifsHandle->begClock) // beg not set yet
        {
            ifsHandle->begClock = ifsClock; // set beg clock in nanoseconds
        }
        ifsHandle->endClock = ifsClock; // move end clock (in nanoseconds)
    }

    if (fseek(ifsHandle->pNdex, 0, SEEK_END))
    {
        RILOG_ERROR(
                "IfsReturnCodeFileSeekingError: fseek(%s, 0, SEEK_END) failed (%d) in line %d of %s\n",
                ifsHandle->ndex, errno, __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeFileSeekingError;
    }

    for (i = 0; i < numPackets; i++)
    {
        if (IfsParsePacket(ifsHandle, pData + i)) // Set and test WHAT
        {
            char temp[256]; // ParseWhat

            ifsHandle->entry.realWhere = ifsHandle->realLoc; // Set real WHERE, offset in packets
            ifsHandle->entry.virtWhere = ifsHandle->virtLoc; // Set virtual WHERE, offset in packets

            if (indexDumpMode == IfsIndexDumpModeAll)
            {
#ifdef DEBUG_ALL_PES_CODES
                RILOG_INFO("%4d  %08lX%08lX = %s\n",
                        indexCase++,
                        (unsigned long)(ifsHandle->entry.what>>32),
                        (unsigned long)ifsHandle->entry.what,
                        ParseWhat(ifsHandle, temp, indexDumpMode, IfsTrue));
#else
                RILOG_INFO("%4d  %08X = %s\n", indexCase++,
                        ifsHandle->entry.what, ParseWhat(ifsHandle, temp,
                                indexDumpMode, IfsTrue));
#endif
                whatAll |= ifsHandle->entry.what;
            }
            if (indexDumpMode == IfsIndexDumpModeDef)
            {
                RILOG_TRACE("%9ld %9ld %s\n", ifsHandle->entry.realWhere, // offset in packets
                        ifsHandle->entry.virtWhere, // offset in packets
                        ParseWhat(ifsHandle, temp, indexDumpMode, IfsTrue));
            }

            if (fwrite(&ifsHandle->entry, 1, sizeof(IfsIndexEntry),
                    ifsHandle->pNdex) != sizeof(IfsIndexEntry))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileWritingError: fwrite(%p, 1, %d, %s) failed (%d) in line %d of %s\n",
                        &ifsHandle->entry, sizeof(IfsIndexEntry),
                        ifsHandle->ndex, errno, __LINE__, __FILE__);
                g_static_mutex_unlock(&(ifsHandle->mutex));
                return IfsReturnCodeFileWritingError;
            }
#ifdef FLUSH_ALL_WRITES
            if (fflush(ifsHandle->pNdex))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileFlushingError: fflush(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, errno, __LINE__, __FILE__);
                g_static_mutex_unlock(&(ifsHandle->mutex));
                return IfsReturnCodeFileFlushingError;
            }
#endif
            ifsHandle->ndexSize += sizeof(IfsIndexEntry);
        }

        ifsHandle->realLoc++; // offset in packets
        ifsHandle->virtLoc++; // offset in packets
    }

    if (fseek(ifsHandle->pMpeg, 0, SEEK_END))
    {
        RILOG_ERROR(
                "IfsReturnCodeFileSeekingError: fseek(%s, 0, SEEK_END) failed (%d) in line %d of %s\n",
                ifsHandle->mpeg, errno, __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeFileSeekingError;
    }
    if (fwrite(pData, sizeof(IfsPacket), numPackets, ifsHandle->pMpeg)
            != numPackets)
    {
        RILOG_ERROR(
                "IfsReturnCodeFileWritingError: fwrite(%p, %d, %ld, %s) failed (%d) in line %d of %s\n",
                pData, sizeof(IfsPacket), numPackets, ifsHandle->mpeg, errno,
                __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeFileWritingError;
    }
#ifdef FLUSH_ALL_WRITES
    if (fflush(ifsHandle->pMpeg))
    {
        RILOG_ERROR(
                "IfsReturnCodeFileFlushingError: fflush(%s) failed (%d) in line %d of %s\n",
                ifsHandle->mpeg, errno, __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeFileFlushingError;
    }
#endif
    ifsHandle->mpegSize += (NumBytes) sizeof(IfsPacket) * (NumBytes) numPackets;

    g_static_mutex_unlock(&(ifsHandle->mutex));
    return IfsReturnCodeNoErrorReported;
}

IfsReturnCode IfsConvert(IfsHandle srcHandle, // Input
        IfsHandle dstHandle, // Input (must be a writer)
        IfsClock * pBegClock, // Input request/Output actual, in nanoseconds
        IfsClock * pEndClock // Input request/Output actual, in nanoseconds
)
{
    IfsIndexEntry * buffer = NULL;
    IfsReturnCode ifsReturnCode;
    FileNumber begFileNumber, endFileNumber;
    NumPackets begPacketNum, endPacketNum, numPackets;
    NumEntries begEntryNum, endEntryNum, numEntries, i;
    IfsClock begClock, endClock;

#ifdef DEBUG_CONVERT_APPEND
    printf("Convert:\n");
#endif

    if (srcHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: srcHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (dstHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: dstHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (pBegClock == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: pBegClock == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (pEndClock == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: pEndClock == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    g_static_mutex_lock(&(dstHandle->mutex));

    if (dstHandle->isReading)
    {
        RILOG_ERROR("IfsReturnCodeMustBeAnIfsWriter: in line %d of %s\n",
                __LINE__, __FILE__);
        g_static_mutex_unlock(&(dstHandle->mutex));
        return IfsReturnCodeMustBeAnIfsWriter;
    }

    do
    {
#ifdef DEBUG_CONVERT_APPEND
        char temp[32]; // IfsToSecs only
#endif
        int b = 0;
        g_static_mutex_lock(&(srcHandle->mutex));

        ifsReturnCode = GetCurrentFileParameters(srcHandle);
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        endClock = srcHandle->endClock < *pEndClock ? srcHandle->endClock
                : *pEndClock;
        begClock = srcHandle->begClock > *pBegClock ? srcHandle->begClock
                : *pBegClock;

#ifdef DEBUG_CONVERT_APPEND
        printf("endClock %s\n", IfsToSecs(endClock, temp));
        printf("begClock %s\n", IfsToSecs(begClock, temp));
#endif

        g_static_mutex_unlock(&(srcHandle->mutex));
        ifsReturnCode = IfsSeekToTimeImpl(srcHandle, IfsDirectEnd, &endClock,
                NULL);
        g_static_mutex_lock(&(srcHandle->mutex));

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        endPacketNum = srcHandle->realLoc;
        endFileNumber = srcHandle->curFileNumber;
        endEntryNum = srcHandle->entryNum;

#ifdef DEBUG_CONVERT_APPEND
        printf("endClock %s endPacketNum %5ld endFileNumber %3ld endEntryNum %4ld\n",
                IfsToSecs(endClock, temp), endPacketNum, endFileNumber, endEntryNum);
#endif

        g_static_mutex_unlock(&(srcHandle->mutex));
        ifsReturnCode = IfsSeekToTimeImpl(srcHandle, IfsDirectBegin, &begClock,
                NULL);
        g_static_mutex_lock(&(srcHandle->mutex));

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        begPacketNum = srcHandle->realLoc;
        begFileNumber = srcHandle->curFileNumber;
        begEntryNum = srcHandle->entryNum;

#ifdef DEBUG_CONVERT_APPEND
        printf("begClock %s begPacketNum %5ld begFileNumber %3ld begEntryNum %4ld\n",
                IfsToSecs(begClock, temp), begPacketNum, begFileNumber, begEntryNum);
#endif

        if (begFileNumber != endFileNumber) // Just convert to the end of the file for now, append more later
        {
            endPacketNum = srcHandle->maxPacket;
            endEntryNum = srcHandle->maxEntry;

#ifdef DEBUG_CONVERT_APPEND
            printf("maxPacket %5ld maxEntry %4ld\n",
                    srcHandle->maxPacket, srcHandle->maxEntry);
#endif

            // Need to calculate the correct endClock time:
            ifsReturnCode = IfsReadNdexEntryAt(srcHandle, endEntryNum);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;
            endClock = srcHandle->entry.when;
            if (fseek(srcHandle->pNdex, begEntryNum * sizeof(IfsIndexEntry),
                    SEEK_SET))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) in line %d of %s\n",
                        srcHandle->ndex, begEntryNum * sizeof(IfsIndexEntry),
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileSeekingError;
                break;
            }
        }

        numPackets = endPacketNum - begPacketNum + 1;
        numEntries = endEntryNum - begEntryNum + 1;

#ifdef DEBUG_CONVERT_APPEND
        printf("numPackets(%5ld) = endPacketNum(%5ld) - begPacketNum(%5ld) + 1\n",
                numPackets, endPacketNum, begPacketNum);
        printf("numEntries(%5ld) = endEntryNum (%5ld) - begEntryNum (%5ld) + 1\n",
                numEntries, endEntryNum, begEntryNum);
#endif
        if (0 == numPackets)
        {
            RILOG_DEBUG("%s numPackets(%5ld) = "
                        "endPacketNum(%5ld) - begPacketNum(%5ld) + 1\n",
                        __func__, numPackets, endPacketNum, begPacketNum);
            RILOG_DEBUG("%s numEntries(%5ld) = "
                        "endEntryNum (%5ld) - begEntryNum (%5ld) + 1\n",
                        __func__, numEntries, endEntryNum, begEntryNum);
            break;
        }
        else
        {
            buffer = g_try_malloc(numPackets * sizeof(IfsPacket));
            if (buffer == NULL)
            {
                RILOG_CRIT(
                        "IfsReturnCodeMemAllocationError: buffer == NULL in line %d of %s trying to allocate %lu\n",
                        __LINE__, __FILE__, numPackets * sizeof(IfsPacket));;
                ifsReturnCode = IfsReturnCodeMemAllocationError;
                break;
            }
        }

        RILOG_TRACE("Copy   %5ld MPEG packets [%5ld to %5ld] (%7ld bytes)\n",
                numPackets, begPacketNum, endPacketNum, sizeof(IfsPacket)
                        * numPackets);
        RILOG_TRACE("Copy   %5ld NDEX entries [%5ld to %5ld] (%7ld bytes)\n",
                numEntries, begEntryNum, endEntryNum, sizeof(IfsIndexEntry)
                        * numEntries);

        // Copy the MPEG data
        if ((b = fread(buffer, sizeof(IfsPacket), numPackets, srcHandle->pMpeg))
                != numPackets)
        {
            RILOG_ERROR("IfsReturnCodeFileReadingError: %d = "
                        "fread(%p, %d, %ld, %s) in line %d of %s\n",
                        b, buffer, sizeof(IfsPacket), numPackets,
                        srcHandle->mpeg, __LINE__, __FILE__);
            if (0 == b)
            {
                // if we've had 4 successive empty fread calls, stop trying!
                if (++srcHandle->numEmptyFreads > 4)
                {
                    ifsReturnCode = IfsReturnCodeFileReadingError;
                    break;
                }
            }
            else
            {
                // we read less than expected, reset our numPackets
                numPackets = b;
            }
        }

        // we've read something, reset our empty read counter...
        srcHandle->numEmptyFreads = 0;

        if (fseek(dstHandle->pMpeg, 0, SEEK_END))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileSeekingError: fseek(%s, 0, SEEK_END) in line %d of %s\n",
                    dstHandle->mpeg, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileSeekingError;
            break;
        }
        if (fwrite(buffer, sizeof(IfsPacket), numPackets, dstHandle->pMpeg)
                != numPackets)
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileWritingError: fwrite(%p, %d, %ld, %s) in line %d of %s\n",
                    buffer, sizeof(IfsPacket), numPackets, dstHandle->mpeg,
                    __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileWritingError;
            break;
        }
#ifdef FLUSH_ALL_WRITES
        if (fflush(dstHandle->pMpeg))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileFlushingError: fflush(%s) in line %d of %s\n",
                    dstHandle->mpeg, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileFlushingError;
            break;
        }
#endif
        dstHandle->mpegSize = (NumBytes) sizeof(IfsPacket)
                * (NumBytes) numPackets;

        // Copy the NDEX data (with modification)
        if ((b = fread(buffer, sizeof(IfsIndexEntry), numEntries,
                 srcHandle->pNdex)) != numEntries)
        {
            RILOG_ERROR("IfsReturnCodeFileReadingError: %d = "
                        "fread(%p, %d, %ld, %s) in line %d of %s\n",
                        b, buffer, sizeof(IfsIndexEntry), numEntries,
                        srcHandle->ndex, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileReadingError;
            break;
        }
        for (i = 0; i < numEntries; i++)
        {
            //          RILOG_INFO("%10ld %3ld -> %3ld\n", i, buffer[i].where, buffer[i].where - begPacketNum);
            buffer[i].virtWhere = (buffer[i].realWhere -= begPacketNum);
        }
        if (fseek(dstHandle->pNdex, 0, SEEK_END))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileSeekingError: fseek(%s, 0, SEEK_END) in line %d of %s\n",
                    dstHandle->ndex, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileSeekingError;
            break;
        }
        if (fwrite(buffer, sizeof(IfsIndexEntry), numEntries, dstHandle->pNdex)
                != numEntries)
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileWritingError: fwrite(%p, %d, %ld, %s) in line %d of %s\n",
                    buffer, sizeof(IfsIndexEntry), numEntries, dstHandle->ndex,
                    __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileWritingError;
            break;
        }
#ifdef FLUSH_ALL_WRITES
        if (fflush(dstHandle->pNdex))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileFlushingError: fflush(%s) in line %d of %s\n",
                    dstHandle->ndex, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileFlushingError;
            break;
        }
#endif
        dstHandle->ndexSize = (NumBytes) sizeof(IfsIndexEntry)
                * (NumBytes) numEntries;

        if (begFileNumber == endFileNumber) // Everything we need is in one file (so far)
        {
            srcHandle->appendFileNumber = endFileNumber;
            srcHandle->appendPacketNum = endPacketNum + 1;
            srcHandle->appendEntryNum = endEntryNum + 1;
            srcHandle->appendIndexShift = begPacketNum;
            srcHandle->appendPrevFiles = 0;
        }
        else // Just converted to the end of the file, set up for the next append
        {
            srcHandle->appendFileNumber = begFileNumber + 1;
            srcHandle->appendPacketNum = 0;
            srcHandle->appendEntryNum = 0;
            srcHandle->appendIndexShift = 0;
            srcHandle->appendPrevFiles = numPackets;
        }

    } while (0);

    if (ifsReturnCode == IfsReturnCodeNoErrorReported)
    {
        dstHandle->begClock = *pBegClock = begClock;
        dstHandle->endClock = *pEndClock = endClock;
    }
    else
    {
        dstHandle->begClock = *pBegClock = 0;
        dstHandle->endClock = *pEndClock = 0;
        dstHandle->mpegSize = 0;
        dstHandle->ndexSize = 0;
    }

    g_static_mutex_unlock(&(srcHandle->mutex));
    g_static_mutex_unlock(&(dstHandle->mutex));

    if (buffer)
        g_free(buffer);

    return ifsReturnCode;
}

IfsReturnCode IfsAppend // Must call IfsConvert() before calling this function
(IfsHandle srcHandle, // Input
        IfsHandle dstHandle, // Input (must be a writer)
        IfsClock * pEndClock // Input request/Output actual, in nanoseconds
)
{
    IfsIndexEntry * buffer = NULL;
    IfsReturnCode ifsReturnCode;
    NumEntries numEntries, i;
    NumPackets numPackets;

    FileNumber begFileNumber; // from srcHandle via append info
    FileNumber endFileNumber; // from srcHandle via seek to endClock
    NumPackets begPacketNum; // from srcHandle via append info
    NumPackets endPacketNum; // from srcHandle via seek to endClock
    NumEntries begEntryNum; // from srcHandle via append info
    NumEntries endEntryNum; // from srcHandle via seek to endClock
    IfsClock endClock; // from srcHandle via IfsInfo then seek to endClock

#ifdef DEBUG_CONVERT_APPEND
    printf("Append:\n");
#endif

    if (srcHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: srcHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (dstHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: dstHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (pEndClock == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: pEndClock == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    g_static_mutex_lock(&(dstHandle->mutex));

    if (dstHandle->isReading)
    {
        RILOG_ERROR("IfsReturnCodeMustBeAnIfsWriter: in line %d of %s\n",
                __LINE__, __FILE__);
        g_static_mutex_unlock(&(dstHandle->mutex));
        return IfsReturnCodeMustBeAnIfsWriter;
    }

    do
    {
#ifdef DEBUG_CONVERT_APPEND
        char temp[32]; // IfsToSecs only
#endif
        g_static_mutex_lock(&(srcHandle->mutex));

        // See how much data is currently available
        ifsReturnCode = GetCurrentFileParameters(srcHandle);

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        endClock = srcHandle->endClock < *pEndClock ? srcHandle->endClock
                : *pEndClock;

#ifdef DEBUG_CONVERT_APPEND
        printf("endClock %s\n", IfsToSecs(endClock, temp));
#endif

        g_static_mutex_unlock(&(srcHandle->mutex));
        ifsReturnCode = IfsSeekToTimeImpl(srcHandle, IfsDirectEnd, &endClock,
                NULL);
        g_static_mutex_lock(&(srcHandle->mutex));

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        endPacketNum = srcHandle->realLoc;
        endFileNumber = srcHandle->curFileNumber;
        endEntryNum = srcHandle->entryNum;

#ifdef DEBUG_CONVERT_APPEND
        printf("endClock %s endPacketNum %5ld endFileNumber %3ld endEntryNum %4ld\n",
                IfsToSecs(endClock, temp), endPacketNum, endFileNumber, endEntryNum);
#endif

        begPacketNum = srcHandle->appendPacketNum;
        begFileNumber = srcHandle->appendFileNumber;
        begEntryNum = srcHandle->appendEntryNum;

#ifdef DEBUG_CONVERT_APPEND
        printf("begClock    NA begPacketNum %5ld begFileNumber %3ld begEntryNum %4ld\n",
                begPacketNum, begFileNumber, begEntryNum);
#endif

        g_static_mutex_unlock(&(srcHandle->mutex));
        ifsReturnCode = IfsOpenActualFiles(srcHandle, begFileNumber, "rb+");
        g_static_mutex_lock(&(srcHandle->mutex));

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        if (fseek(srcHandle->pMpeg, begPacketNum * sizeof(IfsPacket), SEEK_SET))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) in line %d of %s\n",
                    srcHandle->mpeg, begPacketNum * sizeof(IfsPacket),
                    __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileSeekingError;
            break;
        }
        if (fseek(srcHandle->pNdex, begEntryNum * sizeof(IfsIndexEntry),
                SEEK_SET))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) in line %d of %s\n",
                    srcHandle->ndex, begEntryNum * sizeof(IfsIndexEntry),
                    __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileSeekingError;
            break;
        }

        if (begFileNumber != endFileNumber) // Just convert to the end of the file for now, append more later
        {
            struct stat statBuffer;

            if (stat(srcHandle->mpeg, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        srcHandle->mpeg, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeStatError;
                break;
            }
            endPacketNum = (size_t) statBuffer.st_size / sizeof(IfsPacket) - 1;

            if (stat(srcHandle->ndex, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        srcHandle->ndex, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeStatError;
                break;
            }
            endEntryNum = (size_t) statBuffer.st_size / sizeof(IfsIndexEntry)
                    - 1;

#ifdef DEBUG_CONVERT_APPEND
            printf("maxPacket %5ld maxEntry %4ld\n", endPacketNum, endEntryNum);
#endif

            // Need to calculate the correct endClock time:
            ifsReturnCode = IfsReadNdexEntryAt(srcHandle, endEntryNum);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;
            endClock = srcHandle->entry.when;
            if (fseek(srcHandle->pNdex, begEntryNum * sizeof(IfsIndexEntry),
                    SEEK_SET))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) in line %d of %s\n",
                        srcHandle->ndex, begEntryNum * sizeof(IfsIndexEntry),
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileSeekingError;
                break;
            }
        }

        numPackets = endPacketNum - begPacketNum + 1;
        numEntries = endEntryNum - begEntryNum + 1;

#ifdef DEBUG_CONVERT_APPEND
        printf("numPackets(%5ld) = endPacketNum(%5ld) - begPacketNum(%5ld) + 1\n",
                numPackets, endPacketNum, begPacketNum);
        printf("numEntries(%5ld) = endEntryNum (%5ld) - begEntryNum (%5ld) + 1\n",
                numEntries, endEntryNum, begEntryNum);
#endif
        if (0 == numPackets)
        {
            RILOG_DEBUG("%s numPackets(%5ld) = "
                        "endPacketNum(%5ld) - begPacketNum(%5ld) + 1\n",
                        __func__, numPackets, endPacketNum, begPacketNum);
            RILOG_DEBUG("%s numEntries(%5ld) = "
                        "endEntryNum (%5ld) - begEntryNum (%5ld) + 1\n",
                        __func__, numEntries, endEntryNum, begEntryNum);
        }
        else
        {
            buffer = g_try_malloc(numPackets * sizeof(IfsPacket));
            if (buffer == NULL)
            {
                RILOG_CRIT(
                        "IfsReturnCodeMemAllocationError: buffer == NULL in line %d of %s trying to allocate %lu\n",
                        __LINE__, __FILE__, numPackets * sizeof(IfsPacket));;
                ifsReturnCode = IfsReturnCodeMemAllocationError;
                break;
            }
        }

        if (numPackets || numEntries)
        {
            int b = 0;
            RILOG_TRACE(
                    "Append %5ld MPEG packets [%5ld to %5ld] (%7ld bytes)\n",
                    numPackets, begPacketNum, endPacketNum, sizeof(IfsPacket)
                            * numPackets);
            RILOG_TRACE(
                    "Append %5ld NDEX entries [%5ld to %5ld] (%7ld bytes)\n",
                    numEntries, begEntryNum, endEntryNum, sizeof(IfsIndexEntry)
                            * numEntries);

            // Copy the MPEG data
            if ((b = fread(buffer, sizeof(IfsPacket), numPackets,
                     srcHandle->pMpeg)) != numPackets)
            {
                RILOG_WARN("IfsReturnCodeFileReadingError: %d = "
                           "fread(%p, %d, %ld, %s) in line %d of %s\n",
                           b, buffer, sizeof(IfsPacket), numPackets,
                           srcHandle->mpeg, __LINE__, __FILE__);

                if (0 == b)
                {
                    // if we've had 4 successive empty fread calls, stop trying!
                    if (++srcHandle->numEmptyFreads > 4)
                    {
                        ifsReturnCode = IfsReturnCodeFileReadingError;
                        break;
                    }
                }
                else
                {
                    // we read less than expected, reset our numPackets
                    numPackets = b;
                }
            }

            // we've read something, reset our empty read counter...
            srcHandle->numEmptyFreads = 0;

            if (fseek(dstHandle->pMpeg, 0, SEEK_END))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, 0, SEEK_END) in line %d of %s\n",
                        dstHandle->mpeg, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileSeekingError;
                break;
            }
            if (fwrite(buffer, sizeof(IfsPacket), numPackets, dstHandle->pMpeg)
                    != numPackets)
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileWritingError: fwrite(%p, %d, %ld, %s) in line %d of %s\n",
                        buffer, sizeof(IfsPacket), numPackets, dstHandle->mpeg,
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileWritingError;
                break;
            }
#ifdef FLUSH_ALL_WRITES
            if (fflush(dstHandle->pMpeg))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileFlushingError: fflush(%s) in line %d of %s\n",
                        dstHandle->mpeg, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileFlushingError;
                break;
            }
#endif
            dstHandle->mpegSize += (NumBytes) sizeof(IfsPacket)
                    * (NumBytes) numPackets;

            // Copy the NDEX data (with modification)
            if ((b = fread(buffer, sizeof(IfsIndexEntry), numEntries,
                     srcHandle->pNdex)) != numEntries)
            {
                RILOG_ERROR("IfsReturnCodeFileReadingError: %d = "
                            "fread(%p, %d, %ld, %s) in line %d of %s\n",
                            b, buffer, sizeof(IfsIndexEntry), numEntries,
                            srcHandle->ndex, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileReadingError;
                break;
            }
            for (i = 0; i < numEntries; i++)
            {
                //              RILOG_INFO("%10ld %3ld -> %3ld\n",
                //                         i+numEntriesOut,
                //                         buffer[i].where,
                //                         buffer[i].where - srcHandle->appendIndexShift);
                buffer[i].realWhere -= srcHandle->appendIndexShift;
                buffer[i].realWhere += srcHandle->appendPrevFiles;
                buffer[i].virtWhere = buffer[i].realWhere;
            }
            if (fseek(dstHandle->pNdex, 0, SEEK_END))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, 0, SEEK_END) in line %d of %s\n",
                        dstHandle->ndex, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileSeekingError;
                break;
            }
            if (fwrite(buffer, sizeof(IfsIndexEntry), numEntries,
                    dstHandle->pNdex) != numEntries)
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileWritingError: fwrite(%p, %d, %ld, %s) in line %d of %s\n",
                        buffer, sizeof(IfsIndexEntry), numEntries,
                        dstHandle->ndex, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileWritingError;
                break;
            }
#ifdef FLUSH_ALL_WRITES
            if (fflush(dstHandle->pNdex))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileFlushingError: fflush(%s) in line %d of %s\n",
                        dstHandle->ndex, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileFlushingError;
                break;
            }
#endif
            dstHandle->ndexSize += (NumBytes) sizeof(IfsIndexEntry)
                    * (NumBytes) numEntries;
        }
        else
        {
            struct stat statBuffer;
            NumPackets maxPacketNum;

            if (stat(srcHandle->mpeg, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        srcHandle->mpeg, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeStatError;
                break;
            }
            maxPacketNum = (size_t) statBuffer.st_size / sizeof(IfsPacket) - 1;

#ifdef DEBUG_CONVERT_APPEND
            printf("maxPacketNum %5ld endPacketNum %5ld\n", maxPacketNum, endPacketNum);
#endif

            if (endPacketNum < maxPacketNum)
            {
#ifdef DEBUG_CONVERT_APPEND
                printf("SPECIAL CASE DETECTED\n");
#endif

                endClock = *pEndClock;
            }
        }

        if (begFileNumber == endFileNumber) // Everything we need is in one file (so far)
        {
            srcHandle->appendFileNumber = endFileNumber;
            srcHandle->appendPacketNum = endPacketNum + 1;
            srcHandle->appendEntryNum = endEntryNum + 1;
            //cHandle->appendIndexShift = <does not change>
            //cHandle->appendPrevFiles  = <does not change>

        }
        else // Just converted to the end of the file, set up for the next append
        {
            srcHandle->appendFileNumber = begFileNumber + 1;
            srcHandle->appendPacketNum = 0;
            srcHandle->appendEntryNum = 0;
            srcHandle->appendIndexShift = 0;
            srcHandle->appendPrevFiles = dstHandle->mpegSize
                    / sizeof(IfsPacket);
        }

    } while (0);

    if (ifsReturnCode == IfsReturnCodeNoErrorReported)
    {
        dstHandle->endClock = *pEndClock = endClock;
    }
    else
    {
        dstHandle->begClock = 0;
        dstHandle->endClock = *pEndClock = 0;
        dstHandle->mpegSize = 0;
        dstHandle->ndexSize = 0;
    }

    g_static_mutex_unlock(&(srcHandle->mutex));
    g_static_mutex_unlock(&(dstHandle->mutex));

    if (buffer)
        g_free(buffer);

    return ifsReturnCode;
}

IfsReturnCode IfsRead(IfsHandle ifsHandle, // Input
        NumPackets * pNumPackets, // Input request/Output actual
        IfsClock * pCurClock, // Output current clock value
        IfsPacket ** ppData // Output
)
{
    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

    if (ppData != NULL)
        *ppData = NULL;
    if (pNumPackets == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: pNumPackets == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    do
    {
        struct stat statBuffer;
        NumBytes numBytes;
        NumPackets numPackets;
        NumPackets havePackets;

        if (ifsHandle == NULL)
        {
            RILOG_ERROR(
                    "IfsReturnCodeBadInputParameter: ifsHandle == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            *pNumPackets = 0;
            return IfsReturnCodeBadInputParameter;
        }

        g_static_mutex_lock(&(ifsHandle->mutex));

        if (ppData == NULL)
        {
            RILOG_ERROR(
                    "IfsReturnCodeBadInputParameter: ppData == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeBadInputParameter;
            break;
        }

        if (stat(ifsHandle->mpeg, &statBuffer))
        {
            RILOG_ERROR(
                    "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->mpeg, errno, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeStatError;
            break;
        }

        numBytes = (size_t) statBuffer.st_size;
        numPackets = numBytes / sizeof(IfsPacket);
        havePackets = numPackets - ifsHandle->realLoc;

        //      {   char temp[32];
        //
        //          RILOG_INFO("-----------------------\n");
        //          RILOG_INFO("*pNumPackets = %ld packets\n", *pNumPackets);
        //          RILOG_INFO("MPEG file    = %s\n"         , ifsHandle->mpeg);
        //          RILOG_INFO("numBytes     = %s bytes\n"   , IfsLongLongToString(numBytes, temp));
        //          RILOG_INFO("numPackets   = %ld packets\n", numPackets);
        //          RILOG_INFO("realLoc      = %ld packets\n", ifsHandle->realLoc);
        //          RILOG_INFO("virtLoc      = %ld packets\n", ifsHandle->virtLoc);
        //          RILOG_INFO("havePackets  = %ld packets\n", havePackets);
        //      }

        if (havePackets == 0)
        {
            ifsReturnCode = GetCurrentFileSizeAndCount(ifsHandle);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;

            if (ifsHandle->curFileNumber == ifsHandle->endFileNumber)
            {
                RILOG_INFO("IfsReturnCodeReadPastEndOfFile in line %d of %s\n",
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeReadPastEndOfFile;
            }
            else // go to the next file
            {
                g_static_mutex_unlock(&(ifsHandle->mutex));
                ifsReturnCode = IfsOpenActualFiles(ifsHandle,
                        ++ifsHandle->curFileNumber, "rb+");
                g_static_mutex_lock(&(ifsHandle->mutex));

                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                    break;

                ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, 0);
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                    break;

                ifsHandle->realLoc = 0;

                *pNumPackets = 0;
            }
            break;
        }
        else
        {
            if (*pNumPackets > havePackets)
                *pNumPackets = havePackets;

            //      RILOG_INFO("*pNumPackets = %ld packets\n", *pNumPackets);

            numBytes = (NumBytes) * pNumPackets * (NumBytes) sizeof(IfsPacket);

            //      RILOG_INFO("numBytes     = %s bytes\n", IfsLongLongToString(numBytes));

            *ppData = g_try_malloc(numBytes);
            if (*ppData == NULL)
            {
                RILOG_CRIT(
                        "IfsReturnCodeMemAllocationError: *ppData == NULL in line %d of %s\n",
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeMemAllocationError;
                break;
            }

            if (fread(*ppData, 1, numBytes, ifsHandle->pMpeg) != numBytes)
            {
#ifdef STAND_ALONE
#ifdef DEBUG_ERROR_LOGS
                char temp[32];
#endif
#else // not STAND_ALONE
                char temp[32];
#endif

                RILOG_ERROR(
                        "IfsReturnCodeFileReadingError: fread(%p, 1, %s, %s) failed (%d) in line %d of %s\n",
                        *ppData, IfsLongLongToString(numBytes, temp),
                        ifsHandle->mpeg, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileReadingError;
                break;
            }

            ifsHandle->realLoc += *pNumPackets;
            ifsHandle->virtLoc += *pNumPackets;
        }

        if (pCurClock)
        {
            *pCurClock = ifsHandle->entry.when;
            //printf("Clock init %s, %ld %ld\n", IfsToSecs(*pCurClock), ifsHandle->entry.where, ifsHandle->location);

            while (ifsHandle->entry.realWhere < ifsHandle->realLoc)
            {
                if (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry),
                        ifsHandle->pNdex) != sizeof(IfsIndexEntry))
                {
                    break;
                }
                *pCurClock = ifsHandle->entry.when;
                //printf("Clock next %s, %ld %ld\n", IfsToSecs(*pCurClock), ifsHandle->entry.where, ifsHandle->location);
            }
        }
    } while (0);

    g_static_mutex_unlock(&(ifsHandle->mutex));

    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
    {
        *pNumPackets = 0;
        return ifsReturnCode;
    }

    //printf("CurClock is %s\n", IfsToSecs(*pCurClock));

    return IfsReturnCodeNoErrorReported;
}

IfsReturnCode IfsReadPicture // Must call IfsSeekToTime() before calling this function
(IfsHandle ifsHandle, // Input
        IfsPcr ifsPcr, // Input
        IfsPts ifsPts, // Input
        IfsReadType ifsReadType, // Input
        NumPackets * pNumPackets, // Output
        IfsPacket ** ppData, // Output
        NumPackets * pStartPacket // Output
)
{
    if (pNumPackets != NULL)
        *pNumPackets = 0;
    if (ppData != NULL)
        *ppData = NULL;

    if (ifsHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ifsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (pNumPackets == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: pNumPackets == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (ppData == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ppData == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

    g_static_mutex_lock(&(ifsHandle->mutex));
    IfsClock location = ifsHandle->entry.when; // nanoseconds

    IfsClock nextDiff = IFS_UNDEFINED_CLOCK;
    IfsClock prevDiff = IFS_UNDEFINED_CLOCK;
    IfsClock bestDiff = IFS_UNDEFINED_CLOCK;

    NumEntries nextEntry = ifsHandle->entryNum;
    NumEntries prevEntry = ifsHandle->entryNum;
    NumEntries bestEntry = IFS_UNDEFINED_ENTRY;

    NumPackets nextWhere = IFS_UNDEFINED_PACKET;
    NumPackets prevWhere = IFS_UNDEFINED_PACKET;
    NumPackets bestWhere = IFS_UNDEFINED_PACKET;

    FileNumber nextFile = ifsHandle->curFileNumber;
    FileNumber prevFile = ifsHandle->curFileNumber;
    FileNumber bestFile = IFS_UNDEFINED_FILENUMBER;

    struct stat statBuffer;

    (void) ifsPcr;
    (void) ifsPts;

    // Start by figuring out what file to use to find next I frame
    // it could be current, next or previous file
    if (ifsReadType != IfsReadTypePrevious) // scan forward for the next I frame
    {
        // Look forward for start of seq hdr which indicates start of next I frame
        while (IfsTrue)
        {
            // Read entries in current file until end of file reached
            while (nextEntry <= ifsHandle->maxEntry)
            {
                ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, nextEntry);
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                {                     
                    g_static_mutex_unlock(&(ifsHandle->mutex));
                    return ifsReturnCode;
                }
                if (ifsHandle->entry.what & IfsIndexStartSeqHeader)
                {
                    // Found the starting location!
                    // Set next time difference to time of this entry and starting entry time when method was called
                    nextDiff = ifsHandle->entry.when - location; // nanoseconds

                    // Set next where (nbr of pkts) to this entry's where
                    // Virtual is position in overall stream vs Real which is position relative to this file
                    nextWhere = ifsHandle->entry.virtWhere;

#ifndef PRODUCTION_BUILD
                    char temp1[32], temp2[32], temp3[32]; // IfsToSecs only

                    RILOG_TRACE(
                            "\n IndexStartSeqHeader found in entry %3ld in file %2ld init %s now %s diff %s\n",
                            nextEntry, nextFile, IfsToSecs(location, temp1),
                            IfsToSecs(ifsHandle->entry.when, temp2), IfsToSecs(
                                    nextDiff, temp3));
#endif
                    break;
                }

                // Go on to next entry
                nextEntry++;
            }

            // If start of IFrame/seq hdr found, skip opening next file
            if (nextDiff != IFS_UNDEFINED_CLOCK)
                break;

            // If at end of files, not another file to open, skip opening next file
            if (nextFile == ifsHandle->endFileNumber)
                break;

            // Did not find start of IFrame/seq hdr, open next file
            g_static_mutex_unlock(&(ifsHandle->mutex));
            ifsReturnCode = IfsOpenActualFiles(ifsHandle, ++nextFile, "rb+");
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                return ifsReturnCode;

            g_static_mutex_lock(&(ifsHandle->mutex));

            if (stat(ifsHandle->ndex, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, errno, __LINE__, __FILE__);
                g_static_mutex_unlock(&(ifsHandle->mutex));
                return IfsReturnCodeStatError;
            }

            // Reset these two vars so above while loop will restart with new file
            ifsHandle->maxEntry = (size_t) statBuffer.st_size
                    / sizeof(IfsIndexEntry) - 1;
            nextEntry = 0;
        }
    }

    // If current file has changed, open prev file which will be "old" current file
    if (ifsHandle->curFileNumber != prevFile)
    {
        g_static_mutex_unlock(&(ifsHandle->mutex));
        ifsReturnCode = IfsOpenActualFiles(ifsHandle, prevFile, "rb+");
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            return ifsReturnCode;

        g_static_mutex_lock(&(ifsHandle->mutex));
        if (stat(ifsHandle->ndex, &statBuffer))
        {
            RILOG_ERROR(
                    "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->ndex, errno, __LINE__, __FILE__);
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return IfsReturnCodeStatError;
        }
        ifsHandle->maxEntry = (size_t) statBuffer.st_size
                / sizeof(IfsIndexEntry) - 1;
    }

    if (ifsReadType != IfsReadTypeNext) // scan backward for the prev I frame
    {
        // Look backward for start of seq hdr which indicates start of previous I frame
        while (IfsTrue)
        {
            // Read entries in current file until beginning of file reached
            while (prevEntry > 0)
            {
                ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, prevEntry - 1);
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                {
                    g_static_mutex_unlock(&(ifsHandle->mutex));
                    return ifsReturnCode;
                }
                if (ifsHandle->entry.what & IfsIndexStartSeqHeader)
                {
                    // Found the starting location!
                    // Set prev time difference to starting entry time when method was called and time of this entry
                    prevDiff = location - ifsHandle->entry.when; // nanoseconds

                    // Set prev where (nbr of pkts) to this entry's where
                    // Virtual is position in overall stream vs Real which is position relative to this file
                    prevWhere = ifsHandle->entry.virtWhere;

                    // ??? Why are we decrementing this here???
                    prevEntry--;

#ifndef PRODUCTION_BUILD
                    char temp1[32], temp2[32], temp3[32]; // IfsToSecs only

                    RILOG_TRACE(
                            " IndexStartSeqHeader found in entry %3ld in file %2ld init %s now %s diff %s\n",
                            prevEntry, prevFile, IfsToSecs(location, temp1),
                            IfsToSecs(ifsHandle->entry.when, temp2), IfsToSecs(
                                    prevDiff, temp3));
#endif
                    break;
                }
                // Go on to previous entry
                prevEntry--;
            }

            // If start of IFrame found, skip opening next file
            if (prevDiff != IFS_UNDEFINED_CLOCK)
                break;

            // If at first file, not another file to open, skip opening previous file
            if (prevFile == ifsHandle->begFileNumber)
                break;

            // Did not find start of I Frame, open prev file
            g_static_mutex_unlock(&(ifsHandle->mutex));
            ifsReturnCode = IfsOpenActualFiles(ifsHandle, --prevFile, "rb+");
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                return ifsReturnCode;

            g_static_mutex_lock(&(ifsHandle->mutex));

            if (stat(ifsHandle->ndex, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, errno, __LINE__, __FILE__);
                g_static_mutex_unlock(&(ifsHandle->mutex));
                return IfsReturnCodeStatError;
            }

            // Reset these two vars so above while loop will restart with new file
            ifsHandle->maxEntry = (size_t) statBuffer.st_size
                    / sizeof(IfsIndexEntry) - 1;
            prevEntry = ifsHandle->maxEntry + 1;
        }
    }

    // Based on if reading nearest, previous or next, set "best" which is
    // the most likely spot (file, entry and packet) to find a I-Frame
    // from this call's starting entry
    switch (ifsReadType)
    {
    case IfsReadTypeNext:
        // Going forward so use next
        bestDiff = nextDiff;
        bestEntry = nextEntry;
        bestWhere = nextWhere;
        bestFile = nextFile;
        break;

    case IfsReadTypePrevious:
        // Going backward so use prev
        bestDiff = prevDiff;
        bestEntry = prevEntry;
        bestWhere = prevWhere;
        bestFile = prevFile;
        break;

    case IfsReadTypeNearest:
        // Want nearest but next is undefined, so use prev
        if (nextDiff == IFS_UNDEFINED_CLOCK)
        {
            bestDiff = prevDiff;
            bestEntry = prevEntry;
            bestWhere = prevWhere;
            bestFile = prevFile;
        }
        // Want nearest but prev is undefined, so use next
        else if (prevDiff == IFS_UNDEFINED_CLOCK)
        {
            bestDiff = nextDiff;
            bestEntry = nextEntry;
            bestWhere = nextWhere;
            bestFile = nextFile;
        }
        // Both prev & next defined, look at differences b/w next and prev
        // to determine which one is closer
        // Use prev because it is closer than next
        else if (prevDiff < nextDiff)
        {
            bestDiff = prevDiff;
            bestEntry = prevEntry;
            bestWhere = prevWhere;
            bestFile = prevFile;
        }
        // Use next because it is closer than prev
        else
        {
            bestDiff = nextDiff;
            bestEntry = nextEntry;
            bestWhere = nextWhere;
            bestFile = nextFile;
        }
        break;
    }

    // Make sure there is a "best" available
    if (bestDiff == IFS_UNDEFINED_CLOCK)
    {
        RILOG_WARN("IfsReturnCodeIframeNotFound in line %d of %s\n", __LINE__,
                __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeIframeNotFound;
    }

    RILOG_TRACE(" Best entry is %ld in file %2ld\n", bestEntry, bestFile);

    // End up here when next file did not contain a complete IFrame,
    // looking in previous file for a complete IFrame
    TryAgain:

    // If best file is different than current file, open it
    if (ifsHandle->curFileNumber != bestFile)
    {
        g_static_mutex_unlock(&(ifsHandle->mutex));
        ifsReturnCode = IfsOpenActualFiles(ifsHandle, bestFile, "rb+");

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            return ifsReturnCode;

        g_static_mutex_lock(&(ifsHandle->mutex));

        if (stat(ifsHandle->ndex, &statBuffer))
        {
            RILOG_ERROR(
                    "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->ndex, errno, __LINE__, __FILE__);
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return IfsReturnCodeStatError;
        }
        ifsHandle->maxEntry = (size_t) statBuffer.st_size
                / sizeof(IfsIndexEntry) - 1;
    }

    // Initialize flag vars to indicate looking for start of I Frame and
    // end of I Frame has not yet been found
    IfsBoolean endOfIframeFound = IfsFalse;
    IfsBoolean lookingForIframe = IfsTrue;

    // Start with best, use next as looping var
    nextEntry = bestEntry;
    nextFile = bestFile;

    while (IfsTrue)
    {
        // Make sure there is another entry to read and don't go off end
        while (nextEntry <= ifsHandle->maxEntry)
        {
            ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, nextEntry);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            {
                g_static_mutex_unlock(&(ifsHandle->mutex));
                return ifsReturnCode;
            }

            // Look at current entry's type
            switch (ifsHandle->entry.what & IfsIndexStartPicture)
            {
            // Current entry is start of I Frame
            case IfsIndexStartPicture0: // I
                RILOG_TRACE(
                        " IndexStartPicture I found in entry %3ld in file %2ld\n",
                        nextEntry, nextFile);
                // Found start of I Frame
                if (lookingForIframe)
                    // No longer looking for I frame since it has been found
                    lookingForIframe = IfsFalse;
                else
                    // Already found start of I Frame, found start of next I Frame
                    // so end of current I Frame has been found
                    endOfIframeFound = IfsTrue;
                break;

            // Current entry is start of P Frame
            case IfsIndexStartPicture1: // P
                RILOG_TRACE(
                        " IndexStartPicture P found in entry %3ld in file %2ld\n",
                        nextEntry, nextFile);
                // If found P frame prior to I frame, report error & return
                if (lookingForIframe)
                {
                    RILOG_WARN(
                            "IfsReturnCodeFoundPbeforeIframe in line %d of %s\n",
                            __LINE__, __FILE__);
                    g_static_mutex_unlock(&(ifsHandle->mutex));
                    return IfsReturnCodeFoundPbeforeIframe;
                }
                else
                    // Already found start of I Frame, found start of P Frame
                    // so end of current I Frame has been found
                    endOfIframeFound = IfsTrue;
                break;

            // Current entry is start of B Frame
            case IfsIndexStartPicture: // B
                RILOG_TRACE(
                        " IndexStartPicture B found in entry %3ld in file %2ld\n",
                        nextEntry, nextFile);
                // If found B frame prior to I frame, report error & return
                if (lookingForIframe)
                {
                    RILOG_WARN(
                            "IfsReturnCodeFoundBbeforeIframe in line %d of %s\n",
                            __LINE__, __FILE__);
                    g_static_mutex_unlock(&(ifsHandle->mutex));
                    return IfsReturnCodeFoundBbeforeIframe;
                }
                else
                    // Already found start of I Frame, found start of B Frame
                    // so end of current I Frame has been found
                    endOfIframeFound = IfsTrue;
                break;
            }

            // Get out of loop if end of I frame is found
            if (endOfIframeFound)
                break;

            // Still looking, go on to next entry
            nextEntry++;
        }

        // If end of frame was found, done exit loop
        if (endOfIframeFound)
            break;

        // Got to end of entries, exit loop since no more files to look in
        if (nextFile == ifsHandle->endFileNumber)
            break;

        // Open up next file to search
        g_static_mutex_unlock(&(ifsHandle->mutex));
        ifsReturnCode = IfsOpenActualFiles(ifsHandle, ++nextFile, "rb+");

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            return ifsReturnCode;

        g_static_mutex_lock(&(ifsHandle->mutex));

        if (stat(ifsHandle->ndex, &statBuffer))
        {
            RILOG_ERROR(
                    "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->ndex, errno, __LINE__, __FILE__);
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return IfsReturnCodeStatError;
        }

        // Reset these two vars so above while loop will restart with new file
        ifsHandle->maxEntry = (size_t) statBuffer.st_size
                / sizeof(IfsIndexEntry) - 1;
        nextEntry = 0;
    }

    // If found complete I-Frame, get packets of I-Frame to return
    if (endOfIframeFound)
    {
        NumPackets need = ifsHandle->entry.virtWhere - bestWhere + 1;

        // Adjust by one to prevent getting one extra packet which contains start of P frame
        // for trick modes which just want i frame
        if (ifsReadType == IfsReadTypeNearest)
        {
            need--;
        }

        // Determine number of bytes which constitutes picture to be returned
        const size_t bytes = need * sizeof(IfsPacket);

        RILOG_TRACE(" Return data from entry %ld in file %2ld to entry %ld in file %2ld\n",
                bestEntry, bestFile, nextEntry, nextFile);
        RILOG_TRACE(" Return data from packet %ld to %ld (%ld packets, %d bytes)\n",
                bestWhere, ifsHandle->entry.virtWhere, need, bytes);

        // If pointer to start packet is not null, set it to point to best entry's packet position
        if (pStartPacket)
            *pStartPacket = bestWhere;

        // Allocate memory required based on calculated number of bytes
        *ppData = g_try_malloc(bytes);
        if (*ppData == NULL)
        {
            RILOG_CRIT(
                    "IfsReturnCodeMemAllocationError: *ppData == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return IfsReturnCodeMemAllocationError;
        }

        // Initialize counter of number of packets read, starting at best packet position
        NumPackets have = 0;
        while (IfsTrue)
        {
            // File seek to best packet position
            NumPackets got;

            g_static_mutex_unlock(&(ifsHandle->mutex));
            ifsReturnCode = IfsSeekToPacketImpl(ifsHandle, bestWhere, NULL);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                // Unable to go to that packet position, bailing out
                break;

            g_static_mutex_lock(&(ifsHandle->mutex));

            // Read number of packets still needed (??? could we be off by 1 here???)
            if ((got = fread(*ppData + have, sizeof(IfsPacket), need - have,
                    ifsHandle->pMpeg)) == 0)
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileReadingError: fread(%p, %d, %ld, %s) in line %d of %s\n",
                        *ppData, sizeof(IfsPacket), need, ifsHandle->mpeg,
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileReadingError;
                break;
            }

            // Increment number of packets read so far based on number which was just read
            have += got;

            // Have read all packets that are needed?
            if (have == need)
                break;

            // Unable to get all needed packets when read starts at this packet position,
            // Need to increment packet position and read again
            // How can we be sure we are not crossing file or entry boundary????
            RILOG_TRACE(" Crossing file boundary, still need %ld packets\n",
                    need - have);
            bestWhere += got;
        }

        // If bailed out of loop due to unsuccessful read, free allocated memory and return
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
        {
            if (*ppData)
            {
                g_free(*ppData);
                *ppData = NULL;
            }

            g_static_mutex_unlock(&(ifsHandle->mutex));
            return ifsReturnCode;
        }

        // Return number of packet which have been read
        *pNumPackets = have;

        // Update current location so any subsequent reads will be correctly tracked
        ifsHandle->realLoc += *pNumPackets;
        ifsHandle->virtLoc += *pNumPackets;
    }
    // Out of loop but never found start of I-Frame, log and return error
    else if (lookingForIframe)
    {
        RILOG_WARN("IfsReturnCodeIframeStartNotFound in line %d of %s\n",
                __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeIframeStartNotFound;
    }
    // End of IFrame not found but found start of IFrame, got here because got to end of files
    else
    {
        // If start of IFrame was found but not end of IFrame, look in previous file
        // for a complete IFrame
        if ((ifsReadType == IfsReadTypeNearest) && // Should not fail if a previous I frame was found
                (prevDiff != IFS_UNDEFINED_CLOCK) && // A previous I frame was found
                (bestDiff == nextDiff)) // Just got done trying the next I frame and if failed
        {
            bestDiff = prevDiff; // Setup to try the previous I frame
            bestEntry = prevEntry; //
            bestWhere = prevWhere; //
            bestFile = prevFile; //

            prevDiff = IFS_UNDEFINED_CLOCK; // This will prevent an infinite loop

            RILOG_INFO(
                    " Next entry failed, try the previous entry (%ld in file %2ld)\n",
                    bestEntry, bestFile);

            goto TryAgain;
        }

        RILOG_WARN("IfsReturnCodeIframeEndNotFound in line %d of %s\n",
                __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeIframeEndNotFound;
    }

    g_static_mutex_unlock(&(ifsHandle->mutex));
    return IfsReturnCodeNoErrorReported;
}

IfsReturnCode IfsReadNearestPicture // Must call IfsSeekToTime() before calling this function
(IfsHandle ifsHandle, // Input
        IfsPcr ifsPcr, // Input
        IfsPts ifsPts, // Input
        NumPackets * pNumPackets, // Output
        IfsPacket ** ppData // Output
)
{
    return IfsReadPicture(ifsHandle, ifsPcr, ifsPts, IfsReadTypeNearest,
            pNumPackets, ppData, NULL);
}

IfsReturnCode IfsReadNextPicture // Must call IfsSeekToTime() before calling this function
(IfsHandle ifsHandle, // Input
        IfsPcr ifsPcr, // Input
        IfsPts ifsPts, // Input
        NumPackets * pNumPackets, // Output
        IfsPacket ** ppData // Output
)
{
    return IfsReadPicture(ifsHandle, ifsPcr, ifsPts, IfsReadTypeNext,
            pNumPackets, ppData, NULL);
}

IfsReturnCode IfsReadPreviousPicture // Must call IfsSeekToTime() before calling this function
(IfsHandle ifsHandle, // Input
        IfsPcr ifsPcr, // Input
        IfsPts ifsPts, // Input
        NumPackets * pNumPackets, // Output
        IfsPacket ** ppData // Output
)
{
    return IfsReadPicture(ifsHandle, ifsPcr, ifsPts, IfsReadTypePrevious,
            pNumPackets, ppData, NULL);
}

