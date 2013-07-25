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

#define _IFS_IMPL_C "$Rev: 141 $"

#define FLUSH_ALL_WRITES
//#define DEBUG_SEEKS
//#define DEBUG_CONVERT_APPEND

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
#include "ifs_operations.h"
#include "ifs_parse.h"
#include "ifs_utils.h"

log4c_category_t * ifs_RILogCategory = NULL;
#define RILOG_CATEGORY ifs_RILogCategory

#define DEFAULT_IFS_CHUNK_SIZE "600"

IfsIndexDumpMode indexDumpMode = IfsIndexDumpModeOff;

IfsIndex whatAll = 0;
unsigned indexCase = 0;
unsigned ifsFileChunkSize;

// Maintain a list of active writers
//
#define MAX_WRITER_LIST_CNT 128
static IfsHandle writerListHandles[MAX_WRITER_LIST_CNT];
static unsigned writerListCnt = 0;
static GStaticMutex writerListMutex = G_STATIC_MUTEX_INIT;

static void writerListAdd(IfsHandle pIfsHandle);
static void writerListRemove(IfsHandle pIfsHandle);


void IfsInit(void)
{
#ifdef STAND_ALONE
    ifs_RILogCategory = log4c_category_get("RI.IFS");
    ifsFileChunkSize = atoi(DEFAULT_IFS_CHUNK_SIZE);
#else
    const char * chunk;

    ifs_RILogCategory = log4c_category_get("RI.IFS");

    if ((chunk = ricfg_getValue("RIPlatform", "RI.Platform.dvr.IfsChunkSize"))
            == NULL)
    {
        RILOG_WARN(
                "RI.Platform.dvr.IfsChunkSize not specified in the platform config file!\n");
        chunk = DEFAULT_IFS_CHUNK_SIZE;
    }
    ifsFileChunkSize = atoi(chunk);

    int i = 0;
    for (i = 0; i < MAX_WRITER_LIST_CNT; i++)
    {
        writerListHandles[i] = NULL;
    }
#endif
}

static IfsReturnCode IfsOpenImpl(IfsBoolean isReading, // Input  (if true then path+name must exist and maxSize is ignored)
        const char * path, // Input
        const char * name, // Input  (if writing and NULL the name is generated)
        IfsTime maxSize, // Input  (in seconds, 0 = no max, ignored if reading)
        IfsHandle * pIfsHandle // Output (use IfsClose() to g_free)
)
{
    // The various open cases for a writer are:
    //
    //  case  char*name  maxSize  Found   Description
    //  ----  ---------  -------  ------  ------------
    //
    //     1       NULL        0      NA  Generate directory and create the single 0000000000 file
    //
    //     2       NULL    not 0      NA  Generate directory and create the 0000000001 circular file
    //
    //     3   not NULL        0      No  Create the specified directory and single 0000000000 file
    //
    //     4   not NULL    not 0      No  Create the specified directory and the 0000000001 circular file
    //
    //     5   not NULL        0     Yes  If 0000000000 file found open it otherwise report an error
    //
    //     6   not NULL    not 0     Yes  If 0000000000 file found error, otherwise open lowest file name
    //
    // The various open cases for a reader are:
    //
    //  case  char*name  Found   Description
    //  ----  ---------  ------  ------------
    //
    //     7       NULL      NA  Report IfsReturnCodeBadInputParameter
    //
    //     8   not NULL      No  Report IfsReturnCodeFileWasNotFound
    //
    //     9   not NULL     Yes  Open the file for reading

    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported; // used to report errors
    IfsHandle ifsHandle; // IfsHandle being created/opened
    char temp[256]; // filename only  // temporary string storage

    // Check the input parameters, initialize the output parameters and report any errors

    if (pIfsHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: pIfsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    else
        *pIfsHandle = NULL;

    if (path == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: path == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    // check for NULL or empty string name
    if ((name == NULL) || ('\0' == name[0]))
    {
        if (isReading) // Case 7 - report error
        {
            RILOG_ERROR(
                    "IfsReturnCodeBadInputParameter: Reader with name == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            return IfsReturnCodeBadInputParameter;
        }
        else // Cases 1 and 2 - generate a directory name
        {
            // Generated a direcory name of the form XXXXXXXX_xxxx
            // where XXXXXXXX is the hex representation of the epoch seconds
            // and xxxx is a unique number
            static unsigned short uniqueNum = 0u;
            static GStaticMutex uniqueLock = G_STATIC_MUTEX_INIT;

            unsigned short localUniqueNum = 0u;
            g_static_mutex_lock(&uniqueLock);
            localUniqueNum = uniqueNum++;
            g_static_mutex_unlock(&uniqueLock);

            if (sprintf(temp, "%08lX_%04X", time(NULL), localUniqueNum)
                    != 13)
            {
                RILOG_ERROR(
                        "IfsReturnCodeSprintfError: sprintf() != 13 in line %d of %s\n",
                        __LINE__, __FILE__);
                return IfsReturnCodeSprintfError;
            }

            name = temp;
        }
    }

    // Start the process by allocating memory for the IfsHandle

    ifsHandle = g_try_malloc0(sizeof(IfsHandleImpl));
    if (ifsHandle == NULL)
    {
        RILOG_CRIT(
                "IfsReturnCodeMemAllocationError: ifsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeMemAllocationError;
    }

    // Initialize all the pointers in the IfsHandle
    g_static_mutex_init(&(ifsHandle->mutex));

    // (done by g_try_malloc0) ifsHandle->path  = NULL; // current path
    // (done by g_try_malloc0) ifsHandle->name  = NULL; // current name
    // (done by g_try_malloc0) ifsHandle->both  = NULL; // current path + name
    // (done by g_try_malloc0) ifsHandle->mpeg  = NULL; // current path + name + filename.mpg
    // (done by g_try_malloc0) ifsHandle->ndex  = NULL; // current path + name + filename.ndx
    // (done by g_try_malloc0) ifsHandle->pMpeg = NULL; // current MPEG file
    // (done by g_try_malloc0) ifsHandle->pNdex = NULL; // current NDEX file

    // Now fill in all the values of the IfsHandle

    do
    {
        struct stat statBuffer;

        // Process the input parameters

        const size_t pathSize = strlen(path) + 1; // path + \000
        const size_t nameSize = strlen(name) + 1; // name + \000
        const size_t bothSize = pathSize + nameSize; // path + / + name + \000

        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        ifsHandle->numEmptyFreads = 0;
        ifsHandle->isReading = isReading;
        ifsHandle->maxSize = maxSize; // in seconds, 0 = value not used

        ifsHandle->path = g_try_malloc(pathSize);
        ifsHandle->name = g_try_malloc(nameSize);
        ifsHandle->both = g_try_malloc(bothSize);

        if (ifsHandle->path == NULL)
        {
            RILOG_CRIT(
                    "IfsReturnCodeMemAllocationError: ifsHandle->path == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeMemAllocationError;
        }
        if (ifsHandle->name == NULL)
        {
            RILOG_CRIT(
                    "IfsReturnCodeMemAllocationError: ifsHandle->name == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeMemAllocationError;
        }
        if (ifsHandle->both == NULL)
        {
            RILOG_CRIT(
                    "IfsReturnCodeMemAllocationError: ifsHandle->both == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeMemAllocationError;
        }
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            break;

        memcpy(ifsHandle->path, path, pathSize);
        memcpy(ifsHandle->name, name, nameSize);
        memcpy(ifsHandle->both, path, pathSize);
        strcat(ifsHandle->both, "/");
        strcat(ifsHandle->both, name);

        if (stat(ifsHandle->both, &statBuffer)) // The directory was NOT found
        {
            if (isReading) // Case 8 - report error
            {
                RILOG_ERROR(
                        "IfsReturnCodeFileWasNotFound: stat(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->both, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeFileWasNotFound;
                break;
            }

            // Cases 1 through 4 - make the specified (or generated) directory

            if (makedir(ifsHandle->both))
            {
                RILOG_ERROR(
                        "IfsReturnCodeMakeDirectoryError: makedir(%s) failed (%d) in line %d of %s\n",
                        ifsHandle->both, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeMakeDirectoryError;
                break;
            }

            // (done by g_try_malloc0) ifsHandle->mpegSize = 0;
            // (done by g_try_malloc0) ifsHandle->ndexSize = 0;

            ifsHandle->begFileNumber = ifsHandle->endFileNumber = maxSize ? 1
                    : 0;

            // (done by g_try_malloc0) ifsHandle->begClock =
            // (done by g_try_malloc0) ifsHandle->endClock =
            // (done by g_try_malloc0) ifsHandle->nxtClock = 0; // nanoseconds

            ifsReturnCode = IfsOpenActualFiles(ifsHandle,
                    ifsHandle->begFileNumber, "wb+");
        }
        else // The directory was found
        {
            // Cases 5, 6 and 9 - open the existing IFS file

            ifsReturnCode = GetCurrentFileParameters(ifsHandle);
        }

        // (done by g_try_malloc0) ifsHandle->realLoc = 0; // offset in packets
        // (done by g_try_malloc0) ifsHandle->virtLoc = 0; // offset in packets

        ifsHandle->videoPid = ifsHandle->audioPid = IFS_UNDEFINED_PID;

        ifsHandle->ifsState = IfsStateInitial;

        ifsHandle->oldEsp = ifsHandle->oldSc = ifsHandle->oldTp
                = ifsHandle->oldCc = IFS_UNDEFINED_BYTE;

    } while (0);

    if (ifsReturnCode == IfsReturnCodeNoErrorReported)
    {
        *pIfsHandle = ifsHandle;

        if ((indexDumpMode == IfsIndexDumpModeAll) && whatAll)
        {
#ifdef DEBUG_ALL_PES_CODES
            RILOG_INFO("----  ---------------- = -------- --------------- ------------\n");
            RILOG_INFO("      %08lX%08lX\n", (unsigned long)(whatAll>>32), (unsigned long)whatAll);
#else
            RILOG_INFO(
                    "----  -------- = -------- --------------- ------------\n");
            RILOG_INFO("      %08X\n", whatAll);
#endif
        }
    }
    else
    {
        (void) IfsClose(ifsHandle); // Ignore any errors, we already have one...
    }

    return ifsReturnCode;
}

IfsReturnCode IfsOpenWriter(const char * path, // Input
        const char * name, // Input  (if NULL the name is generated)
        IfsTime maxSize, // Input  (in seconds, 0 = no max)
        IfsHandle * pIfsHandle // Output (use IfsClose() to g_free)
)
{
    IfsReturnCode ifsReturnCode =
        IfsOpenImpl(IfsFalse, // Input  (if true then path+name must exist and maxSize is ignored)
            path, // Input
            name, // Input  (if writing and NULL the name is generated)
            maxSize, // Input  (in seconds, 0 = no max, ignored if reading)
            pIfsHandle); // Output (use IfsClose() to g_free)

    // If writer was successfully opened, add to writer list
    if (ifsReturnCode == IfsReturnCodeNoErrorReported)
    {
        writerListAdd(*pIfsHandle);
    }

    return ifsReturnCode;
}

IfsReturnCode IfsOpenReader(const char * path, // Input
        const char * name, // Input
        IfsHandle * pIfsHandle // Output (use IfsClose() to g_free)
)
{
    return IfsOpenImpl(IfsTrue, // Input  (if true then path+name must exist and maxSize is ignored)
            path, // Input
            name, // Input  (if writing and NULL the name is generated)
            0, // Input  (in seconds, 0 = no max, ignored if reading)
            pIfsHandle); // Output (use IfsClose() to g_free)
}

IfsReturnCode IfsDelete(const char * path, // Input
        const char * name // Input
)
{
    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

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

    {
        const size_t pathSize = strlen(path) + 1; // path + \000
        const size_t nameSize = strlen(name) + 1; // name + \000
        const size_t bothSize = pathSize + nameSize; // path + / + name + \000

        char * both = g_try_malloc(bothSize); // g_free here
        if (both == NULL)
        {
            RILOG_CRIT(
                    "IfsReturnCodeMemAllocationError: both == NULL in line %d of %s\n",
                    __LINE__, __FILE__);
            return IfsReturnCodeMemAllocationError;
        }

        memcpy(both, path, pathSize);
        strcat(both, "/");
        strcat(both, name);

        do
        {
            struct dirent * pDirent;
            DIR * pDir = opendir(both);
            if (pDir == NULL)
            {
                RILOG_ERROR(
                        "IfsReturnCodeOpenDirectoryError: opendir(%s) failed (%d) in line %d of %s\n",
                        both, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeOpenDirectoryError;
                break;
            }

            //          RILOG_INFO("\nDeleting files found in %s:\n", both);

            while ((pDirent = readdir(pDir)) != NULL)
            {
                FileNumber newFileNumber;
                char temp[1024]; // path and filename

                if (sscanf(pDirent->d_name, "%010lu.", &newFileNumber))
                {
                    strcpy(temp, both);
                    strcat(temp, "/");
                    strcat(temp, pDirent->d_name);

                    if (remove(temp))
                    {
                        RILOG_ERROR(
                                "IfsReturnCodeFileDeletingError: remove(%s) failed (%d) in line %d of %s\n",
                                temp, errno, __LINE__, __FILE__);
                        ifsReturnCode = IfsReturnCodeFileDeletingError;
                    }

                    //                  RILOG_INFO("   %s\n", pDirent->d_name);
                }
            }

            if (closedir(pDir))
            {
                RILOG_ERROR(
                        "IfsReturnCodeCloseDirectoryError: closedir(%s) failed (%d) in line %d of %s\n",
                        both, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeCloseDirectoryError;
            }

            if (rmdir(both))
            {
                RILOG_ERROR(
                        "IfsReturnCodeDeleteDirectoryError: rmdir(%s) failed (%d) in line %d of %s\n",
                        both, errno, __LINE__, __FILE__);
                ifsReturnCode = IfsReturnCodeDeleteDirectoryError;
            }

        } while (0);

        g_free(both);
    }

    return ifsReturnCode;
}

// IfsHandle operations:

IfsReturnCode IfsStart(IfsHandle ifsHandle, // Input (must be a writer)
        IfsPid videoPid, // Input
        IfsPid audioPid // Input
)
{
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

    ifsHandle->videoPid = videoPid;
    ifsHandle->audioPid = audioPid;

    if (indexDumpMode == IfsIndexDumpModeAll)
    {
        RILOG_INFO("\n");

#ifdef DEBUG_ALL_PES_CODES
        RILOG_INFO("Case  What             = Header   Start/Adapt     Extension   \n");
        RILOG_INFO("----  ---------------- = -------- --------------- ------------\n");
#else
        RILOG_INFO("Case  What     = Header   Start/Adapt     Extension   \n");
        RILOG_INFO("----  -------- = -------- --------------- ------------\n");
#endif
    }

    g_static_mutex_unlock(&(ifsHandle->mutex));
    return IfsReturnCodeNoErrorReported;
}

IfsReturnCode IfsSetMaxSize(IfsHandle ifsHandle, // Input (must be a writer)
        IfsTime maxSize // Input (in seconds, 0 is illegal)
)
{
    if (ifsHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ifsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }
    if (maxSize == 0)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: maxSize == 0 in line %d of %s\n",
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
    if (ifsHandle->maxSize == 0)
    {
        RILOG_ERROR(
                "IfsReturnCodeIllegalOperation: ifsHandle->maxSize == 0 in line %d of %s\n",
                __LINE__, __FILE__);
        g_static_mutex_unlock(&(ifsHandle->mutex));
        return IfsReturnCodeIllegalOperation;
    }

    ifsHandle->maxSize = maxSize; // in seconds, 0 is illegal

    g_static_mutex_unlock(&(ifsHandle->mutex));
    return IfsReturnCodeNoErrorReported;
}

IfsReturnCode IfsStop(IfsHandle ifsHandle // Input (must be a writer)
)
{
    IfsReturnCode ifsReturnCode = IfsReturnCodeNoErrorReported;

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

    ifsHandle->ifsState = IfsStateInitial; // Reset the PES state machine

    (void) CloseActualFiles(ifsHandle); // Don't care about errors here
    g_static_mutex_unlock(&(ifsHandle->mutex));

    do
    {
        struct dirent * pDirent;
        DIR * pDir = opendir(ifsHandle->both);
        if (pDir == NULL)
        {
            RILOG_ERROR(
                    "IfsReturnCodeOpenDirectoryError: opendir(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->both, errno, __LINE__, __FILE__);
            return IfsReturnCodeOpenDirectoryError;
        }

        //      RILOG_INFO("\nDeleting files found in %s:\n", ifsHandle->both);

        while ((pDirent = readdir(pDir)) != NULL)
        {
            FileNumber newFileNumber;
            char temp[1024]; // path and filename

            if (sscanf(pDirent->d_name, "%010lu.", &newFileNumber))
            {
                strcpy(temp, ifsHandle->both);
                strcat(temp, "/");
                strcat(temp, pDirent->d_name);

                if (remove(temp))
                {
                    RILOG_ERROR(
                            "IfsReturnCodeFileDeletingError: remove(%s) failed (%d) in line %d of %s\n",
                            temp, errno, __LINE__, __FILE__);
                    ifsReturnCode = IfsReturnCodeFileDeletingError;
                }

                //              RILOG_INFO("   %s\n", pDirent->d_name);
            }
        }

        if (closedir(pDir))
        {
            RILOG_ERROR(
                    "IfsReturnCodeCloseDirectoryError: closedir(%s) failed (%d) in line %d of %s\n",
                    ifsHandle->both, errno, __LINE__, __FILE__);
            ifsReturnCode = IfsReturnCodeCloseDirectoryError;
        }
        if (ifsReturnCode != IfsReturnCodeNoErrorReported)
            return ifsReturnCode;

        g_static_mutex_lock(&(ifsHandle->mutex));
        ifsHandle->numEmptyFreads = 0;
        ifsHandle->mpegSize = 0;
        ifsHandle->ndexSize = 0;

        ifsHandle->begClock = ifsHandle->endClock = ifsHandle->nxtClock = 0;

        ifsHandle->begFileNumber = ifsHandle->endFileNumber
                = ifsHandle->maxSize ? 1 : 0;

        ifsReturnCode = IfsOpenActualFiles(ifsHandle, ifsHandle->begFileNumber,
                "wb+");

        ifsHandle->realLoc = 0; // offset in packets
        ifsHandle->virtLoc = 0; // offset in packets

        ifsHandle->videoPid = ifsHandle->audioPid = IFS_UNDEFINED_PID;

        ifsHandle->ifsState = IfsStateInitial;

        ifsHandle->oldEsp = ifsHandle->oldSc = ifsHandle->oldTp
                = ifsHandle->oldCc = IFS_UNDEFINED_BYTE;
        g_static_mutex_unlock(&(ifsHandle->mutex));

    } while (0);

    return ifsReturnCode;
}

IfsReturnCode IfsClose(IfsHandle ifsHandle // Input
)
{
    if (ifsHandle == NULL)
    {
        RILOG_ERROR(
                "IfsReturnCodeBadInputParameter: ifsHandle == NULL in line %d of %s\n",
                __LINE__, __FILE__);
        return IfsReturnCodeBadInputParameter;
    }

    g_static_mutex_lock(&(ifsHandle->mutex));

    // If this is a writer, remove it from list
    if (!ifsHandle->isReading)
    {
        // Remove this writer from the list
        writerListRemove(ifsHandle);
    }

    if (ifsHandle->path)
        g_free(ifsHandle->path); // g_try_malloc in IfsOpenImpl()
    if (ifsHandle->name)
        g_free(ifsHandle->name); // g_try_malloc in IfsOpenImpl()
    if (ifsHandle->both)
        g_free(ifsHandle->both); // g_try_malloc in IfsOpenImpl()
    if (ifsHandle->mpeg)
        g_free(ifsHandle->mpeg); // g_try_malloc in GenerateFileNames()
    if (ifsHandle->ndex)
        g_free(ifsHandle->ndex); // g_try_malloc in GenerateFileNames()
    if (ifsHandle->pMpeg)
        fclose(ifsHandle->pMpeg); // open in IfsOpenActualFiles()
    if (ifsHandle->pNdex)
        fclose(ifsHandle->pNdex); // open in IfsOpenActualFiles()

    g_static_mutex_unlock(&(ifsHandle->mutex));
    g_free(ifsHandle); // g_try_malloc0 in IfsOpenImpl()

    return IfsReturnCodeNoErrorReported;
}

IfsBoolean IfsHasWriter(IfsHandle ifsHandle)
{
    if ((ifsHandle->path == NULL) || (ifsHandle->name == NULL))
    {
        RILOG_WARN("supplied handle had null name and or path, unable to check for writer\n");
        return IfsFalse;
    }

    g_static_mutex_lock(&writerListMutex);

    IfsHandle curHandle = NULL;
    int i = 0;
    for (i = 0; i < MAX_WRITER_LIST_CNT; i++)
    {
        if (writerListHandles[i] != NULL)
        {
            curHandle = writerListHandles[i];
            if ((curHandle->path != NULL) && (curHandle->name != NULL))
            {
                if (strcmp(curHandle->path, ifsHandle->path) == 0)
                {
                    if (strcmp(curHandle->name, ifsHandle->name) == 0)
                    {
                        // Found writer in list, return true
                        //RILOG_INFO("found active writer with path: %s, name: %s\n",
                        //        curHandle->path, curHandle->name);
                        g_static_mutex_unlock(&writerListMutex);
                        return IfsTrue;
                    }
                }
            }
        }
    }
    g_static_mutex_unlock(&writerListMutex);

    return IfsFalse;
}

static void writerListAdd(IfsHandle ifsHandle)
{
    g_static_mutex_lock(&writerListMutex);

    // Find the next available slot to stick this writer
    int idx = -1;
    int i = 0;
    for (i = 0; i < MAX_WRITER_LIST_CNT; i++)
    {
        if (writerListHandles[i] == NULL)
        {
            writerListHandles[i] = ifsHandle;
            idx = i;
            break;
        }
    }
    if (idx == -1)
    {
        RILOG_WARN("No more slots available, current cnt: %d\n", writerListCnt);
    }
    else
    {
        writerListCnt++;
    }
    g_static_mutex_unlock(&writerListMutex);
}

static void writerListRemove(IfsHandle ifsHandle)
{
    g_static_mutex_lock(&writerListMutex);

    // Find this writer in the list
    int idx = -1;
    int i = 0;
    for (i = 0; i < MAX_WRITER_LIST_CNT; i++)
    {
        if (writerListHandles[i] == ifsHandle)
        {
            writerListHandles[i] = NULL;
            idx = i;
            break;
        }
    }
    if (idx == -1)
    {
        RILOG_WARN("Unable to find writer, current cnt: %d\n", writerListCnt);
    }
    else
    {
        writerListCnt--;
    }
    g_static_mutex_unlock(&writerListMutex);
}

