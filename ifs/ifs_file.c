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

#ifdef WIN32
#define makedir(p) mkdir(p)
#else
#define makedir(p) mkdir(p,0777)
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
#include "ifs_h262_parse.h"
#include "ifs_h264_parse.h"
#include "ifs_h265_parse.h"

extern log4c_category_t * ifs_RILogCategory;
#define RILOG_CATEGORY ifs_RILogCategory

unsigned ifsFileChunkSize = 600;

// Path/Name operations:
// WARNING: it is assumed that the ifsHandle->mutex is locked outside this func.
IfsReturnCode GenerateFileNames(IfsHandle ifsHandle,
        FileNumber fileNumber, char ** const pMpeg, char ** const pNdex)
{
    // The maximum file name value is 4294967295
    //
    // If each file is only one SECOND of recording time then this is:
    //
    //      4,294,967,295 seconds
    //         71,582,788 minutes
    //          1,193,046 hours
    //             49,710 days
    //                136 years
    //
    // of continuous recording time in the TSB.
    //
    // And more resonable values for the amount of time stored in each file
    // will provide an even larger possible recording range.  So all is well.

    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

    const size_t bothSize = strlen(ifsHandle->both); // path + / + name
    const size_t fullSize = bothSize + 16; // path + / + name [ + / + <10> + . + <3> + \000 ] = +16

    if (*pMpeg)
        g_free(*pMpeg);
    *pMpeg = g_try_malloc(fullSize);

    if (*pNdex)
        g_free(*pNdex);
    *pNdex = g_try_malloc(fullSize);

    if (*pMpeg == NULL)
    {
        RILOG_CRIT(
                "IfsReturnCodeMemAllocationError: *pMpeg == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        ifsReturnCode = IfsReturnCodeMemAllocationError;
    }

    if (*pNdex == NULL)
    {
        RILOG_CRIT(
                "IfsReturnCodeMemAllocationError: *pNdex == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        ifsReturnCode = IfsReturnCodeMemAllocationError;
    }

    if (ifsReturnCode == IfsReturnCodeNoErrorReported)
    {
        memcpy(*pMpeg, ifsHandle->both, bothSize);
        memcpy(*pMpeg + bothSize, "/", 1);
        if (sprintf(*pMpeg + bothSize + 1, "%010lu", fileNumber) == 10) // max value is 4294967295
        {
            memcpy(*pNdex, *pMpeg, fullSize);
            strcat(*pMpeg, ".mpg");
            strcat(*pNdex, ".ndx");
            return IfsReturnCodeNoErrorReported;
        }

        RILOG_ERROR(
                "IfsReturnCodeSprintfError: sprintf() != 10 in line %d of %s\n",
                __LINE__, __FILE__);
        ifsReturnCode = IfsReturnCodeSprintfError;
    }

    if (*pMpeg)
    {
        g_free(*pMpeg);
        *pMpeg = NULL;
    }
    if (*pNdex)
    {
        g_free(*pNdex);
        *pNdex = NULL;
    }

    return ifsReturnCode;
}

// WARNING: it is assumed that the ifsHandle->mutex is locked outside this func.
IfsReturnCode CloseActualFiles(IfsHandle ifsHandle)
{
    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

    ifsHandle->curFileNumber = 0;

    if (ifsHandle->pMpeg)
    {
        if (fflush(ifsHandle->pMpeg))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileFlushingError: fflush(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->mpeg, errno, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileFlushingError;
        }
        if (fclose(ifsHandle->pMpeg))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileClosingError: fclose(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->mpeg, errno, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileClosingError;
        }
        ifsHandle->pMpeg = NULL;
    }

    if (ifsHandle->pNdex)
    {
        if (fflush(ifsHandle->pNdex))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileFlushingError: fflush(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->ndex, errno, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileFlushingError;
        }
        if (fclose(ifsHandle->pNdex))
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileClosingError: fclose(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->ndex, errno, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileClosingError;
        }
        ifsHandle->pNdex = NULL;
    }

    return ifsReturnCode;
}

// WARNING: it is assumed that the ifsHandle->mutex is locked outside this func.
IfsReturnCode IfsOpenActualFiles(IfsHandle ifsHandle, FileNumber fileNumber,
        const char * const mode)
{
    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

    (void) CloseActualFiles(ifsHandle); // Don't care about errors here

    ifsReturnCode = GenerateFileNames(ifsHandle, fileNumber, &ifsHandle->mpeg,
            &ifsHandle->ndex);
    if (ifsReturnCode == IfsReturnCodeNoErrorReported)
    {
        ifsHandle->pMpeg = fopen(ifsHandle->mpeg, mode);
        ifsHandle->pNdex = fopen(ifsHandle->ndex, mode);

        if (ifsHandle->pMpeg == NULL)
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileOpeningError: fopen(%s, \"%s\") failed (%d: %s) in line %d of %s\n",
                    ifsHandle->mpeg, mode, errno, strerror(errno), __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileOpeningError;
        }
        if (ifsHandle->pNdex == NULL)
        {
            RILOG_ERROR(
                    "IfsReturnCodeFileOpeningError: fopen(%s, \"%s\") failed (%d: %s) in line %d of %s\n",
                    ifsHandle->ndex, mode, errno, strerror(errno), __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeFileOpeningError;
        }
    }

    if (ifsReturnCode != IfsReturnCodeNoErrorReported) // Clean up and report the error
    {
        (void) CloseActualFiles(ifsHandle); // Don't care about errors here
    }
    else
        ifsHandle->curFileNumber = fileNumber;

    return ifsReturnCode;
}

// WARNING: it is assumed that the ifsHandle->mutex is locked outside this func.
IfsReturnCode GetCurrentFileSizeAndCount(IfsHandle ifsHandle)
{
    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

    struct stat statBuffer;
    struct dirent * pDirent;
    DIR * pDir = opendir(ifsHandle->both);
    if (pDir == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeOpenDirectoryError: opendir(%s) failed (%d) in line %d of %s\n",
                ifsHandle->both, errno, __LINE__, __FILE__);
        return IfsReturnCodeOpenDirectoryError;
    }

    ifsHandle->mpegSize = 0;
    ifsHandle->ndexSize = 0;
    ifsHandle->begFileNumber = 0xFFFFFFFF;
    ifsHandle->endFileNumber = 0x00000000;

    while ((pDirent = readdir(pDir)) != NULL)
    {
        FileNumber newFileNumber;
        char temp[1024]; // path and filename

        if (sscanf(pDirent->d_name, "%010lu.%s", &newFileNumber, temp))
        {
            IfsBoolean isMpeg = !strcmp(temp, "mpg");
            IfsBoolean isNdex = !strcmp(temp, "ndx");

            if (newFileNumber > ifsHandle->endFileNumber)
                ifsHandle->endFileNumber = newFileNumber;
            if (newFileNumber < ifsHandle->begFileNumber)
                ifsHandle->begFileNumber = newFileNumber;

            strcpy(temp, ifsHandle->both);
            strcat(temp, "/");
            strcat(temp, pDirent->d_name);

            if (stat(temp, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        temp, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeStatError;
                break;
            }

            if (isMpeg)
                ifsHandle->mpegSize += (size_t) statBuffer.st_size;
            if (isNdex)
                ifsHandle->ndexSize += (size_t) statBuffer.st_size;
        }
    }

    if (closedir(pDir))
    {
        RILOG_ERROR(
                "IfsReturnCodeCloseDirectoryError: closedir(%s) failed (%d) in line %d of %s\n",
                ifsHandle->both, errno, __LINE__, __FILE__);
        ifsReturnCode = IfsReturnCodeCloseDirectoryError;
    }

    if (ifsHandle->begFileNumber == 0xFFFFFFFF) // did not find any files
    {
        RILOG_ERROR(
                "IfsReturnCodeFileOpeningError: did not find any files in line %d of %s\n",
                __LINE__, __FILE__);
        ifsReturnCode = IfsReturnCodeFileOpeningError;
    }

    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
    {
        ifsHandle->mpegSize = 0;
        ifsHandle->ndexSize = 0;
    }

    return ifsReturnCode;
}

IfsReturnCode IfsPathNameInfo(const char * path, // Input
        const char * name, // Input
        IfsInfo ** ppIfsInfo // Output (use IfsFreeInfo() to g_free)
)
{
    IfsReturnCode ifsReturnCode;
    IfsHandle ifsHandle;

    if (ppIfsInfo == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ppIfsInfo == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    else
        *ppIfsInfo = NULL;

    if (path == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: path == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (name == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: name == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    ifsReturnCode = IfsOpenReader(path, name, &ifsHandle);
    if (ifsReturnCode == IfsReturnCodeNoErrorReported)
    {
        ifsReturnCode = IfsHandleInfo(ifsHandle, ppIfsInfo);
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
        {
            (void) IfsClose(ifsHandle); // Ignore any errors, we already have one...
        }
        else
        {
            ifsReturnCode = IfsClose(ifsHandle);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            {
                (void) IfsFreeInfo(*ppIfsInfo); // Ignore any errors, we already have one...
                *ppIfsInfo = NULL;
            }
        }
    }

    return ifsReturnCode;
}

// WARNING: it is assumed that the ifsHandle->mutex is locked outside this func.
IfsReturnCode IfsReadNdexEntryAt(IfsHandle ifsHandle, NumEntries entry)
{
    if (fseek(ifsHandle->pNdex, entry * sizeof(IfsIndexEntry), SEEK_SET))
    {
        RILOG_ERROR(
                "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) failed (%d) in line %d of %s\n",
                ifsHandle->ndex, entry * sizeof(IfsIndexEntry), errno,
                __LINE__, __FILE__);
        return IfsReturnCodeFileSeekingError;
    }
    if (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry), ifsHandle->pNdex)
            != sizeof(IfsIndexEntry))
    {
        RILOG_ERROR(
                "IfsReturnCodeFileReadingError: fread(%p, 1, %d, %s) failed (%d) in line %d of %s\n",
                &ifsHandle->entry, sizeof(IfsIndexEntry), ifsHandle->ndex,
                errno, __LINE__, __FILE__);
        return IfsReturnCodeFileReadingError;
    }
    return IfsReturnCodeNoErrorReported;
}

// WARNING: it is assumed that the ifsHandle->mutex is locked outside this func.
IfsReturnCode GetCurrentFileParameters(IfsHandle ifsHandle)
{
    // NOTE:  if this is a reader then ignore maxSize
    //
    // Gets the current values of all the following parameters:
    //
    //      ifsHandle->mpegSize      from GetCurrentFileSizeAndCount()
    //      ifsHandle->ndexSize      from GetCurrentFileSizeAndCount()
    //      ifsHandle->begFileNumber from GetCurrentFileSizeAndCount()
    //      ifsHandle->endFileNumber from GetCurrentFileSizeAndCount()
    //      ifsHandle->begClock
    //      ifsHandle->endClock
    //      ifsHandle->nxtClock
    //      ifsHandle->mpeg          from IfsOpenActualFiles()
    //      ifsHandle->ndex          from IfsOpenActualFiles()
    //      ifsHandle->pMpeg         from IfsOpenActualFiles()
    //      ifsHandle->pNdex         from IfsOpenActualFiles()
    //      ifsHandle->curFileNumber from IfsOpenActualFiles()

    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

    struct stat statBuffer;

    // Save the runtime data before performing the necessary operations.
    FileNumber curFile = ifsHandle->curFileNumber;
    long       posMpeg = 0;
    long       posNdex = 0;
    if (curFile != 0)
    {
        posMpeg = ftell(ifsHandle->pMpeg);
        posNdex = ftell(ifsHandle->pNdex);
    }

    ifsHandle->begClock = 0;
    ifsHandle->endClock = 0;
    ifsHandle->nxtClock = 0;

    // This is needed so the size calculation will be accurate
    (void) CloseActualFiles(ifsHandle); // Don't care about errors here

    ifsReturnCode = GetCurrentFileSizeAndCount(ifsHandle);
    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
        return ifsReturnCode;

    // If we get here then ifsHandle->mpegSize, ifsHandle->ndexSize,
    // ifsHandle->begFileNumber and ifsHandle->endFileNumber are all valid

    if (ifsHandle->begFileNumber && ifsHandle->endFileNumber) // The file is circular
    {
        FileNumber fileNumber = ifsHandle->endFileNumber;

        if (!ifsHandle->isReading && !ifsHandle->maxSize) // The handle is not circular
        {
            RILOG_ERROR(
                    "IfsReturnCodeBadMaxSizeValue: ifsHandle should be circular but it is not in line %d of %s\n",
                    __LINE__, __FILE__);
            return IfsReturnCodeBadMaxSizeValue; // Should be circular but it is not
        }

        do // Process the first entry
        {
            ifsReturnCode = IfsOpenActualFiles(ifsHandle,
                    ifsHandle->begFileNumber, "rb+");
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;

            if (stat(ifsHandle->ndex, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeStatError;
                break;
            }

            if ((size_t) statBuffer.st_size >= sizeof(IfsIndexEntry)) // has at least one entry
            {
                ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, 0);
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                    break;

                ifsHandle->begClock = ifsHandle->entry.when; // nanoseconds
            }
            // else this is an empty index file

        } while (0);

        do // Find the last indexing entry
        {
            ifsReturnCode = IfsOpenActualFiles(ifsHandle, fileNumber, "rb+");
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;

            if (stat(ifsHandle->ndex, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeStatError;
                break;
            }

            if ((size_t) statBuffer.st_size >= sizeof(IfsIndexEntry)) // has at least one entry
            {
                ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, 0);
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                    break;

                ifsHandle->nxtClock = ifsHandle->entry.when + (ifsFileChunkSize
                        * NSEC_PER_SEC); // nanoseconds

                ifsReturnCode = IfsReadNdexEntryAt(ifsHandle,
                        ((size_t) statBuffer.st_size / sizeof(IfsIndexEntry)
                                - 1));
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                    break;

                ifsHandle->endClock = ifsHandle->entry.when; // nanoseconds

                break; // DONE with no error
            }
            // else this is an empty index file, try the previous file
        } while (--fileNumber);

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            return ifsReturnCode;

        // If we get here then ifsHandle->mpegSize, ifsHandle->ndexSize,
        // ifsHandle->begFileNumber, ifsHandle->endFileNumber,
        // ifsHandle->nxtClock and ifsHandle->endClock are all valid
    }
    else if (!ifsHandle->begFileNumber && !ifsHandle->endFileNumber) // The file is not circular
    {
        if (!ifsHandle->isReading && ifsHandle->maxSize) // The handle is circular
        {
            RILOG_ERROR(
                    "IfsReturnCodeBadMaxSizeValue: ifsHandle should not be circular but it is in line %d of %s\n",
                    __LINE__, __FILE__);
            return IfsReturnCodeBadMaxSizeValue; // Should be one file but it is not
        }

        do
        {
            ifsReturnCode = IfsOpenActualFiles(ifsHandle,
                    ifsHandle->begFileNumber, "rb+");
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;

            if (stat(ifsHandle->ndex, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeStatError;
                break;
            }

            if ((size_t) statBuffer.st_size >= sizeof(IfsIndexEntry)) // has at least one entry
            {
                ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, 0);
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                    break;

                ifsHandle->begClock = ifsHandle->entry.when; // nanoseconds

                ifsReturnCode = IfsReadNdexEntryAt(ifsHandle,
                        ((size_t) statBuffer.st_size / sizeof(IfsIndexEntry)
                                - 1));
                if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                    break;

                ifsHandle->endClock = ifsHandle->entry.when; // nanoseconds
            }

        } while (0);
    }
    else
    {
        RILOG_ERROR(
                "IfsReturnCodeBadMaxSizeValue: ifsHandle is corrupted in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    // Restore previously saved runtime parameters.
    if (curFile != 0)
    {
        ifsReturnCode = IfsOpenActualFiles(ifsHandle, curFile, "rb+");
        if (ifsReturnCode == IfsReturnCodeNoErrorReported)
        {
            if (fseek(ifsHandle->pMpeg, posMpeg, SEEK_SET))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) failed (%d) in line %d of %s\n",
                        ifsHandle->mpeg, posMpeg, errno, __LINE__, __FILE__);
                return IfsReturnCodeFileSeekingError;
            }
            if (fseek(ifsHandle->pNdex, posNdex, SEEK_SET))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, posNdex, errno, __LINE__, __FILE__);
                return IfsReturnCodeFileSeekingError;
            }
        }
    }

    return ifsReturnCode;
}

IfsReturnCode IfsSeekToTimeImpl // Must call GetCurrentFileParameters() before calling this function
(IfsHandle ifsHandle, // Input
        IfsDirect ifsDirect, // Input  either IfsDirectBegin,
        //        IfsDirectEnd or IfsDirectEither
        IfsClock * pIfsClock, // Input  requested/Output actual, in nanoseconds
        NumPackets * pPosition // Output packet position, optional, can be NULL
)
{
#ifdef STAND_ALONE
#ifdef DEBUG_SEEKS
    char temp1[32], temp2[32], temp3[32]; // IfsToSecs only
#else
#ifdef DEBUG_ERROR_LOGS
    char temp1[32], temp2[32]; // IfsToSecs only
#endif
#endif
#else // not STAND_ALONE
#ifdef DEBUG_SEEKS
    char temp1[32], temp2[32], temp3[32]; // IfsToSecs only
#else
    char temp1[32], temp2[32]; // IfsToSecs only
#endif
#endif

    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;
    IfsClock ifsClock = *pIfsClock;
    g_static_mutex_lock(&(ifsHandle->mutex));

    if (ifsClock < ifsHandle->begClock)
    {
        RILOG_WARN(
                "IfsReturnCodeSeekOutsideFile: %s < %s (begClock) in line %d of %s\n",
                IfsToSecs(ifsClock, temp1), IfsToSecs(ifsHandle->begClock,
                        temp2), __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeSeekOutsideFile;
    }

    if (ifsClock > ifsHandle->endClock)
    {
        RILOG_WARN(
                "IfsReturnCodeSeekOutsideFile: %s > %s (endClock) in line %d of %s\n",
                IfsToSecs(ifsClock, temp1), IfsToSecs(ifsHandle->endClock,
                        temp2), __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeSeekOutsideFile;
    }

    for (ifsHandle->curFileNumber = ifsHandle->begFileNumber; ifsHandle->curFileNumber
            <= ifsHandle->endFileNumber; ifsHandle->curFileNumber++)
    {
        IfsClock begClock;
        IfsClock endClock;
        struct stat statBuffer;

        g_static_mutex_unlock(&(ifsHandle->mutex));
        ifsReturnCode = IfsOpenActualFiles(ifsHandle, ifsHandle->curFileNumber,
                "rb+");
        g_static_mutex_lock(&(ifsHandle->mutex));

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        if (stat(ifsHandle->ndex, &statBuffer))
        {
            RILOG_ERROR(
                    "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->ndex, errno, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeStatError;
            break;
        }

        ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, 0);
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        begClock = ifsHandle->entry.when - 1; // nanoseconds

        ifsReturnCode = IfsReadNdexEntryAt(ifsHandle,
                ((size_t) statBuffer.st_size / sizeof(IfsIndexEntry) - 1));
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        endClock = ifsHandle->entry.when + 1; // nanoseconds

#ifdef DEBUG_SEEKS
        RILOG_INFO("%s <= %s <= %s ?\n",
                IfsToSecs(begClock, temp1),
                IfsToSecs(ifsClock, temp2),
                IfsToSecs(endClock, temp3));
#endif

        if (ifsClock <= endClock)
        {
            // Found the file!
            //
            // entry = m(x - b)
            //
            // x = ifsClock(nsec)
            //
            // b = begTime(nsec)
            //
            // m = rise / run
            //   = numEntries(entries) / (endClock(nsec) - begClock(nsec))
            //
            // entry = [numEntries(entries) / (endClock(nsec) - begClock(nsec))][locations(nsec) - begClock(nsec)]
            //       = numEntries(entries)[locations(nsec) - begClock(nsec)] / [(endClock(nsec) - begClock(nsec))]

            llong diff;
            NumEntries entry;

            ifsHandle->maxEntry = (size_t) statBuffer.st_size
                    / sizeof(IfsIndexEntry) - 1;

            entry = (ifsClock <= begClock ? 0 : ifsHandle->maxEntry * (ifsClock
                    - begClock) / (endClock - begClock));

            ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, entry);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;

            diff = ifsClock - ifsHandle->entry.when; // nanoseconds

#ifdef DEBUG_SEEKS
            RILOG_INFO("\nSeek to file number %3ld about entry %3ld want %s got %s diff %s\n",
                    ifsHandle->curFileNumber,
                    entry,
                    IfsToSecs(ifsClock, temp1),
                    IfsToSecs(ifsHandle->entry.when, temp2),
                    IfsToSecs(diff, temp3));
#endif

            if (diff > 0) // scan forward for best seek position
            {
                while (entry < ifsHandle->maxEntry)
                {
                    llong temp;

                    ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, ++entry);
                    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                        break;

                    temp = ifsClock - ifsHandle->entry.when; // nanoseconds
#ifdef DEBUG_SEEKS
                    RILOG_INFO("                          Try entry %3ld want %s got %s diff %s\n",
                            entry,
                            IfsToSecs(ifsClock, temp1),
                            IfsToSecs(ifsHandle->entry.when, temp2),
                            IfsToSecs(temp, temp3));
#endif
                    if (temp >= 0)
                    {
                        // Must be better (or the same), keep trying (unless done)
                        diff = temp;
                    }
                    else
                    {
                        // We are done here but which one is better?
                        if (-temp < diff)
                        {
                            // The negative one is better
                            diff = temp;
                        }
                        else // The old positive one was better
                        {
                            entry--;
                        }
                        break;
                    }
                }

                if (ifsDirect == IfsDirectBegin)
                {
#ifdef DEBUG_SEEKS
                    RILOG_INFO("                          Try backup\n");
#endif
                    while (entry)
                    {
                        llong temp;

                        ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, --entry);
                        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                            break;

                        temp = ifsClock - ifsHandle->entry.when; // nanoseconds
#ifdef DEBUG_SEEKS
                        RILOG_INFO("                          Try entry %3ld want %s got %s diff %s\n",
                                entry,
                                IfsToSecs(ifsClock, temp1),
                                IfsToSecs(ifsHandle->entry.when, temp2),
                                IfsToSecs(temp, temp3));
#endif
                        if (temp != diff) // Moved back as far as we can go
                        {
                            entry++;
                            break;
                        }
                    }
                }
            }
            else if (diff < 0)
            {
                while (entry)
                {
                    llong temp;

                    ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, --entry);
                    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                        break;

                    temp = ifsClock - ifsHandle->entry.when; // nanoseconds
#ifdef DEBUG_SEEKS
                    RILOG_INFO("                          Try entry %3ld want %s got %s diff %s\n",
                            entry,
                            IfsToSecs(ifsClock, temp1),
                            IfsToSecs(ifsHandle->entry.when, temp2),
                            IfsToSecs(temp, temp3));
#endif
                    if (temp <= 0)
                    {
                        // Must be better (or the same), keep trying (unless done)
                        diff = temp;
                    }
                    else
                    {
                        // We are done here but which one is better?
                        if (temp < -diff)
                        {
                            // The positive one is better
                            diff = temp;
                        }
                        else // The old negative one was better
                        {
                            entry++;
                        }
                        break;
                    }
                }

                if (ifsDirect == IfsDirectEnd)
                {
#ifdef DEBUG_SEEKS
                    RILOG_INFO("                          Try forward\n");
#endif
                    while (entry < ifsHandle->maxEntry)
                    {
                        llong temp;

                        ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, ++entry);
                        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                            break;

                        temp = ifsClock - ifsHandle->entry.when; // nanoseconds
#ifdef DEBUG_SEEKS
                        RILOG_INFO("                          Try entry %3ld want %s got %s diff %s\n",
                                entry,
                                IfsToSecs(ifsClock, temp1),
                                IfsToSecs(ifsHandle->entry.when, temp2),
                                IfsToSecs(temp, temp3));
#endif
                        if (temp != diff) // Moved forward as far as we can go
                        {
                            entry--;
                            break;
                        }
                    }
                }
            }
            else
            {
                switch (ifsDirect)
                {
                case IfsDirectBegin:
#ifdef DEBUG_SEEKS
                    RILOG_INFO("                          Try backup\n");
#endif
                    while (entry)
                    {
                        llong temp;

                        ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, --entry);
                        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                            break;

                        temp = ifsClock - ifsHandle->entry.when; // nanoseconds
#ifdef DEBUG_SEEKS
                        RILOG_INFO("                          Try entry %3ld want %s got %s diff %s\n",
                                entry,
                                IfsToSecs(ifsClock, temp1),
                                IfsToSecs(ifsHandle->entry.when, temp2),
                                IfsToSecs(temp, temp3));
#endif
                        if (temp != diff) // Moved back as far as we can go
                        {
                            entry++;
                            break;
                        }
                    }
                    break;

                case IfsDirectEnd:
#ifdef DEBUG_SEEKS
                    RILOG_INFO("                          Try forward %ld\n",
                            ifsHandle->maxEntry);
#endif
                    while (entry < ifsHandle->maxEntry)
                    {
                        llong temp;

                        ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, ++entry);
                        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                            break;

                        temp = ifsClock - ifsHandle->entry.when; // nanoseconds
#ifdef DEBUG_SEEKS
                        RILOG_INFO("                          Try entry %3ld want %s got %s diff %s\n",
                                entry,
                                IfsToSecs(ifsClock, temp1),
                                IfsToSecs(ifsHandle->entry.when, temp2),
                                IfsToSecs(temp, temp3));
#endif
                        if (temp != diff) // Moved forward as far as we can go
                        {
                            entry--;
                            break;
                        }
                    }
                    break;

                case IfsDirectEither:
                    break;
                }
            }

            ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, entry);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;
            if (fseek(ifsHandle->pNdex, entry * sizeof(IfsIndexEntry), SEEK_SET))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, entry * sizeof(IfsIndexEntry), errno,
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileSeekingError;
                break;
            }

#ifdef DEBUG_SEEKS
            RILOG_INFO("                      Best is entry %3ld want %s got %s diff %s\n",
                    entry,
                    IfsToSecs(ifsClock, temp1),
                    IfsToSecs(ifsHandle->entry.when, temp2),
                    IfsToSecs(diff, temp3));
            RILOG_INFO("                                        when %s where %ld/%ld\n",
                    IfsToSecs(ifsHandle->entry.when, temp1),
                    ifsHandle->entry.realWhere,
                    ifsHandle->entry.virtWhere);
#endif

            if (fseek(ifsHandle->pMpeg, ifsHandle->entry.realWhere
                    * sizeof(IfsPacket), SEEK_SET))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) failed (%d) in line %d of %s\n",
                        ifsHandle->mpeg, ifsHandle->entry.realWhere
                                * sizeof(IfsPacket), errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileSeekingError;
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

            ifsHandle->maxPacket = (size_t) statBuffer.st_size
                    / sizeof(IfsPacket) - 1;
            ifsHandle->realLoc = ifsHandle->entry.realWhere;
            ifsHandle->virtLoc = ifsHandle->entry.virtWhere;
            ifsHandle->entryNum = entry;

            *pIfsClock = ifsHandle->entry.when;
            if (pPosition)
                *pPosition = ifsHandle->virtLoc;
            break;
        }
    }

    g_static_mutex_unlock(&(ifsHandle->mutex));
    return ifsReturnCode;
}

IfsReturnCode IfsSeekToTime(IfsHandle ifsHandle, // Input
        IfsDirect ifsDirect, // Input  either IfsDirectBegin,
        //        IfsDirectEnd or IfsDirectEither
        IfsClock * pIfsClock, // Input  requested/Output actual, in nanoseconds
        NumPackets * pPosition // Output packet position, optional, can be NULL
)
{
    if (ifsHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ifsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    if ((ifsDirect != IfsDirectBegin) && (ifsDirect != IfsDirectEnd)
            && (ifsDirect != IfsDirectEither))
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ifsDirect is not valid in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    if (pIfsClock == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: pIfsClock == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    g_static_mutex_lock(&(ifsHandle->mutex));

    if ((*pIfsClock < ifsHandle->begClock)
            || (*pIfsClock > ifsHandle->endClock))
    {
        IfsReturnCode ifsReturnCode = GetCurrentFileParameters(ifsHandle);
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
        {
            g_static_mutex_unlock(&(ifsHandle->mutex));
            return ifsReturnCode;
        }
    }

    g_static_mutex_unlock(&(ifsHandle->mutex));
    return IfsSeekToTimeImpl(ifsHandle, ifsDirect, pIfsClock, pPosition);
}

IfsReturnCode IfsSeekToPacketImpl // Must call GetCurrentFileSizeAndCount() before calling this function
(IfsHandle ifsHandle, // Input
        NumPackets virtPos, // Input desired (virtual) packet position
        IfsClock * pIfsClock // Output clock value, optional, can be NULL
)
{
    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;
    NumPackets begPosition = 0;
    NumPackets endPosition = 0;
    NumPackets realPos; // desired (real) packet position
    g_static_mutex_lock(&(ifsHandle->mutex));

    for (ifsHandle->curFileNumber = ifsHandle->begFileNumber; ifsHandle->curFileNumber
            <= ifsHandle->endFileNumber; ifsHandle->curFileNumber++)
    {
        struct stat statBuffer;

        g_static_mutex_unlock(&(ifsHandle->mutex));
        ifsReturnCode = IfsOpenActualFiles(ifsHandle, ifsHandle->curFileNumber,
                "rb+");
        g_static_mutex_lock(&(ifsHandle->mutex));

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        if (stat(ifsHandle->mpeg, &statBuffer))
        {
            RILOG_ERROR(
                    "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->mpeg, errno, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeStatError;
            break;
        }

        ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, 0);
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        begPosition = ifsHandle->entry.virtWhere - ifsHandle->entry.realWhere;
        endPosition = begPosition + (size_t) statBuffer.st_size
                / sizeof(IfsPacket);

#ifdef DEBUG_SEEKS
        RILOG_INFO("%ld <= %ld < %ld ?\n", begPosition, virtPos, endPosition);
#endif

        if ((begPosition <= virtPos) && (virtPos < endPosition))
        {
            // Found the file!
            //
            // Estimate the starting point for the seek into the ndex file by
            // calculating the percentage into the mpeg file:
            //
            // percentage = (virtPos-begPosition) / (endPosition-begPosition)
            //

#ifdef DEBUG_SEEKS
            char temp[32]; // IfsToSecs only
            long realDiff;
#endif
            long virtDiff;
            NumEntries entry;
            if (stat(ifsHandle->ndex, &statBuffer))
            {
                RILOG_ERROR(
                        "IfsReturnCodeStatError: stat(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeStatError;
                break;
            }

            realPos = virtPos - begPosition;

            ifsHandle->maxEntry = (size_t) statBuffer.st_size
                    / sizeof(IfsIndexEntry) - 1;

            entry = ifsHandle->maxEntry * realPos / (endPosition - begPosition);

            ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, entry);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;

            virtDiff = virtPos - ifsHandle->entry.virtWhere;
#ifdef DEBUG_SEEKS
            realDiff = realPos - ifsHandle->entry.realWhere;

            RILOG_INFO("\nSeek to file number %3ld about entry %3ld want %ld/%ld got %ld/%ld diff %ld/%ld\n",
                    ifsHandle->curFileNumber,
                    entry,
                    realPos,
                    virtPos,
                    ifsHandle->entry.realWhere,
                    ifsHandle->entry.virtWhere,
                    realDiff,
                    virtDiff);
#endif

            if (virtDiff > 0) // scan forward for best seek
            {
                while (entry < ifsHandle->maxEntry)
                {
#ifdef DEBUG_SEEKS
                    long realTemp;
#endif
                    long virtTemp;

                    ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, ++entry);
                    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                        break;

#ifdef DEBUG_SEEKS
                    realTemp = realPos - ifsHandle->entry.realWhere;
#endif
                    virtTemp = virtPos - ifsHandle->entry.virtWhere;

#ifdef DEBUG_SEEKS
                    RILOG_INFO("                          Try entry %3ld want %ld/%ld got %ld/%ld diff %ld/%ld\n",
                            entry,
                            realPos,
                            virtPos,
                            ifsHandle->entry.realWhere,
                            ifsHandle->entry.virtWhere,
                            realTemp,
                            virtTemp);
#endif

                    if (virtTemp >= 0)
                    {
                        // Must be better (or the same), keep trying (unless done)
#ifdef DEBUG_SEEKS
                        realDiff = realTemp;
#endif
                        virtDiff = virtTemp;
                    }
                    else
                    {
                        // We are done here but which one is better?
                        if (-virtTemp < virtDiff)
                        {
                            // The negative one is better
#ifdef DEBUG_SEEKS
                            realDiff = realTemp;
                            virtDiff = virtTemp;
#endif
                        }
                        else // The old positive one was better
                        {
                            entry--;
                        }
                        break;
                    }
                }
            }
            else if (virtDiff < 0)
            {
                while (entry)
                {
#ifdef DEBUG_SEEKS
                    long realTemp;
#endif
                    long virtTemp;

                    ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, --entry);
                    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                        break;

                    virtTemp = virtPos - ifsHandle->entry.virtWhere;
#ifdef DEBUG_SEEKS
                    realTemp = realPos - ifsHandle->entry.realWhere;

                    RILOG_INFO("                          Try entry %3ld want %ld/%ld got %ld/%ld diff %ld/%ld\n",
                            entry,
                            realPos,
                            virtPos,
                            ifsHandle->entry.realWhere,
                            ifsHandle->entry.virtWhere,
                            realTemp,
                            virtTemp);
#endif

                    if (virtTemp <= 0)
                    {
                        // Must be better (or the same), keep trying (unless done)
#ifdef DEBUG_SEEKS
                        realDiff = realTemp;
#endif
                        virtDiff = virtTemp;
                    }
                    else
                    {
                        // We are done here but which one is better?
                        if (virtTemp < -virtDiff)
                        {
                            // The positive one is better
#ifdef DEBUG_SEEKS
                            realDiff = realTemp;
                            virtDiff = virtTemp;
#endif
                        }
                        else // The old negative one was better
                        {
                            entry++;
                        }
                        break;
                    }
                }
            }

            ifsReturnCode = IfsReadNdexEntryAt(ifsHandle, entry);
            if (ifsReturnCode != IfsReturnCodeNoErrorReported)
                break;
            if (fseek(ifsHandle->pNdex, entry * sizeof(IfsIndexEntry), SEEK_SET))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) failed (%d) in line %d of %s\n",
                        ifsHandle->ndex, entry * sizeof(IfsIndexEntry), errno,
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileSeekingError;
                break;
            }

#ifdef DEBUG_SEEKS
            RILOG_INFO("                      Best is entry %3ld want %ld/%ld got %ld/%ld diff %ld/%ld\n",
                    entry,
                    realPos,
                    virtPos,
                    ifsHandle->entry.realWhere,
                    ifsHandle->entry.virtWhere,
                    realDiff,
                    virtDiff);
            RILOG_INFO("                                        when %s where %ld/%ld\n",
                    IfsToSecs(ifsHandle->entry.when, temp),
                    ifsHandle->entry.realWhere,
                    ifsHandle->entry.virtWhere);
#endif

            if (fseek(ifsHandle->pMpeg, realPos * sizeof(IfsPacket), SEEK_SET))
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileSeekingError: fseek(%s, %ld, SEEK_SET) failed (%d) in line %d of %s\n",
                        ifsHandle->mpeg, realPos * sizeof(IfsPacket), errno,
                        __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileSeekingError;
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

            ifsHandle->maxPacket = (size_t) statBuffer.st_size
                    / sizeof(IfsPacket) - 1;
            ifsHandle->realLoc = realPos;
            ifsHandle->virtLoc = virtPos;
            ifsHandle->entryNum = entry;

            if (pIfsClock)
                *pIfsClock = ifsHandle->entry.when;
            break;
        }
    }

    // All errors come here
    g_static_mutex_unlock(&(ifsHandle->mutex));

    return ifsReturnCode;
}

IfsReturnCode IfsSeekToPacket(IfsHandle ifsHandle, // Input
        NumPackets virtPos, // Input desired (virtual) packet position
        IfsClock * pIfsClock // Output clock value, optional, can be NULL
)
{
    IfsReturnCode ifsReturnCode;

    if (ifsHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ifsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    g_static_mutex_lock(&(ifsHandle->mutex));
    ifsReturnCode = GetCurrentFileSizeAndCount(ifsHandle);
    g_static_mutex_unlock(&(ifsHandle->mutex));

    if (ifsReturnCode != IfsReturnCodeNoErrorReported)
        return ifsReturnCode;

    return IfsSeekToPacketImpl(ifsHandle, virtPos, pIfsClock);
}

