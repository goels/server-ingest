// COPYRIGHT_BEGIN
//  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
//  
//  Copyright (C) 2013, Cable Television Laboratories, Inc. 
//  
//  This software is available under multiple licenses: 
//  
//  (1) BSD 2-clause 
//   Redistribution and use in source and binary forms, with or without modification, are
//   permitted provided that the following conditions are met:
//        �Redistributions of source code must retain the above copyright notice, this list 
//             of conditions and the following disclaimer.
//        �Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include "ifs_utils.h"
#include "ifs_GenTrickPlay.h"


#define IFS_TEST_FLAG_SPS	0x01
#define IFS_TEST_FLAG_PICI	0x04
#define IFS_TEST_FLAG_PUSI	0x08


//
//

static unsigned IfsCheck_SEQ_PIC(IfsCodecType codecType, ullong ifsIndex);
static IfsBoolean handle_ForwardTrickFileIndexing(trickInfo *tinfo);
static IfsBoolean handle_ReverseTrickFileIndexing(trickInfo *tinfo);
static IfsBoolean IfsCopyPackets(FILE *infile_ts, FILE *outfile_tm, unsigned long pktCount, unsigned pktSize);
static IfsBoolean IfsCopyFrameData(trickInfo *tinfo);
static IfsBoolean IfsCopyEntrySetData(trickInfo *tinfo);
static IfsBoolean IfsCopyIndexData(trickInfo *tinfo);
static IfsBoolean IfsCopyVobuIndexData(trickInfo *tinfo);
static IfsBoolean getPatPmtPackets(streamInfo *strmInfo, trickInfo *tinfo);
static void IfsSaveReferenceFrameData(trickInfo *tinfo);
static IfsBoolean generate_normalPlayIndexFile(trickInfo *tinfo);
static void write_indexEntries(FILE *index_file, char *frame_type, IfsClock time, int64_t byte_offset, int64_t frame_size);
static void get_frameTypeH262(unsigned long long index, char* frame_type);
static void get_frameTypeH264(unsigned long long index, char* frame_type);
//-------------------------------------------------------


typedef struct stat Stat;


static int do_mkdir(const char *path, mode_t mode)
{
    Stat            st;
    int             status = 0;

    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }
    return(status);
}

/**
** mkpath - ensure all directories in path exist
** Algorithm takes the pessimistic view and works top-down to ensure
** each directory in path exists, rather than optimistically creating
** the last element and working backwards.
*/
static int mkpath(const char *path, mode_t mode)
{
    char           *pp;
    char           *sp;
    int             status;
    char           copypath[MAX_PATH];


    strcpy(copypath, path);
    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(path, mode);
     return (status);
}


// -----------------------------------------------------
// trick mode file generation stuff
// -----------------------------------------------------

IfsBoolean generate_trickfile(char *indexfilename, streamInfo *strmInfo, int trick_speed, char *desdir)
{
	static char tmpFileName[MAX_PATH];
	static char trick_filename[MAX_PATH];

	entrySet   trick_entryset;
    refIframeEntry refIframe;
    refVobuEntry refVobu;
    refVobuEntry currentVobu;
	trickInfo tinfo = {NULL, IfsFalse, IfsFalse, IfsFalse,
						0, 2, 1, 2, 2, 0, 0, "", "",
						NULL, NULL, NULL, NULL, NULL,
						&trick_entryset, &refIframe, &refVobu, &currentVobu,
						0, 0, NULL, NULL, 0 };


    printf("Info: Generating trick play file...\n");

    if( indexfilename == NULL || strmInfo == NULL)
    {
    	printf("Error: input file name pointers are NULL\n");
    	return IfsFalse;
    }
    if(strmInfo->stream_filename == NULL)
    {
    	printf("Error: transport stream file name pointer is NULL\n");
    	return IfsFalse;
    }

    tinfo.ifsHandle = g_try_malloc0(sizeof(IfsHandleImpl));
    if (tinfo.ifsHandle == NULL)
    {
        printf("Error: Could not allocate memory for handle\n");
        return IfsFalse;
    }
    if(strmInfo->tsPktSize)
    {
    	tinfo.ifsHandle->pktSize = strmInfo->tsPktSize;
    }
    else
    {
    	printf("Error: stream packet size is zero, using default of 188 bytes\n");
    	tinfo.ifsHandle->pktSize = IFS_TRANSPORT_PACKET_SIZE;

    }

    if(trick_speed == -1 || trick_speed == 0)
    {
        printf("Error: Invalid trick speed, Trick Speeds -1 and 0 not allowed.\n");
        return IfsFalse;
    }
    if(trick_speed < 0 && trick_speed >= -200)
    {
    	tinfo.trick_direction = -1;
    	tinfo.trick_speed =  tinfo.trick_direction * trick_speed;
    }
    else
	if(trick_speed > 0 && trick_speed <= 200)
	{
		tinfo.trick_direction = 1;
		tinfo.trick_speed = trick_speed;
	}
 	else
 	{
 		printf("Error: Trick Speed argument is expected to be between -200 to +200.\n");
 		return IfsFalse;
 	}

	tinfo.codecType = strmInfo->codecType;
	tinfo.containerType = strmInfo->containerType;
	tinfo.trick_entryset->codecType = strmInfo->codecType;
	tinfo.trick_entryset->containerType = strmInfo->containerType;

    switch(strmInfo->codecType)
    {
    	case IfsCodecTypeH264:	// H.264
			IfsSetCodec(tinfo.ifsHandle, IfsCodecTypeH264);
			tinfo.parseWhat = tinfo.ifsHandle->codec->h264->ParseWhat;
			break;
    	case IfsCodecTypeH262:
    	default:
    		IfsSetCodec(tinfo.ifsHandle, IfsCodecTypeH262);
			tinfo.parseWhat = tinfo.ifsHandle->codec->h262->ParseWhat;
   		break;
    }

    if(strstr(indexfilename, ".ndx"))
	{
    	char extn[6];

    	if (strstr(strmInfo->stream_filename, ".ts4") ||
        	strstr(strmInfo->stream_filename, ".ts2") ||
        	strstr(strmInfo->stream_filename, ".mpg") ||
        	strstr(strmInfo->stream_filename, ".vob") ||
        	strstr(strmInfo->stream_filename, ".ts"))
    	    // (.vob is for dvd derived program streams)
        {
    		printf("stream file name: %s\n", strmInfo->stream_filename);
           	strcpy(tinfo.ts_filename, strmInfo->stream_filename);
            strcpy(tinfo.trick_filename,  strmInfo->stream_filename);
            strcpy(extn, "ts"); // default extension
            char * ext = strrchr(tinfo.trick_filename, '.');
            if (ext)
            {
             	strncpy(extn, ext, 5);
             	extn[5] = '\0';
            	*ext = '\0';
            	tinfo.newTrickFile = IfsTrue;
            }
            printf("Info: trick mode filename: %s, trick_speed: %d\n", tinfo.trick_filename, tinfo.trick_speed);
        }
        else
        {
        	printf("Error: Invalid Argument: expected a valid stream file name, .ts4 or .ts2\n");
        	g_free(tinfo.ifsHandle);
        	tinfo.ifsHandle = NULL;
        	return IfsFalse;
        }

	    printf("Info: Opening index file: %s\n", indexfilename);
		tinfo.ifsHandle->pNdex = fopen(indexfilename, "rb");
		if (tinfo.ifsHandle->pNdex)
		{
			if (!fread(&tinfo.ifsHandle->entry, 1, sizeof(IfsIndexEntry),
					tinfo.ifsHandle->pNdex) == sizeof(IfsIndexEntry))
			{
				printf("Error: Error opening/reading the index file: %s\n", indexfilename);
				if(tinfo.ifsHandle)
					g_free(tinfo.ifsHandle);
				return IfsFalse;
			}
		}
        // Rewind the index file to start processing from the first index entry
        rewind(tinfo.ifsHandle->pNdex);

        char dest[MAX_PATH] = "./";

        if(desdir && (strlen(desdir) > 0))
        {
        	int len;
        	strcpy(dest, desdir);
        	len = strlen(desdir);
        	if(desdir[len-1] != '/')
        	{
        		dest[len] = '/';
        		dest[len+1] = '\0';
        	}
#if 0
        	// check to make sure directory exists, otherwise create it
			struct stat st = {0};

			if (stat(dest, &st) == -1) {
				mkdir(dest, 0700);
			}
#endif
			mkpath(dest, 0700);

        }
		// Open input/output files
		printf("Info: Opening mpeg/ts file: %s for generating trick mode stream...\n", tinfo.ts_filename);
		tinfo.pFile_ts = fopen(tinfo.ts_filename, "rb");

		if(desdir)
		{// destination directory specified, get rid of any current directory name attached
			char *ptr = NULL;
			strcpy(tmpFileName, tinfo.trick_filename);
			if((ptr = strrchr(tmpFileName, '/')) != NULL)
				strcpy(tinfo.trick_filename, (ptr+1));
		}
		else
		{
		    strcpy(dest, "");
		}
		sprintf(trick_filename, "%s%s.%s%d_1%s", dest, tinfo.trick_filename, ((trick_speed < 0) ? "-" : ""), tinfo.trick_speed, extn);
		printf("Info: Opening trick mode file: %s for generating trick mode stream...\n", trick_filename);
		tinfo.pFile_tm = fopen(trick_filename, "wb");
		sprintf(tmpFileName, "%s%s.%s%d_1%s.index", dest, tinfo.trick_filename, ((trick_speed < 0) ? "-" : ""), tinfo.trick_speed, extn);
		printf("Info: Opening trick mode index file: %s for generating trick mode indexes...\n", tmpFileName);
		tinfo.pFile_ndx = fopen(tmpFileName, "w");
		sprintf(tmpFileName, "%s%s.%s%d_1%s.info", dest, tinfo.trick_filename, ((trick_speed < 0) ? "-" : ""), tinfo.trick_speed, extn);
		printf("Info: Opening trick info file: %s \n", tmpFileName);
		tinfo.pFile_info = fopen(tmpFileName, "w");
		// check if all files opened correctly
		if(!tinfo.pFile_ts | !tinfo.pFile_tm | !tinfo.pFile_ndx || !tinfo.ifsHandle->pNdex)
		{
			printf("Error: Unable to open input-output file.\n");
			if( tinfo.pFile_ts) fclose(tinfo.pFile_ts);
			if( tinfo.pFile_tm) fclose(tinfo.pFile_tm);
			if( tinfo.pFile_ndx) fclose(tinfo.pFile_ndx);
			if( tinfo.pFile_info) fclose(tinfo.pFile_info);

			if (tinfo.ifsHandle->pNdex) fclose(tinfo.ifsHandle->pNdex);
			if(tinfo.ifsHandle)
				g_free(tinfo.ifsHandle);
			return IfsFalse;
		}

		 getPatPmtPackets(strmInfo, &tinfo);

	   // Handle the trick file generation
		if(tinfo.trick_direction == -1)
			handle_ReverseTrickFileIndexing(&tinfo);
		else
		{
		    if(tinfo.trick_speed ==  1)
		        // This method indexes all frame types (I, B, P as well as VOBUs for program streams)
		        generate_normalPlayIndexFile(&tinfo);
		    else
			    handle_ForwardTrickFileIndexing(&tinfo);
		}
	}
    else
    {
    	printf("Error: Incorrect index file name specified.\n");
    	return IfsFalse;
    }

    if(tinfo.pFile_info)
    {
		printf("framerate=%d\n", tinfo.framerate );
		fprintf(tinfo.pFile_info, "framerate=%d\n", tinfo.framerate);

    }
    if(tinfo.newTrickFile == IfsTrue)
	{
		if( tinfo.pFile_ts) fclose(tinfo.pFile_ts);
		if( tinfo.pFile_tm) fclose(tinfo.pFile_tm);
		if( tinfo.pFile_ndx) fclose(tinfo.pFile_ndx);
		if( tinfo.pFile_info) fclose(tinfo.pFile_info);
		if(trick_speed == 1)
			remove(trick_filename);
;		printf("\nInfo: Successfully generated the trick play file.\n");
	}
	return IfsTrue;
}



static unsigned IfsCheck_SEQ_PIC(IfsCodecType codecType, ullong ifsIndex)
{
		unsigned i = 0;

		if (codecType == IfsCodecTypeH264)
		{
			if (ifsIndex & IfsIndexNalUnitSeqParamSet)
				i |= 0x01;
			if (ifsIndex & IfsIndexPictureI)
				i |= 0x04;
			if(ifsIndex & IfsIndexHeaderPusiBit)
				i |= 0x08;
		}
		else if(codecType == IfsCodecTypeH262)
		{
			if (ifsIndex & IfsIndexStartSeqHeader)
				i |= 0x01;
			if (ifsIndex & IfsIndexStartPicture0)
				i |= 0x04;
			if(ifsIndex & IfsIndexHeaderPusiBit)
				i |= 0x08;

		}
		return i;
}

static IfsBoolean get_indexEntry(FILE *fpIndex, IfsIndexEntry *iEntry, unsigned long long flags)
{
    IfsBoolean bRead = IfsFalse;

    if(!(fpIndex) || !(iEntry))
    {
        printf("Error: input parameter pointer is NULL\n");
        return IfsFalse;
    }
    iEntry->what = 0;
    iEntry->realWhere = 0;

    // read entries until matching with flags
    do
    {
        if(iEntry->what & flags)
        {
            bRead = IfsTrue;
            break;
        }
    } while (fread(iEntry, 1, sizeof(IfsIndexEntry), fpIndex) == sizeof(IfsIndexEntry));
    return bRead;
}

static IfsBoolean generate_normalPlayIndexFile(trickInfo *tinfo)
{
	IfsBoolean bRead = IfsFalse;
    NumEntries numEntries = 0;
    unsigned long long ifsIndex = 0;
    int64_t frameSize = 0;
    int64_t vobuSize = 0;
    int64_t vobuOffset = 0;
    IfsIndexEntry iEntry = {0, 0, 0, 0, 0, 0};
    char frame_type[256];

    printf("Opening index file...\n");
    if (tinfo->ifsHandle->pNdex)
    {
        if (fread(&iEntry, 1, sizeof(IfsIndexEntry),
                tinfo->ifsHandle->pNdex) == sizeof(IfsIndexEntry))
        ;
    }
    else
    {
        printf("Error opening/reading the index file!\n");
        return IfsFalse;
    }

    do
    {
        ifsIndex =  iEntry.what;
        // Index vobu offsets (for program stream only)
        if(tinfo->containerType == IfsContainerTypeMpeg2Ps)
        {
            if(ifsIndex & IfsIndexStartSysHeader)
            {
                if(tinfo->firstRefVobu == IfsFalse)
                {
                    tinfo->firstRefVobu = IfsTrue;
                }
                else
                {
                    if(tinfo->refVobu->index & IfsIndexStartSysHeader)
                    {
                        NumPackets numPacks = (tinfo->refVobu->entry.packWhere-1) < 0 ? 0 : tinfo->refVobu->entry.packWhere-1;
                        strcpy(frame_type, "S S ");
                        vobuOffset = numPacks * PROGRAM_STREAM_PACK_SIZE;
                        tinfo->refVobu->pktCount =  iEntry.packWhere - tinfo->refVobu->entry.packWhere;
                        vobuSize = tinfo->refVobu->pktCount * PROGRAM_STREAM_PACK_SIZE;
                        write_indexEntries(tinfo->pFile_ndx, frame_type, tinfo->refVobu->entry.when, vobuOffset, vobuSize);
                    }
                }
                tinfo->refVobu->entry = iEntry;
                tinfo->refVobu->index = ifsIndex;
            }
        }
        // Index frame type, offset etc.
        {
            switch(tinfo->codecType)
            {
                case IfsCodecTypeH264:
                {
                    if( (ifsIndex & IfsIndexPictureI) ||
                        (ifsIndex & IfsIndexPictureP) ||
                        (ifsIndex & IfsIndexPictureB) )
                    {
                        if(tinfo->firstRefIframe == IfsFalse)
                        {
                            tinfo->firstRefIframe = IfsTrue;
                        }
                        else
                        {
                            //frame type
                            get_frameTypeH264(tinfo->refIframe->index, frame_type);
                            //byte offset and time entry values
                            tinfo->byteOffset = (tinfo->refIframe->entry.realWhere * tinfo->ifsHandle->pktSize);
                            tinfo->refIframe->pktCount =  iEntry.realWhere - tinfo->refIframe->entry.realWhere;
                            frameSize = tinfo->refIframe->pktCount * tinfo->ifsHandle->pktSize;
                            write_indexEntries(tinfo->pFile_ndx, frame_type, tinfo->refIframe->entry.when, tinfo->byteOffset, frameSize);
                        }
                        tinfo->refIframe->entry = iEntry;
                        tinfo->refIframe->index = ifsIndex;
                    }
                }
                    break;
                case IfsCodecTypeH262:
                {
                    if(ifsIndex & IfsIndexStartPicture)
                    {
                        if(tinfo->firstRefIframe == IfsFalse)
                        {
                            tinfo->firstRefIframe = IfsTrue;
                        }
                        else
                        {
                            get_frameTypeH262(tinfo->refIframe->index, frame_type);
                            tinfo->byteOffset = (tinfo->refIframe->entry.realWhere * tinfo->ifsHandle->pktSize);
                            tinfo->refIframe->pktCount =  iEntry.realWhere - tinfo->refIframe->entry.realWhere;
                            frameSize = tinfo->refIframe->pktCount * tinfo->ifsHandle->pktSize;
                            write_indexEntries(tinfo->pFile_ndx, frame_type, tinfo->refIframe->entry.when, tinfo->byteOffset, frameSize);
                        }
                        tinfo->refIframe->entry = iEntry;
                        tinfo->refIframe->index = ifsIndex;
                    }
                }
                    break;
                default:
                    break;
            }
        }
        numEntries++;
    } while (fread(&iEntry, 1, sizeof(IfsIndexEntry),
            tinfo->ifsHandle->pNdex) == sizeof(IfsIndexEntry));

    // Write the last entries
    if(tinfo->refVobu->index & IfsIndexStartSysHeader)
    {
        NumPackets numPacks = (tinfo->refVobu->entry.packWhere-1) < 0 ? 0 : tinfo->refVobu->entry.packWhere-1;
        strcpy(frame_type, "S S ");
        vobuOffset = numPacks * PROGRAM_STREAM_PACK_SIZE;
        tinfo->refVobu->pktCount =  iEntry.packWhere - tinfo->refVobu->entry.packWhere;
        vobuSize = tinfo->refVobu->pktCount * PROGRAM_STREAM_PACK_SIZE;
        write_indexEntries(tinfo->pFile_ndx, frame_type, tinfo->refVobu->entry.when, vobuOffset, vobuSize);
    }
    switch(tinfo->codecType)
    {
        case IfsCodecTypeH264:
            if( (tinfo->refIframe->index & IfsIndexPictureI) ||
                (tinfo->refIframe->index & IfsIndexPictureP) ||
                (tinfo->refIframe->index & IfsIndexPictureB) )
            {
                get_frameTypeH264(tinfo->refIframe->index, frame_type);
                tinfo->byteOffset = (tinfo->refIframe->entry.realWhere * tinfo->ifsHandle->pktSize);
                tinfo->refIframe->pktCount =  iEntry.realWhere - tinfo->refIframe->entry.realWhere;
                frameSize = tinfo->refIframe->pktCount * tinfo->ifsHandle->pktSize;
                write_indexEntries(tinfo->pFile_ndx, frame_type, tinfo->refIframe->entry.when, tinfo->byteOffset, frameSize);
            }
            break;
        case IfsCodecTypeH262:
            if(tinfo->refIframe->index & IfsIndexStartPicture)
            {
                get_frameTypeH262(tinfo->refIframe->index, frame_type);
                tinfo->byteOffset = (tinfo->refIframe->entry.realWhere * tinfo->ifsHandle->pktSize);
                tinfo->refIframe->pktCount =  iEntry.realWhere - tinfo->refIframe->entry.realWhere;
                frameSize = tinfo->refIframe->pktCount * tinfo->ifsHandle->pktSize;
                write_indexEntries(tinfo->pFile_ndx, frame_type, tinfo->refIframe->entry.when, tinfo->byteOffset, frameSize);
            }
            break;
        default:
            break;
    }
    return bRead;
}

static void write_indexEntries(FILE *index_file, char *frame_type, IfsClock time, int64_t byte_offset, int64_t frame_size)
{
    char temp1[32]; // IfsToSecs only
    float secx;
    fprintf(index_file, "%s", frame_type);
    secx = IfsConvertToSecs(IfsToSecs(time, temp1));
    //printf("time = %011.3f byteOffset = %019lld  frameSize = %010lld \n", secx,
    //        byte_offset, frameSize);
    fprintf(index_file, "%011.3f %019lld %010lld %14c\n", secx,  byte_offset, frame_size, ' ');
}

static void get_frameTypeH262(unsigned long long index, char* frame_type)
{
    switch (index & IfsIndexStartPicture)
    {
        case IfsIndexStartPicture0:
            strcpy(frame_type, "V I ");
            break;
        case IfsIndexStartPicture1:
            strcpy(frame_type, "V P ");
            break;
        case IfsIndexStartPicture:
            strcpy(frame_type, "V B ");
            break;
        default:
            break;
    }
}

static void get_frameTypeH264(unsigned long long index, char* frame_type)
{
    if (index & IfsIndexPictureI)
    {
        strcpy(frame_type, "V I ");
    }
    else if (index & IfsIndexPictureP)
    {
        strcpy(frame_type, "V P ");
    }
    else if (index & IfsIndexPictureB)
    {
        strcpy(frame_type, "V B ");
    }
}

static IfsBoolean get_indexEntrySet(FILE *fpIndex, entrySet *iEntrySet)
{
    IfsBoolean result = IfsFalse;
    IfsIndexEntry entry = {0, 0, 0, 0, 0, 0};
    unsigned long foundWhat = 0;

    if(!(fpIndex) || !(iEntrySet))
    {
        printf("Error: input parameter pointer is NULL\n");
        return IfsFalse;
    }
    // clear the entries
    iEntrySet->entry_AUD = entry;
    iEntrySet->entry_SPS = entry;
    iEntrySet->entry_IframeB =  iEntrySet->entry_IframeE;
    iEntrySet->entry_IframeE = entry;


    switch(iEntrySet->codecType)
    {
        case IfsCodecTypeH264:  // H.264
            // first get the AUD or SPS entry
            if(get_indexEntry(fpIndex, &entry, (IFS_FLAG_H264_AUD | IFS_FLAG_H264_SPS)) )
            {
                if(entry.what & IFS_FLAG_H264_AUD)
                {
                    foundWhat = IFS_FLAG_H264_AUD;
                    iEntrySet->entry_AUD = entry;
                    //printf("Info: Fetch -> AUD entry found, packet index: %06ld\n", entry.realWhere);
                }

                // check for SPS
                if(entry.what & IFS_FLAG_H264_SPS)
                {
                    foundWhat = IFS_FLAG_H264_SPS;
                    iEntrySet->entry_SPS = entry;
                    //printf("Info: SPS entry found, packet index: %06ld\n", entry.realWhere);
                }
                else if(entry.what & (IFS_FLAG_H264_PIC_I | IFS_FLAG_H264_PIC_B | IFS_FLAG_H264_PIC_P))
                {
                    return IfsFalse;
                }
                else if(get_indexEntry(fpIndex, &entry, (IFS_FLAG_H264_SPS | IFS_FLAG_H264_PIC_I | IFS_FLAG_H264_PIC_B | IFS_FLAG_H264_PIC_P)))
                {
                    if(!(entry.what & IFS_FLAG_H264_SPS))   // Wrong AUD entry, SPS is expected here
                        return IfsFalse;
                    foundWhat = IFS_FLAG_H264_SPS;
                    iEntrySet->entry_SPS = entry;
                    //printf("Info: Fetch -> SPS entry found, packet index: %06ld\n", entry.realWhere);
                }
                else
                {
                    printf("Error: SPS/SEQ not found\n");
                    return IfsFalse;
                }
                // check if AUD found, else set AUD entry to SPS
                if(!(foundWhat & IFS_FLAG_H264_AUD))
                    iEntrySet->entry_AUD = iEntrySet->entry_SPS;

                // Now, check for I-frame
                if(entry.what & IFS_FLAG_H264_PIC_I)
                {
                    foundWhat = IFS_FLAG_H264_PIC_I;
                    iEntrySet->entry_IframeB = entry;
                    //printf("Info: I frame entry found, packet index: %06ld\n", entry.realWhere);
                }
                else if(get_indexEntry(fpIndex, &entry, IFS_FLAG_H264_PIC_I))
                {
                    foundWhat = IFS_FLAG_H264_PIC_I;
                    iEntrySet->entry_IframeB = entry;
                    //printf("Info: Fetch -> I-frame entry found, packet index: %06ld\n", entry.realWhere);
                }
                else
                {
                    printf("Error: I frame entry not found\n");
                    return IfsFalse;
                }

                // here we should have the I-frame entry
                // now get the end of the I-frame i.e. next B,P,I,or,SPS etc.
                if(get_indexEntry(fpIndex, &entry, IFS_FLAG_H264_PIC_IEnd))
                {
                    iEntrySet->entry_IframeE = entry;
                    //printf("Info: Fetch -> I-frame entry found, packet index: %06ld\n", entry.realWhere);
                    result = IfsTrue;   // now we have every entry needed
                }
            }
            else // did not get the entry
            {
                printf("Error: AUD or SPS not found, possible end of file\n");
            }
            break;
        case IfsCodecTypeH262: // H.262
            // first get the AUD or SPS entry
            //if(get_indexEntry(fpIndex, &entry, (IFS_FLAG_H262_SEQ | IFS_FLAG_H262_GOP)) )
            if(get_indexEntry(fpIndex, &entry, IFS_FLAG_H262_SEQ) )
            {
                // check for SEQ
                if(entry.what & IFS_FLAG_H262_SEQ)
                {
                    foundWhat = IFS_FLAG_H262_SEQ;
                    iEntrySet->entry_SPS = entry;
                    printf("Info: Fetch --> Seq Hdr entry found, packet index: %06ld\n", entry.realWhere);
                    printf("Index: 0X%08lX %08lX\n", (unsigned long) (entry.what >> 32), (unsigned long)entry.what);
                }
                else
                {
                    printf("Error: SEQ header not found\n");
                    return IfsFalse;
                }

                if(entry.what & IFS_FLAG_H262_GOP)  //check if got GOP, it may not be there
                {
                    foundWhat = IFS_FLAG_H262_GOP;
                    iEntrySet->entry_AUD = entry;
                    printf("Info: GOP entry found, packet index: %06ld\n", entry.realWhere);
                }
                else
                {   // set it to SEQ
                    iEntrySet->entry_AUD = iEntrySet->entry_SPS;
                }

                // Now, check for I-frame, we might have I frame
                if(entry.what & IFS_FLAG_H262_PIC_I)
                {
                    foundWhat = IFS_FLAG_H262_PIC_I;
                    iEntrySet->entry_IframeB = entry;
                    printf("Info: I-frameB entry found, packet index: %06ld\n", entry.realWhere);
                }
                else    // I not found yet, so get it
                if(get_indexEntry(fpIndex, &entry, IFS_FLAG_H262_PIC_I))
                {
                    //if(entry.what & IFS_FLAG_H262_PIC_I)
                    {
                        foundWhat = IFS_FLAG_H262_PIC_I;
                        iEntrySet->entry_IframeB = entry;
                        printf("Info: I-frameB entry found, packet index: %06ld\n", entry.realWhere);
                    }
                }
                else
                {
                    printf("Error: I frame entry not found\n");
                    return IfsFalse;
                }

                // here we should have the I-frame entry
                // now get the end of the I-frame i.e. next B,P,I,or,SPS etc.
                if(get_indexEntry(fpIndex, &entry, IFS_FLAG_H262_PIC_IEnd))
                {
                    iEntrySet->entry_IframeE = entry;
                    printf("Info: Fetch -> I-frameE entry found, packet index: %06ld\n", entry.realWhere);
                    result = IfsTrue;   // now we have every entry needed
                }
            }
            else
            {
                printf("Error: SEQ header not found\n");
                return IfsFalse;
            }
            break;
        default:
            printf("Error: Invalid stream format specified: %d\n", iEntrySet->codecType);
            return IfsFalse;
    }
    return result;
}

static IfsBoolean get_vobuIndexEntrySet(FILE *fpIndex, entrySet *iEntrySet)
{
    IfsBoolean result = IfsFalse;
    IfsIndexEntry entry = {0, 0, 0, 0};
    unsigned long foundWhat = 0;

    if(!(fpIndex) || !(iEntrySet))
    {
        printf("Error: input parameter pointer is NULL\n");
        return IfsFalse;
    }
    iEntrySet->entry_sys_hdr_VobuB =  entry;

    switch(iEntrySet->codecType)
    {
        case IfsCodecTypeH264:  // H.264
            break;
        case IfsCodecTypeH262: // H.262
            // Find system header (specific to program streams)
            if(iEntrySet->containerType == IfsContainerTypeMpeg2Ps)
            {
                //printf("Find system header..\n");
                {
                    if(get_indexEntry(fpIndex, &entry, IfsIndexStartSysHeader))
                    {
                        iEntrySet->entry_sys_hdr_VobuB = entry;
                        printf("Info: Fetch -> entry_sys_hdr_VobuB entry found, pack index: %06ld\n", entry.packWhere);
                        result = IfsTrue;
                    }
                }
                //printf("Find seq header..\n");
                {
                    {
                        // check for SEQ
                        if(entry.what & IFS_FLAG_H262_SEQ)
                        {
                            foundWhat = IFS_FLAG_H262_SEQ;
                            iEntrySet->entry_SPS = entry;
                            printf("Info: Fetch --> Seq Hdr entry found, packet index: %06ld\n", entry.realWhere);
                            printf("Index: 0X%08lX %08lX\n", (unsigned long) (entry.what >> 32), (unsigned long)entry.what);
                        }
                        else
                        if(get_indexEntry(fpIndex, &entry, IFS_FLAG_H262_SEQ))
                        {
                            foundWhat = IFS_FLAG_H262_SEQ;
                            iEntrySet->entry_SPS = entry;
                            printf("Info: Fetch --> Seq Hdr entry found, packet index: %06ld\n", entry.realWhere);
                        }
                        else
                        {
                            printf("Error: SEQ header not found\n");
                            return IfsFalse;
                        }

                        if(entry.what & IFS_FLAG_H262_GOP)  //check if got GOP, it may not be there
                        {
                            foundWhat = IFS_FLAG_H262_GOP;
                            iEntrySet->entry_AUD = entry;
                            printf("Info: GOP entry found, packet index: %06ld\n", entry.realWhere);
                        }
                        else
                        {   // set it to SEQ
                            iEntrySet->entry_AUD = iEntrySet->entry_SPS;
                        }

                        // Now, check for I-frame, we might have I frame
                        if(entry.what & IFS_FLAG_H262_PIC_I)
                        {
                            foundWhat = IFS_FLAG_H262_PIC_I;
                            iEntrySet->entry_IframeB = entry;
                            printf("Info: I-frameB entry found, packet index: %06ld\n", entry.realWhere);
                        }
                        else    // I not found yet, so get it
                        if(get_indexEntry(fpIndex, &entry, IFS_FLAG_H262_PIC_I))
                        {
                            //if(entry.what & IFS_FLAG_H262_PIC_I)
                            {
                                foundWhat = IFS_FLAG_H262_PIC_I;
                                iEntrySet->entry_IframeB = entry;
                                printf("Info: I-frameB entry found, packet index: %06ld\n", entry.realWhere);
                            }
                        }
                        else
                        {
                            printf("Error: I frame entry not found\n");
                            return IfsFalse;
                        }

                        // here we should have the I-frame entry
                        // now get the end of the I-frame i.e. next B,P,I,or,SPS etc.
                        if(get_indexEntry(fpIndex, &entry, IFS_FLAG_H262_PIC_IEnd))
                        {
                            iEntrySet->entry_IframeE = entry;
                            printf("Info: Fetch -> I-frameE entry found, packet index: %06ld\n", entry.realWhere);
                            result = IfsTrue;   // now we have every entry needed
                        }
                    }
                }
            }

            break;
        default:
            printf("Error: Invalid stream format specified: %d\n", iEntrySet->codecType);
            return IfsFalse;
    }
    return result;
}

static IfsBoolean handle_ReverseTrickFileIndexing(trickInfo *tinfo)
{
	entrySet	thisSet;
	unsigned int entrySetCount = 0;
	int64_t last_byte_offset=0;

	if(!tinfo)
	{
		printf("Error: input parameter is NULL\n");
		return IfsFalse;
	}
	tinfo->trick_entryset = &thisSet;
	thisSet.codecType = tinfo->codecType;
    thisSet.containerType = tinfo->containerType;
	thisSet.valid = IfsFalse;

	if(tinfo->ifsHandle->pNdex)
	{
		unsigned int iCnt = 1;
	    char temp1[32]; // IfsToSecs only
	    char temp2[256]; // ParseWhat
	    IfsBoolean ret = IfsFalse;

		// goto the end of the file
        fseek(tinfo->ifsHandle->pNdex, 0, SEEK_END);
        while (fseek(tinfo->ifsHandle->pNdex, -(sizeof(IfsIndexEntry)*iCnt++), SEEK_END) >= 0)
        {
            if(fread(&tinfo->ifsHandle->entry, 1, sizeof(IfsIndexEntry), tinfo->ifsHandle->pNdex) == sizeof(IfsIndexEntry))
            {
            	if(IfsCheck_SEQ_PIC(tinfo->codecType, tinfo->ifsHandle->entry.what) & IFS_TEST_FLAG_SPS)
				{
					// go back by one entry, point to the found entry
			       	fseek(tinfo->ifsHandle->pNdex, -(sizeof(IfsIndexEntry)*(iCnt-1)), SEEK_END);
			       	printf("Info: entry: %06d -  %6ld %s  %s\n", iCnt, tinfo->ifsHandle->entry.realWhere,
							IfsToSecs(tinfo->ifsHandle->entry.when, temp1),
							tinfo->parseWhat(tinfo->ifsHandle, temp2, IfsIndexDumpModeDef, IfsTrue));
			        // Compute VOBU index data (specific to program streams only)
			        if(thisSet.containerType == IfsContainerTypeMpeg2Ps)
			        {
			            // Add vobu indexes to the index files
                        ret = get_vobuIndexEntrySet(tinfo->ifsHandle->pNdex, &thisSet);
			        }
			        else
			        {
	                   ret = get_indexEntrySet(tinfo->ifsHandle->pNdex, &thisSet);
			        }
                    if(ret == IfsTrue)
                    {
                        thisSet.valid = IfsTrue;
                        IfsCopyEntrySetData(tinfo);
                        thisSet.valid = IfsFalse;
                        entrySetCount++;
                        last_byte_offset = tinfo->byteOffset + (tinfo->refIframe->pktCount* tinfo->ifsHandle->pktSize);
                        printf("%019lld %010lld\n ", last_byte_offset,  tinfo->refIframe->pktCount* tinfo->ifsHandle->pktSize);
                    }
				}
            }
        }
        {
            // Add a 0-time entry as the last index for negative trick streams
            float secx = 0;
            int frame_size=0;
            printf("%019lld %010d\n ", last_byte_offset,  frame_size);
            fprintf(tinfo->pFile_ndx, "V I %011.3f %019lld %010d %14c\n", secx,
                    last_byte_offset,  frame_size, ' ');
        }
	}
	// read entry set from Index file
	printf("Info: Total entrySet Count: %d\n", entrySetCount);
	return IfsTrue;
}


static IfsBoolean handle_ForwardTrickFileIndexing(trickInfo *tinfo)
{
	entrySet	thisSet;
	unsigned int entrySetCount = 0;
    IfsBoolean ret = IfsFalse;

	if(!tinfo)
	{
		printf("Error: input parameter is NULL\n");
		return IfsFalse;
	}
	tinfo->trick_entryset = &thisSet;
	thisSet.codecType = tinfo->codecType;
	thisSet.containerType = tinfo->containerType;
	thisSet.valid = IfsFalse;

    if(tinfo->ifsHandle->pNdex)
    {
        do
        {
            // Compute VOBU index data (specific to program streams only)
            if(thisSet.containerType == IfsContainerTypeMpeg2Ps)
            {
                // Add vobu indexes to the index files
                ret = get_vobuIndexEntrySet(tinfo->ifsHandle->pNdex, &thisSet);
            }
            else
            {
                ret = get_indexEntrySet(tinfo->ifsHandle->pNdex, &thisSet);
            }
            if(ret == IfsTrue)
            {
                thisSet.valid = IfsTrue;
                IfsCopyEntrySetData(tinfo);
                thisSet.valid = IfsFalse;
            }
            entrySetCount++;
        }   while(!feof(tinfo->ifsHandle->pNdex));
        // copy the last entry
        thisSet.valid = IfsTrue;
        IfsCopyEntrySetData(tinfo);
    }
    // read entry set from Index file
    printf("Info: Total entrySet Count: %d\n", entrySetCount);

	return IfsTrue;
}

static IfsBoolean IfsCopyVobuIndexData(trickInfo *tinfo)
{
    IfsBoolean retVal = IfsFalse;

    if(tinfo->pFile_ndx)
    {
        char temp1[32];
        float secx;
        if(tinfo->containerType == IfsContainerTypeMpeg2Ps)
        {
            // write vobu index
            // vobu is between two system headers
            int numPacks = tinfo->refVobu->pktCount;
            tinfo->refVobu->vobu_size = (numPacks*PROGRAM_STREAM_PACK_SIZE);
            secx = IfsConvertToSecs(IfsToSecs(tinfo->refVobu->entry.when, temp1));
            // The last vobu size cannot be determined from the index info
            // leave it as 0 as an indication to rygel to use file size to
            // determine size of last vobu as ---> file_size - last_byte_offset
            if(tinfo->currentVobu->entry.packWhere == 0)
                tinfo->refVobu->vobu_size = 0;
            //((vobu_size > 0) ? vobu_size-1: 0)
            printf("time = %011.3f byte_offset = %019lld vobu_size = %010lld \n", secx,
                    tinfo->refVobu->byteOffset, tinfo->refVobu->vobu_size);
            fprintf(tinfo->pFile_ndx, "S S %011.3f %019lld %010lld %14c\n", secx,
                    tinfo->refVobu->byteOffset,  tinfo->refVobu->vobu_size, ' ');
            tinfo->refVobu->byteOffset += tinfo->refVobu->vobu_size;
        }
        retVal = IfsTrue;
    }
    return retVal;
}

static IfsBoolean IfsCopyIndexData(trickInfo *tinfo)
{
	IfsBoolean retVal = IfsFalse;

	if(tinfo->pFile_ndx)
	{

		char temp1[32];
		float secx;
        if(tinfo->refIframe->speed == 1)
        {
            // For normal play index file the byte offsets are relative to the
            // original media file
            tinfo->byteOffset = (tinfo->refIframe->entry.realWhere * tinfo->ifsHandle->pktSize);
        }
        //-------------------
        {
 		secx = IfsConvertToSecs(IfsToSecs(tinfo->refIframe->entry.when, temp1));
        printf("time = %011.3f tinfo->byteOffset = %019lld  tinfo->refIframe->pktCount = %d tinfo->ifsHandle->pktSize = %d\n", secx,
                tinfo->byteOffset, (int) tinfo->refIframe->pktCount,  (int)tinfo->ifsHandle->pktSize);
		fprintf(tinfo->pFile_ndx, "V I %011.3f %019lld %010lld %14c\n", secx,
				tinfo->byteOffset,	tinfo->refIframe->pktCount* tinfo->ifsHandle->pktSize, ' ');
        }
	    retVal = IfsTrue;
	}
	return retVal;
}

static IfsBoolean IfsCopyEntrySetData(trickInfo *tinfo)
{
    char temp1[32]; // IfsToSecs only
    IfsBoolean copyFrameData = IfsFalse;
    IfsBoolean copyVobuData = IfsFalse;
	if(tinfo->trick_entryset->valid == IfsTrue)
	{
	    {
	        if(tinfo->containerType == IfsContainerTypeMpeg2Ps)
	        {
	            tinfo->currentVobu->entry = tinfo->trick_entryset->entry_sys_hdr_VobuB;
	            if(tinfo->firstRefVobu == IfsFalse)
	            {
	                tinfo->firstRefVobu = IfsTrue;
	            }
	            else
	            {
	                if(tinfo->trick_speed == 1) // for normal speed, just copy the index data only
	                {
	                    printf("Info: tinfo->currentVobu->entry.packWhere: %d tinfo->refVobu->entry.packWhere: %d\n",
	                            (int)tinfo->currentVobu->entry.packWhere, (int)tinfo->refVobu->entry.packWhere);
                        tinfo->refVobu->pktCount =  tinfo->currentVobu->entry.packWhere -
                                    tinfo->refVobu->entry.packWhere;
	                    IfsCopyVobuIndexData(tinfo);
	                }
	                else
	                    copyVobuData = IfsTrue;
	            }
	        }
            tinfo->ifsHandle->entry = tinfo->trick_entryset->entry_IframeB;

            printf("Info: Next I frame: %ld, packet index: %6ld - time stamp:  %s\n",
                ++tinfo->uIframeNum, tinfo->ifsHandle->entry.realWhere,
                IfsToSecs(tinfo->ifsHandle->entry.when, temp1));

            if(tinfo->firstRefIframe == IfsFalse)
            {
                tinfo->newTrickFile = IfsTrue;
                tinfo->firstRefIframe = IfsTrue;
                tinfo->refIframe->speed = tinfo->trick_speed;
                tinfo->total_frame_count = 0;
            }
            else
            {
                if(tinfo->trick_speed == 1) // for normal speed, just copy the index data only
                    IfsCopyIndexData(tinfo);
                else
                    copyFrameData = IfsTrue;
            }
	    }

	    if(tinfo->containerType == IfsContainerTypeMpeg2Ps)
	    {
	        if(copyVobuData && copyFrameData)
	            IfsCopyFrameData(tinfo);
	    }
	    else
	    {
	        if(copyFrameData)
	            IfsCopyFrameData(tinfo);
	    }

	    IfsSaveReferenceFrameData(tinfo);
	}
	return IfsTrue; // check the return value in this function
}

static void IfsSaveReferenceFrameData(trickInfo *tinfo)
{
    {
        // save current I frame as new Reference frame
        tinfo->refIframe->entry = tinfo->ifsHandle->entry;
        tinfo->refIframe->index = 0;
        tinfo->refIframe->uIframeNum = tinfo->uIframeNum;
        {   // point the reference I-frame where to SPS or AUD, because these are the entries to be copied along I-frame
            // I-frame happens after or at the same entry as of AUD and SPS
            NumPackets whereAUDorSPS = (tinfo->ifsHandle->codecType == IfsCodecTypeH264) ?
                                            tinfo->trick_entryset->entry_AUD.realWhere :
                                            tinfo->trick_entryset->entry_SPS.realWhere;
            tinfo->refIframe->pktCount = tinfo->trick_entryset->entry_IframeE.realWhere - whereAUDorSPS;
            tinfo->refIframe->entry.realWhere = whereAUDorSPS;
            if(!((IfsCheck_SEQ_PIC(tinfo->codecType, tinfo->trick_entryset->entry_IframeE.what)) & IFS_TEST_FLAG_PUSI))
                tinfo->refIframe->pktCount++;   // add 1 more packet for partial I frame
        }
        printf("\n==============================\n");
        printf("Info: refIFrame->realWhere: %06ld\n",
                tinfo->refIframe->entry.realWhere);
        printf("==============================\n");
        if(tinfo->containerType == IfsContainerTypeMpeg2Ps)
        {
            // save the current VOBU as a Reference
            printf("Info: tinfo->currentVobu->entry.packWhere: %d tinfo->refVobu->entry.packWhere: %d\n",
                    (int)tinfo->currentVobu->entry.packWhere, (int)tinfo->refVobu->entry.packWhere);
            if(tinfo->trick_direction == -1)
            {
                tinfo->refVobu->pktCount =  tinfo->refVobu->entry.packWhere -
                        tinfo->currentVobu->entry.packWhere;
            }
            else
            {
                if(tinfo->trick_speed > 1)
                {
                    tinfo->refVobu->pktCount =  tinfo->currentVobu->entry.packWhere -
                            tinfo->refVobu->entry.packWhere;
                }
            }

            tinfo->refVobu->entry = tinfo->currentVobu->entry;
        }
    }
}


static IfsBoolean IfsCopyPackets(FILE *infile_ts, FILE *outfile_tm, unsigned long pktCount, unsigned pktSize)
{
	char *tmpBuf;
	IfsBoolean rVal = IfsFalse;

	if(!infile_ts || !outfile_tm)
	{
		printf("Error: Input/Output file pointers are NULL\n");
		return IfsFalse;
	}
	else if(pktCount < 1)
	{
		printf("Error: Packet count is 0, nothing to copy\n");
		return IfsFalse;
	}
//	printf("Info: Copying %06ld packets to trick play file\n", pktCount);
	tmpBuf = g_try_malloc(pktSize * pktCount);
	if(tmpBuf == NULL)
		printf("Error: Error allocating buffer\n");
	else
	{
		if(fread(tmpBuf, pktSize, pktCount,  infile_ts) == pktCount)
		{
			int x;

			x = fwrite(tmpBuf, pktSize, pktCount, outfile_tm);	// write packet to trick file
			if(x != pktCount)
				printf("Error: Error writing %06ld packets\n", pktCount);
			else
			{
//				printf("Info: Copied %06ld packets\n", pktCount);
				rVal = IfsTrue;
			}
		}
		else
		{
			printf("Error: Error reading the data from input file\n");
		}
	}
	if(tmpBuf)
		g_free(tmpBuf);
	return rVal;
}


static IfsBoolean IfsCopyFrameData(trickInfo *tinfo)
{
 	char temp1[32]; // IfsToSecs onl
	char temp2[32]; // IfsToSecs onl
	float secs = 0,  factor = 0, count = 0;
	long repeat_count = 0;

	if (tinfo->total_frame_count == 0 )
	{
		printf("Info: First frame\n");
		repeat_count = 1;			// always copy the first frame
	}

	if(tinfo->refIframe->speed > 1)
	{
		tinfo->refIframe->timeToNextI =  (tinfo->ifsHandle->entry.when - tinfo->refIframe->entry.when);

		secs = IfsConvertToSecs(IfsToSecs(tinfo->refIframe->timeToNextI, temp1));

    	factor = (tinfo->refIframe->speed*NUMBER_OF_IFRAMES_SEC <= MAX_FRAMES_PER_SEC_LIMIT) ? 1/NUMBER_OF_IFRAMES_SEC :
														((tinfo->refIframe->speed*NUMBER_OF_IFRAMES_SEC)/MAX_FRAMES_PER_SEC_LIMIT)*(1/NUMBER_OF_IFRAMES_SEC);
    	if(tinfo->framerate == 0)
    	{
    		tinfo->framerate = (int) (tinfo->refIframe->speed*NUMBER_OF_IFRAMES_SEC <= MAX_FRAMES_PER_SEC_LIMIT) ?
    												(tinfo->refIframe->speed*NUMBER_OF_IFRAMES_SEC) :
    												MAX_FRAMES_PER_SEC_LIMIT;
    	}
		if(tinfo->trick_direction == -1)
			secs = tinfo->trick_direction * secs;

		if(secs < factor)
		{
			tinfo->ifsHandle->entry.when = tinfo->refIframe->entry.when;
			printf("Info: factor is greater than secs, skip this frame\n");
			repeat_count = 0;
	        if(tinfo->containerType == IfsContainerTypeMpeg2Ps)
	        {
	            tinfo->currentVobu->entry.when = tinfo->refVobu->entry.when;
	        }
		}
		else
		{
			count = secs/factor;
			repeat_count += round(count);
		}
	}

 	printf("\nReference I frame Info:\n");
	printf("Frame number = %ld\nPacket index = %ld\nPacket count = %d\nTime stamp = %s\nTimeToNextI = %s\n",
		tinfo->refIframe->uIframeNum,
		tinfo->refIframe->entry.realWhere,
		tinfo->refIframe->pktCount,
		IfsToSecs(tinfo->refIframe->entry.when, temp1),
		IfsToSecs(tinfo->refIframe->timeToNextI, temp2));
	printf("Info: Time from Ref to next I = %f\n", secs);
	printf("Info: Count of Ref frames to repeat = %ld\n", repeat_count);


	if(repeat_count <= 0)
		return IfsFalse;	// return if nothing to copy

	if((tinfo->pFile_ts != NULL) && (tinfo->pFile_tm != NULL))
	{
		IfsBoolean addPatPmt = IfsTrue;

		if(tinfo->patPackets && tinfo->pmtPackets && (addPatPmt == IfsTrue))
		{ // copy pat/ pmt packets
			int x;
			x = fwrite(tinfo->patPackets, 1, tinfo->patByteCount, tinfo->pFile_tm);	// write packet to trick file
			if(x != tinfo->patByteCount)
				printf("Error: Error writing %06d PAT bytes\n", tinfo->patByteCount);
			else
			{
				printf("Debug: Copied %d Pat bytes\n", tinfo->patByteCount);
			}

			x = fwrite(tinfo->pmtPackets, 1, tinfo->pmtByteCount, tinfo->pFile_tm);	// write packet to trick file
			if(x != tinfo->pmtByteCount)
				printf("Error: Error writing %06d PMT bytes\n", tinfo->pmtByteCount);
			else
			{
				printf("Debug: Copied %d Pmt bytes\n", tinfo->pmtByteCount);
			}
		}

		if(repeat_count) // transfer packets
		{
			unsigned repeat_times = repeat_count;
			unsigned long pktCount = 0;

			while(repeat_times-- > 0)
			{
		        if(tinfo->containerType == IfsContainerTypeMpeg2Ps)
		        {
                    int numPacks = tinfo->refVobu->pktCount;
                    long iframe_packet_count = 0;
                    int64_t i_frame_offset = 0;
                    int64_t start_offset = tinfo->refVobu->entry.realWhere * tinfo->ifsHandle->pktSize;
                    // Seek to the correct place in the media file and copy the VOBU
	                if(fseek(tinfo->pFile_ts, start_offset, SEEK_SET))
	                    printf("Error: seek error\n");
	                // This should be number of packs to copy
	                pktCount = numPacks;
	                printf("Info: Total packs to copy: %ld from packet position: %ld\n", pktCount, tinfo->refIframe->entry.realWhere);
	                IfsCopyPackets(tinfo->pFile_ts, tinfo->pFile_tm, numPacks, PROGRAM_STREAM_PACK_SIZE);
	                printf("Copied %lu packs of size: %d \n",
	                        (unsigned long)pktCount, PROGRAM_STREAM_PACK_SIZE);
	                IfsCopyVobuIndexData(tinfo);
	                IfsCopyIndexData(tinfo);
	                // For trick files we are only copying the VOBUs and the byte offsets
	                // should be relative to the newly generated trick file
                    printf("Info: tinfo->ifsHandle->entry.realWhere: %d tinfo->refVobu->entry.realWhere: %d\n",
                                    (int)tinfo->ifsHandle->entry.realWhere, (int)tinfo->refVobu->entry.realWhere);
                    if(tinfo->trick_direction == -1)
                    {
                        iframe_packet_count = tinfo->refVobu->entry.realWhere - tinfo->ifsHandle->entry.realWhere;
                    }
                    else
                    {
                        iframe_packet_count = tinfo->ifsHandle->entry.realWhere - tinfo->refVobu->entry.realWhere;
                    }
	                i_frame_offset = iframe_packet_count * tinfo->ifsHandle->pktSize;
                    printf("Info: i_frame_offset: %lld tinfo->refVobu->vobu_size: %lld\n",
                                   i_frame_offset, tinfo->refVobu->vobu_size);
	                tinfo->byteOffset += (i_frame_offset + tinfo->refVobu->vobu_size);
                    tinfo->refVobu->vobu_size =  numPacks * PROGRAM_STREAM_PACK_SIZE;
		        }
		        else
		        {
	                if(fseek(tinfo->pFile_ts, (tinfo->refIframe->entry.realWhere * tinfo->ifsHandle->pktSize), SEEK_SET))
	                    printf("Error: seek error\n");

	                pktCount = tinfo->refIframe->pktCount;
                    printf("Info: Total packets to copy: %d from packet position: %ld\n", tinfo->refIframe->pktCount, tinfo->refIframe->entry.realWhere);

	                IfsCopyPackets(tinfo->pFile_ts, tinfo->pFile_tm, pktCount, tinfo->ifsHandle->pktSize);
	                printf("Copied %lu packets of size: %lu \n",
	                        (unsigned long)pktCount, (unsigned long)tinfo->ifsHandle->pktSize);

	                IfsCopyIndexData(tinfo);
	                //
	                // For trick files we are only copying the I-frames and the byte offsets
	                // should be relative to the newly generated trick file
	                tinfo->byteOffset += (tinfo->refIframe->pktCount * tinfo->ifsHandle->pktSize);
		        }
				tinfo->total_frame_count++;
			}
	        // add up pat/pmt byte count to the total for offset calculations
			// transport stream content only
	        tinfo->byteOffset  += (tinfo->patByteCount + tinfo->pmtByteCount);
		}
	}
	return IfsTrue;
}


static IfsBoolean getPatPmtPackets(streamInfo *strmInfo, trickInfo *tinfo)
{
	// get the PAT/PMT packets
	int i, patCnt = 0, pmtCnt = 0;
	int patByteCount = 0, pmtByteCount = 0;
	IfsBoolean result = IfsTrue;

	for (i=0; i<MAX_PAT_PKT_COUNT; i++)
		if(strmInfo->pat_pkt_nums[i] > 0)
			patCnt++;
	for (i=0; i<MAX_PMT_PKT_COUNT; i++)
		if(strmInfo->pmt_pkt_nums[i] > 0)
			pmtCnt++;

	if(patCnt & pmtCnt)
	{
		tinfo->patPackets = g_try_malloc(patCnt * strmInfo->tsPktSize);
		if(tinfo->patPackets)
		{
			tinfo->pmtPackets = g_try_malloc(pmtCnt * strmInfo->tsPktSize);
			if(!tinfo->pmtPackets)
			{
				g_free(tinfo->patPackets);
				tinfo->patPackets = NULL;
			}
			else	// get PAT/PMT packets
			{
				for(i=0; i<patCnt; i++)
				{
					if((strmInfo->pat_pkt_nums[i]-1) >= 0)
					{
						fseek(tinfo->pFile_ts, ((strmInfo->pat_pkt_nums[i]-1)*strmInfo->tsPktSize), SEEK_SET);
						if(fread(tinfo->patPackets, 1, strmInfo->tsPktSize,  tinfo->pFile_ts) != strmInfo->tsPktSize)
							result = IfsFalse;
						else
							patByteCount += strmInfo->tsPktSize;
						printf("\nDebug: Pat packet number: %d\n", strmInfo->pat_pkt_nums[i]);
						printf("Debug: Pat Byte Count: %d\n", patByteCount);
					}
				}
				for(i=0; i<pmtCnt; i++)
				{
					if((strmInfo->pmt_pkt_nums[i]-1) >= 0)
					{
						fseek(tinfo->pFile_ts, ((strmInfo->pmt_pkt_nums[i]-1)*strmInfo->tsPktSize), SEEK_SET);
						if(fread(tinfo->pmtPackets, 1, strmInfo->tsPktSize,  tinfo->pFile_ts) != strmInfo->tsPktSize)
							result = IfsFalse;
						else
							pmtByteCount += strmInfo->tsPktSize;
						printf("Debug: Pmt packet number: %d\n", strmInfo->pmt_pkt_nums[i]);
						printf("Debug: Pmt Byte Count: %d\n", pmtByteCount);
					}
				}
				if(result == IfsFalse)
				{
					if(tinfo->patPackets)
						g_free(tinfo->patPackets);
					if(tinfo->pmtPackets)
						g_free(tinfo->pmtPackets);
					tinfo->patPackets = NULL;
					tinfo->pmtPackets = NULL;
					tinfo->patByteCount = 0;
					tinfo->pmtByteCount = 0;
					printf("Error: Problem getting PAT/PMT packets.");
				}
				else
				{
					tinfo->patByteCount = patByteCount;
					tinfo->pmtByteCount = pmtByteCount;
				}
			}
		}
	}
	return result;
}


