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


#include "IfsIntf.h"
#include "getMpegTsPsi.h"
#include "ifs_file.h"
#include "ifs_impl.h"
#include "ifs_h262_parse.h"
#include "ifs_h264_parse.h"
#include "ifs_h265_parse.h"
#include "ifs_utils.h"
#include "ifs_GenTrickPlay.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif



static char* containerName[] = {"ERROR","MPEGPS","MPEGTS","MPEG4"};
static char* codecName[] = {"ERROR","H.261","H.262","H.263","H.264","H.265"};

static IfsBoolean dumpIndexFile(char *index_filename, unsigned format);
static IfsBoolean generate_IndexFile(char* inputFile, char *index_filename, IfsHandle ifsHandle, streamInfo *strmInfo);


//---------------------------

static IfsBoolean dumpIndexFile(char *index_filename, unsigned format)
{
    IfsClock duration = 0;
    time_t seconds = 0;
    IfsIndexEntry firstEntry, lastEntry;
    NumEntries numEntries = 0;
	IfsHandle ifsHandle = NULL;

    void (*countIndexes)(ullong ifsIndex) = NULL;
	char* (*parseWhat)(IfsHandle ifsHandle, char * temp,
						const IfsIndexDumpMode ifsIndexDumpMode, const IfsBoolean) = NULL;
    void (*dumpIndexes)(void) = NULL;

    if(!index_filename)
    {
    	printf("Index file name pointer is null\n");
    	return IfsFalse;
    }

    ifsHandle = g_try_malloc0(sizeof(IfsHandleImpl));
    if (ifsHandle == NULL)
    {
        printf("Could not allocate memory\n");
        return IfsFalse;
    }

    switch(format)
    {
    	case 4:	// H.264
			IfsSetCodec(ifsHandle, IfsCodecTypeH264);
			break;
    	case 2:
    	default:
    		IfsSetCodec(ifsHandle, IfsCodecTypeH262);
    		break;
    }
    printf("Opening index file: %s\n", index_filename);
	ifsHandle->pNdex = fopen(index_filename, "rb");
	if (ifsHandle->pNdex)
	{
		if (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry),
				ifsHandle->pNdex) == sizeof(IfsIndexEntry))
		;
	}
	else
	{
		printf("Error opening/reading the index file: %s\n", index_filename);
		if(ifsHandle)
			g_free(ifsHandle);
		return IfsFalse;
	}

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
			printf("IfsReturnCodeBadInputParameter: ifsHandle->codec not set\n");
			break;
	}

    firstEntry = ifsHandle->entry;

    if (ifsHandle->ndex)
        printf("\n%s:\n\n", ifsHandle->ndex);

	printf("Packet(where)  Index Information (what happened at that time and location)\n");
	printf(	"-------------  -----------------------------------------------------------\n");
	do
	{
		char temp1[32]; // IfsToSecs only
		char temp[256]; // ParseWhat

		countIndexes(ifsHandle->entry.what);

		printf("%6ld %s  %s\n", ifsHandle->entry.realWhere,
			 IfsToSecs(ifsHandle->entry.when, temp1),
			 parseWhat(ifsHandle, temp, IfsIndexDumpModeDef,
			 IfsTrue));
		numEntries++;
	} while (fread(&ifsHandle->entry, 1, sizeof(IfsIndexEntry),
			ifsHandle->pNdex) == sizeof(IfsIndexEntry));

    lastEntry = ifsHandle->entry;
    duration = lastEntry.when - firstEntry.when;

    printf("\n");

    dumpIndexes();
    printf("\n");
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
    return IfsTrue;
}

static IfsBoolean
generate_IndexFile(char *stream_filename, char *index_filename, IfsHandle ifsHandle, streamInfo *strmInfo)
{

    FILE * pInFile = NULL;
    NumBytes pktSize = IFS_TRANSPORT_PACKET_SIZE;// TODO: support multiple sizes
    NumPackets numPackets = 0;
    IfsPacket ifsPacket = { { 0 } };

    int i = 0;


    if(!strmInfo || !stream_filename)
    {
    	printf("Error: input pointers NULL\n");
    	return IfsFalse;
    }
    else
    {
        pInFile = fopen(stream_filename, "rb");
        if(!pInFile)
        {
        	printf("Error opening input file: %s\n", stream_filename);
        	return IfsFalse;
        }
    }
    // initialize local variables
    pktSize = strmInfo->tsPktSize;
	//printf("generate_IndexFile pktSize:%d\n", (int)pktSize);
	if (IfsOpenWriter(".", index_filename, 0, &ifsHandle)
			!= IfsReturnCodeNoErrorReported)
	{
		printf("Problems opening ifs writer\n");
		return IfsFalse;
	}

	// TODO: set the correct container!
	if (IfsSetContainer(ifsHandle, strmInfo->containerType,
		pktSize) != IfsReturnCodeNoErrorReported)
	{
		printf("Problems setting ifs container\n");
		return IfsFalse;
	}

	if (IfsSetCodec(ifsHandle, strmInfo->codecType)
			!= IfsReturnCodeNoErrorReported)
	{
		printf("Problems setting ifs codec\n");
		return IfsFalse;
	}

	switch(strmInfo->codecType)
	{
		case IfsCodecTypeH264:
			IfsSetMode(IfsIndexDumpModeOff, IfsH264IndexerSettingDefault);
			break;
		case IfsCodecTypeH262:
		default:
			IfsSetMode(IfsIndexDumpModeOff, IfsH262IndexerSettingDefault);
			break;
	}

	if (IfsStart(ifsHandle, strmInfo->videoPID, strmInfo->audioPID)
			!= IfsReturnCodeNoErrorReported)
    {
	    printf("Problems starting ifs writer\n");
	    //return IfsTrue;
    }
    i = 0;
    //printf("Indexing video file ...pktSize:%d\n", (int)pktSize);

    // Pretend each packet contains 1 second of data...
    while ((numPackets = fread(&ifsPacket, pktSize, 1, pInFile)) != 0)
    {
     	if (IfsWrite(ifsHandle, ++i * NSEC_PER_SEC, numPackets,
                &ifsPacket) != IfsReturnCodeNoErrorReported)
        {
            printf("Problems writing packet\n");
            fclose(pInFile);
            return IfsFalse;
        }
     }

    if(pInFile)
    {
    	fclose(pInFile);
        printf("Successfully indexed %s...\n", stream_filename);
    }

    return IfsTrue;
}


static IfsBoolean
get_streamInfo(char* inputFile, streamInfo *strmInfo)
{
    struct stream stream = { { 0 } };

    FILE * pInFile = NULL;
    IfsContainerType containerType = 0;
    IfsCodecType codecType = 0;

    if(!strmInfo || !inputFile)
    {
    	printf("Error: input pointers NULL\n");
    	return IfsFalse;
    }

   if(( pInFile = fopen(inputFile, "rb")) == NULL)
	   return IfsFalse;

    // get PIDs
    stream.firstPAT = TRUE;
    stream.firstPMT = TRUE;
    stream.patPID = 0;
    stream.pmtPID = 0x1FFF;
    stream.videoPID = 0x1FFF;
    stream.audioPID = 0x1FFF;
    stream.flags = 0;

    // start by assuming TS containers...
    containerType = IfsContainerTypeMpeg2Ts;

    if (NULL == (stream.buffer = (unsigned char*)g_malloc0(4*MAX_TS_PKT_SIZE)))
    {
        fclose(pInFile);
        fprintf(stderr, "could not allocate memory for stream\n");
        exit(-1);
    }
    // note: 4 packets are needed to discover the packet size.
    stream.bufLen = fread(stream.buffer, 1, 4 * MAX_TS_PKT_SIZE, pInFile);
    stream.offset = 0;
    discoverPktSize(&stream);

    if (0 == stream.tsPktSize)
    {
        printf("(is not a TS, checking PS)\n");
        containerType = IfsContainerTypeMpeg2Ps;
        codecType = IfsCodecTypeH262;
        stream.tsPktSize = IFS_TRANSPORT_PACKET_SIZE; 
    }
    else
    {

		unsigned pktCnt = 1;		// number of packets to parse in one pass
		unsigned long pktNum = 0;	// current packet number
		int patCnt = 0, pmtCnt = 0;
		IfsBoolean found_pat_pkt = IfsFalse, found_pmt_pkt = IfsFalse;
		printf("---------------------------------------------\n");
		printf("file name: %s\n", inputFile);
        printf("packet size: %d\n", stream.tsPktSize);
        rewind(pInFile);

        // read and toss the bytes preceeding the SYNC
        if (stream.offset > 0)
            fread(stream.buffer, 1, stream.offset, pInFile);

        do
        {
        	pktNum++;
           	stream.flags = 0;

          	stream.bufLen = fread(stream.buffer, 1, pktCnt * stream.tsPktSize, pInFile);
            decode_mp2ts(&stream, NULL, NULL);

            // ******* PAT/PMT packet identification stuff ***********

            if(stream.flags & (FLAGS_FOUND_PAT | FLAGS_FOUND_PAT_SEC))
            {
            	//printf("PAT in packet number: %ld\n", pktNum);
            	if(patCnt < MAX_PAT_PKT_COUNT)
            		stream.pat_pkt_nums[patCnt++] = pktNum;
            	if(stream.flags & FLAGS_FOUND_PAT)
            		found_pat_pkt = IfsTrue;
            }

            if(stream.flags & (FLAGS_FOUND_PMT | FLAGS_FOUND_PMT_SEC))
            {
               	//printf("PMT in packet number: %ld\n", pktNum);
            	if(pmtCnt < MAX_PMT_PKT_COUNT)
            		stream.pmt_pkt_nums[pmtCnt++] = pktNum;
            	if(stream.flags & FLAGS_FOUND_PMT)
            		found_pmt_pkt = IfsTrue;
            }
            if((pmtCnt >= MAX_PMT_PKT_COUNT) ||
               ((found_pat_pkt == IfsTrue ) && (found_pmt_pkt == IfsTrue)))
            {
            	for(patCnt=0; patCnt < MAX_PAT_PKT_COUNT; patCnt++)
            		if(stream.pat_pkt_nums[patCnt] > 0)
            			printf("PAT Packet[%d]: %d\n", patCnt, stream.pat_pkt_nums[patCnt]);
            	for(pmtCnt=0; pmtCnt < MAX_PMT_PKT_COUNT; pmtCnt++)
               		if(stream.pmt_pkt_nums[pmtCnt] > 0)
               			printf("PMT Packet[%d]: %d\n", pmtCnt, stream.pmt_pkt_nums[pmtCnt]);
            	break;
            }
         } while (!feof(pInFile));

        codecType = stream.codecType;
    }

    free(stream.buffer);
    fclose(pInFile);
    printf("\n%s [ %s ]\n\n", containerName[containerType], codecName[codecType]);
    if(!strmInfo)
    {
    	printf("Stream info structure pointer is null\n");
    	return IfsFalse;
    }
    else
    {
    	int i = 0;

    	strmInfo->program = stream.program;
    	strmInfo->tsPktSize = stream.tsPktSize;
    	strmInfo->videoPID = stream.videoPID;
    	strmInfo->audioPID = stream.audioPID;
    	strmInfo->patPID = stream.patPID;
    	strmInfo->pmtPID = stream.pmtPID;
    	strmInfo->containerType = containerType;
    	strmInfo->codecType = codecType;
    	strmInfo->stream_filename = inputFile;
    	for(i=0; i<MAX_PAT_PKT_COUNT; i++)
    		strmInfo->pat_pkt_nums[i] = stream.pat_pkt_nums[i];
    	for(i=0; i<MAX_PMT_PKT_COUNT; i++)
    		strmInfo->pmt_pkt_nums[i] = stream.pmt_pkt_nums[i];
    }
    return IfsTrue;
}


static void show_usage()
{
	printf("Usage1:  indexVideo <input>\n");
	printf("         Generates the index file\n");
	printf("         Where: <input> is one of the following three cases:\n");
	printf("          		1) an MPEG2TS file such as plain.mpg\n");
	printf("          		2) an MPEG2PS file such as departing_earth.ps\n");
	printf("          		3) an MPEG4 file such as mp4_video_in_mp2ts.ts\n");
	printf("Usage2:  indexVideo <index_filename> <format>\n");
	printf("		 Dumps the binary index file entries in text format\n");
	printf("         Where: <index_filename> is file name of the index file to be dumped in text format\n");
	printf("			    <format> is '2' for mpeg-2, '4' for h.264 file format of the index file\n");
	printf("Usage3:  indexVideo <stream file name> <index_filename> <trick_speed>\n");
	printf("		 Generates the trick file using the index file name, stream file name, and trick speed\n");
	printf("         Where: <index_filename> is file name of the index file to be used for generating the trick file\n");
	printf("			   <stream file name> file name of the associated stream file used with the index file\n");
	printf("		       <trick_speed> is the number '2 to 200' used for FF speed of the trick file\n");
	printf("		       <trick_speed> is the number '-2 to -200' used for FR speed of the trick file\n");
	printf("		       <trick_speed> is the number '1' used for generating 1x speed index text file for the normal stream file\n");
}

int
main(int argc, char *argv[])
{
    //const time_t now = time(NULL);
    IfsHandle handle = { 0 };
    streamInfo strmInfo = {0, 0, 0, 0, 0, 0, 0, 0, NULL};
    int retVal = 0;

   // printf("\n%s, version %d, at %ld %s\n", argv[0], INTF_RELEASE_VERSION, now,
   //         asctime(gmtime(&now)));

    if ((argc == 2) &&
    		(strstr(argv[1], ".ts") || strstr(argv[1], ".mpg")))
    {
    	printf("Generate index file ..\n");
    	if(get_streamInfo(argv[1], &strmInfo) == IfsTrue)
    		if(generate_IndexFile(argv[1], NULL, handle, &strmInfo) == IfsTrue)
    			retVal = 1;
    }
    else if((argc == 3) &&	// index with specified file name for .ndx file
    		(strstr(argv[1], ".ts") || strstr(argv[1], ".mpg")))
    {
    	printf("Generate index file: %s\n", argv[2]);
    	if(get_streamInfo(argv[1], &strmInfo) == IfsTrue)
    		if(generate_IndexFile(argv[1], argv[2], handle, &strmInfo) == IfsTrue)
    			retVal = 1;
    }
    else if((argc == 3) &&
    		(strstr(argv[1], ".ndx")))
    {
		printf("Dump index file...\n");
		if(dumpIndexFile(argv[1], atoi(argv[2])) == IfsTrue)
			retVal = 1;
    }
    else if((argc >= 4) &&
    		(strstr(argv[1], ".ts") || strstr(argv[1], ".mpg"))	 &&
    		(strstr(argv[2], ".ndx")))
    {
    	printf("Generate trick play file ...\n");
    	if(get_streamInfo(argv[1], &strmInfo) == IfsTrue)
    	{
    		char *des_dir = NULL;
    		strmInfo.stream_filename = argv[1];
    		if(argc > 4)
    			des_dir = argv[4];
     		if(generate_trickfile(argv[2], &strmInfo, atoi(argv[3]), des_dir) == IfsTrue)
    			retVal = 1;
    	}
    }
    else
    {
    	show_usage();
    }
    IfsClose(handle);
   	printf("Index Generation %s\n", (retVal == 0) ? "failed" : "successful");
   	return (retVal);
}



