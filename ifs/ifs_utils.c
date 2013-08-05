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

#define _IFS_UTILS_C "$Rev: 141 $"

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
#include "ifs_h262_parse.h"
#include "ifs_h264_parse.h"
#include "ifs_h265_parse.h"

extern log4c_category_t * ifs_RILogCategory;
#define RILOG_CATEGORY ifs_RILogCategory

extern IfsIndexDumpMode indexDumpMode;
extern ullong whatAll;
extern unsigned indexCase;
extern unsigned ifsFileChunkSize;


// Utility operations:

ullong IfsGetWhatAll(void)
{
    return whatAll;
}

const char * IfsReturnCodeToString(const IfsReturnCode ifsReturnCode)
{
    switch (ifsReturnCode)
    {
    case IfsReturnCodeNoErrorReported:
        return "No Error Reported";
    case IfsReturnCodeBadInputParameter:
        return "Bad Input Parameter";
    case IfsReturnCodeBadMaxSizeValue:
        return "Bad MaxSize Value";
    case IfsReturnCodeMemAllocationError:
        return "Mem Allocation Error";
    case IfsReturnCodeIllegalOperation:
        return "Illegal Operation";
    case IfsReturnCodeMustBeAnIfsWriter:
        return "Must be an IFS writer";
    case IfsReturnCodeSprintfError:
        return "Sprintf Error";
    case IfsReturnCodeStatError:
        return "Stat Error";
    case IfsReturnCodeMakeDirectoryError:
        return "Make Directory Error";
    case IfsReturnCodeOpenDirectoryError:
        return "Open Directory Error";
    case IfsReturnCodeCloseDirectoryError:
        return "Close Directory Error";
    case IfsReturnCodeDeleteDirectoryError:
        return "Delete Directory Error";
    case IfsReturnCodeFileOpeningError:
        return "File Opening Error";
    case IfsReturnCodeFileSeekingError:
        return "File Seeking Error";
    case IfsReturnCodeFileWritingError:
        return "File Writing Error";
    case IfsReturnCodeFileReadingError:
        return "File Reading Error";
    case IfsReturnCodeFileFlushingError:
        return "File Flushing Error";
    case IfsReturnCodeFileClosingError:
        return "File Closing Error";
    case IfsReturnCodeFileDeletingError:
        return "File Deleting Error";
    case IfsReturnCodeFileWasNotFound:
        return "File Was Not Found";
    case IfsReturnCodeReadPastEndOfFile:
        return "Read Past End Of File";
    case IfsReturnCodeSeekOutsideFile:
        return "Seek Outside File";
    case IfsReturnCodeIframeNotFound:
        return "I Frame Not Found";
    case IfsReturnCodeFoundPbeforeIframe:
        return "Found P Before I Frame";
    case IfsReturnCodeFoundBbeforeIframe:
        return "Found B Before I Frame";
    case IfsReturnCodeIframeStartNotFound:
        return "I Frame Start Not Found";
    case IfsReturnCodeIframeEndNotFound:
        return "I Frame End Not Found";
    }
    return "unknown";
}

char * IfsToSecs(const IfsClock ifsClock, char * const temp) // temp[] must be at least 23 characters, returns temp
{
    // Largest number is -18446744073.709551615 = 22 characters

    if ((llong) ifsClock < 0)
    {
        long secs = -((llong) ifsClock / (llong) NSEC_PER_SEC);
        long nsec = -((llong) ifsClock % (llong) NSEC_PER_SEC);

        if (secs)
        {
            if (nsec)
                sprintf(temp, "%5ld.%09ld", -secs, nsec);
            else
                sprintf(temp, "%5ld", -secs);
        }
        else
        {
            if (nsec)
                sprintf(temp, "   -0.%09ld", nsec);
            else
                sprintf(temp, "   -0");
        }
    }
    else
    {
        long secs = ifsClock / NSEC_PER_SEC;
        long nsec = ifsClock % NSEC_PER_SEC;

        if (nsec)
            sprintf(temp, "%5ld.%09ld", secs, nsec);
        else
            sprintf(temp, "%5ld", secs);
    }

    return temp;
}

char * IfsLongLongToString(ullong value, char * const temp) // temp[] must be at least 27 characters, returns temp
{
    // Largest number is 18,446,744,073,709,551,615 = 26 characters
    // Largest divide is  1,000,000,000,000,000,000

    IfsBoolean zeros = IfsFalse;
    int where = 0;

#define TestAndPrint(div)                     \
    if (value >= div)                             \
    {                                             \
        where += sprintf(&temp[where],            \
                         zeros ? "%03d," : "%d,", \
                         (short)(value/div));     \
        value %= div;                             \
        zeros = IfsTrue;                          \
    }                                             \
    else if (zeros)                               \
        where += sprintf(&temp[where], "000,");

    TestAndPrint(1000000000000000000LL)
    TestAndPrint(1000000000000000LL )
    TestAndPrint(1000000000000LL )
    TestAndPrint(1000000000LL )
    TestAndPrint(1000000LL )
    TestAndPrint(1000LL )

#undef TestAndPrint

    sprintf(&temp[where], zeros ? "%03d" : "%d", (short) value);

    return temp;
}

void IfsDumpInfo(const IfsInfo * const pIfsInfo)
{
    char temp[32];

    RILOG_INFO("pIfsInfo->path     %s\n", pIfsInfo->path);
    RILOG_INFO("pIfsInfo->name     %s\n", pIfsInfo->name);
    RILOG_INFO("pIfsInfo->mpegSize %s\n", IfsLongLongToString(
            pIfsInfo->mpegSize, temp));
    RILOG_INFO("pIfsInfo->ndexSize %s\n", IfsLongLongToString(
            pIfsInfo->ndexSize, temp));
    RILOG_INFO("pIfsInfo->begClock %s\n", IfsToSecs(pIfsInfo->begClock, temp));
    RILOG_INFO("pIfsInfo->endClock %s\n", IfsToSecs(pIfsInfo->endClock, temp));
    RILOG_INFO("pIfsInfo->maxSize  %ld\n", pIfsInfo->maxSize);
}

void IfsDumpHandle(const IfsHandle ifsHandle)
{
    char temp[256]; // Used by ParseWhat
    char* (*parseWhat)(IfsHandle ifsHandle, char * temp,
        const IfsIndexDumpMode ifsIndexDumpMode, const IfsBoolean) = NULL;
    void (*dumpHandle)(const IfsHandle ifsHandle) = NULL;

    switch (ifsHandle->codecType)
    {
        case IfsCodecTypeH261:
        case IfsCodecTypeH262:
        case IfsCodecTypeH263:
            parseWhat = ifsHandle->codec->h262->ParseWhat;
            dumpHandle = ifsHandle->codec->h262->DumpHandle;
            break;
        case IfsCodecTypeH264:
            parseWhat = ifsHandle->codec->h264->ParseWhat;
            dumpHandle = ifsHandle->codec->h264->DumpHandle;
            break;
        case IfsCodecTypeH265:
            parseWhat = ifsHandle->codec->h265->ParseWhat;
            dumpHandle = ifsHandle->codec->h265->DumpHandle;
            break;
        default:
            RILOG_ERROR("IfsReturnCodeBadInputParameter: ifsHandle->codec "
                        "not set in line %d of %s\n", __LINE__, __FILE__);
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return;
    }

    g_static_mutex_lock(&(ifsHandle->mutex));
    RILOG_INFO("ifsHandle->path             %s\n", ifsHandle->path); // char *
    RILOG_INFO("ifsHandle->name             %s\n", ifsHandle->name); // char *
    RILOG_INFO("ifsHandle->mpegSize         %s\n", IfsLongLongToString //
            (ifsHandle->mpegSize, temp)); // NumBytes
    RILOG_INFO("ifsHandle->ndexSize         %s\n", IfsLongLongToString //
            (ifsHandle->ndexSize, temp)); // NumBytes
    RILOG_INFO("ifsHandle->both             %s\n", ifsHandle->both); // char *
    RILOG_INFO("ifsHandle->mpeg             %s\n", ifsHandle->mpeg); // char *
    RILOG_INFO("ifsHandle->ndex             %s\n", ifsHandle->ndex); // char *
    RILOG_INFO("ifsHandle->begFileNumber    %ld\n", ifsHandle->begFileNumber); // FileNumber
    RILOG_INFO("ifsHandle->endFileNumber    %ld\n", ifsHandle->endFileNumber); // FileNumber
    RILOG_INFO("ifsHandle->pMpeg            %p\n", ifsHandle->pMpeg); // FILE *
    RILOG_INFO("ifsHandle->pNdex            %p\n", ifsHandle->pNdex); // FILE *
    RILOG_INFO("ifsHandle->realLoc          %ld\n", ifsHandle->realLoc); // NumPackets
    RILOG_INFO("ifsHandle->virtLoc          %ld\n", ifsHandle->virtLoc); // NumPackets
    RILOG_INFO("ifsHandle->begClock         %s\n", IfsToSecs //
            (ifsHandle->begClock, temp)); // IfsClock
    RILOG_INFO("ifsHandle->endClock         %s\n", IfsToSecs //
            (ifsHandle->endClock, temp)); // IfsClock
    RILOG_INFO("ifsHandle->nxtClock         %s\n", IfsToSecs //
            (ifsHandle->nxtClock, temp)); // IfsClock
    RILOG_INFO("ifsHandle->entry.when       %s\n", IfsToSecs //
            (ifsHandle->entry.when, temp)); // IfsClock
    RILOG_INFO("ifsHandle->entry.what       %s\n", parseWhat(ifsHandle, temp, //
            IfsIndexDumpModeDef, //
            IfsFalse)); // IfsIndex
    RILOG_INFO("ifsHandle->entry.realWhere  %ld\n", ifsHandle->entry.realWhere); // NumPackets
    RILOG_INFO("ifsHandle->entry.virtWhere  %ld\n", ifsHandle->entry.virtWhere); // NumPackets
    RILOG_INFO("ifsHandle->maxPacket        %ld\n", ifsHandle->maxPacket); // NumPackets
    RILOG_INFO("ifsHandle->curFileNumber    %ld\n", ifsHandle->curFileNumber); // FileNumber
    RILOG_INFO("ifsHandle->entryNum         %ld\n", ifsHandle->entryNum); // NumEntries
    RILOG_INFO("ifsHandle->maxEntry         %ld\n", ifsHandle->maxEntry); // NumEntries
    RILOG_INFO("ifsHandle->appendFileNumber %ld\n", ifsHandle->appendFileNumber); // FileNumber
    RILOG_INFO("ifsHandle->appendPacketNum  %ld\n", ifsHandle->appendPacketNum); // NumPackets
    RILOG_INFO("ifsHandle->appendEntryNum   %ld\n", ifsHandle->appendEntryNum); // NumEntries
    RILOG_INFO("ifsHandle->appendIndexShift %ld\n", ifsHandle->appendIndexShift); // NumPackets
    RILOG_INFO("ifsHandle->maxSize          %ld\n", ifsHandle->maxSize); // IfsTime
    g_static_mutex_unlock(&(ifsHandle->mutex));
    dumpHandle(ifsHandle);
}

void IfsSetMode(const IfsIndexDumpMode ifsIndexDumpMode,
        const ullong ifsIndexerSetting)
{
    indexDumpMode = ifsIndexDumpMode;
    SetIndexer(ifsIndexerSetting);
}

IfsReturnCode IfsFreeInfo(IfsInfo * pIfsInfo // Input
)
{
    if (pIfsInfo == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: pIfsInfo == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    if (pIfsInfo->codec)
    {
        g_free(pIfsInfo->codec);
        pIfsInfo->codec = NULL;
    }
    if (pIfsInfo->path)
    {
        g_free(pIfsInfo->path);
        pIfsInfo->path = NULL;
    }
    if (pIfsInfo->name)
    {
        g_free(pIfsInfo->name);
        pIfsInfo->name = NULL;
    }
    g_free(pIfsInfo);
    pIfsInfo = NULL;

    return IfsReturnCodeNoErrorReported;
}

IfsReturnCode IfsHandleInfo(IfsHandle ifsHandle, // Input
        IfsInfo ** ppIfsInfo // Output (use IfsFreeInfo() to g_free)
)
{
    IfsReturnCode ifsReturnCode;
    IfsBoolean retry = IfsTrue;

    if (ppIfsInfo == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ppIfsInfo == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    else
        *ppIfsInfo = NULL;

    if (ifsHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ifsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    g_static_mutex_lock(&(ifsHandle->mutex));

    do
    {
        ifsReturnCode = GetCurrentFileParameters(ifsHandle);

        if (ifsReturnCode == IfsReturnCodeNoErrorReported)
        {
            break;
        }
        else if (retry == IfsFalse)
        {
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return ifsReturnCode;
        }
        else
        {
            retry = IfsFalse;
            RILOG_INFO("%s: GetCurrentFileParameters retry at %d of %s\n",
                        __FUNCTION__, __LINE__, __FILE__);
        }

    } while (retry == IfsTrue);


    {   // scope
        IfsInfo * pIfsInfo;

        const size_t pathSize = strlen(ifsHandle->path) + 1;
        const size_t nameSize = strlen(ifsHandle->name) + 1;
        const IfsClock difClock = ifsHandle->endClock - ifsHandle->begClock;

        pIfsInfo = g_try_malloc(sizeof(IfsInfo)); // g_free in IfsFreeInfo()
        if (pIfsInfo == NULL)
        {
            RILOG_CRIT(
                    "IfsReturnCodeMemAllocationError: pIfsInfo == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return IfsReturnCodeMemAllocationError;
        }

        pIfsInfo->codec = g_try_malloc(sizeof(IfsCodecImpl)); // g_free in IfsFreeInfo()
        pIfsInfo->maxSize = ifsHandle->maxSize; // in seconds, 0 = value not used
        pIfsInfo->path = NULL; // filled in below
        pIfsInfo->name = NULL; // filled in below
        pIfsInfo->mpegSize = ifsHandle->mpegSize;
        pIfsInfo->ndexSize = ifsHandle->ndexSize;
        pIfsInfo->begClock = (ifsHandle->maxSize && (difClock
                > ifsHandle->maxSize * NSEC_PER_SEC) ? ifsHandle->endClock
                - ifsHandle->maxSize * NSEC_PER_SEC : ifsHandle->begClock);
        pIfsInfo->endClock = ifsHandle->endClock; // in nanoseconds

        if (ifsHandle->codecType == IfsCodecTypeH262)
        {
            pIfsInfo->codec->h262->videoPid = ifsHandle->codec->h262->videoPid;
            pIfsInfo->codec->h262->audioPid = ifsHandle->codec->h262->audioPid;
        }

        pIfsInfo->path = g_try_malloc(pathSize); // g_free in IfsFreeInfo()
        pIfsInfo->name = g_try_malloc(nameSize); // g_free in IfsFreeInfo()

        if (pIfsInfo->path == NULL)
        {
            (void) IfsFreeInfo(pIfsInfo); // Ignore any errors, we already have one...
            RILOG_CRIT(
                    "IfsReturnCodeMemAllocationError: pIfsInfo->path == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return IfsReturnCodeMemAllocationError;
        }

        if (pIfsInfo->name == NULL)
        {
            (void) IfsFreeInfo(pIfsInfo); // Ignore any errors, we already have one...
            RILOG_CRIT(
                    "IfsReturnCodeMemAllocationError: pIfsInfo->name == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return IfsReturnCodeMemAllocationError;
        }

        memcpy(pIfsInfo->path, ifsHandle->path, pathSize);
        memcpy(pIfsInfo->name, ifsHandle->name, nameSize);

        *ppIfsInfo = pIfsInfo;
    }

    g_static_mutex_unlock(&(ifsHandle->mutex));
    return IfsReturnCodeNoErrorReported;
}

