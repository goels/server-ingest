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

#ifndef MAX_PATH
#define MAX_PATH 260
#endif


static char* containerName[] = {"ERROR","MPEGPS","MPEGTS","MPEG4"};
static char* codecName[] = {"ERROR","H.261","H.262","H.263","H.264","H.265"};

static void
Index(char* inputFile, IfsHandle ifsHandle)
{
    struct stream stream = { { 0 } };
    FILE * pInFile = NULL;
    IfsPacket ifsPacket = { { 0 } };
    IfsContainerType containerType = 0;
    IfsCodecType codecType = 0;
    NumPackets numPackets = 0;
    NumBytes pktSize = IFS_TRANSPORT_PACKET_SIZE;// TODO: support multiple sizes
    int i = 0;

    printf("%s opening %s", __func__, inputFile);
    pInFile = fopen(inputFile, "rb");

    // get PIDs
    stream.firstPAT = TRUE;
    stream.firstPMT = TRUE;
    stream.patPID = 0;
    stream.pmtPID = 0x1FFF;
    stream.videoPID = 0x1FFF;
    stream.audioPID = 0x1FFF;

    // start by assuming TS containers...
    containerType = IfsContainerTypeMpeg2Ts;

    if (NULL == (stream.buffer = (unsigned char*)g_malloc0(4*MAX_TS_PKT_SIZE)))
    {
        fclose(pInFile);
        fprintf(stderr, "could not allocate memory for stream\n");
        exit(-1);
    }

    stream.bufLen = fread(stream.buffer, 1, 4 * MAX_TS_PKT_SIZE, pInFile);
    stream.offset = 0;
    discoverPktSize(&stream);

    if (0 == stream.tsPktSize)
    {
        printf("(is not a TS, checking PS)\n");
        containerType = IfsContainerTypeMpeg2Ps;
        codecType = IfsCodecTypeH262;
    }
    else
    {
        printf("  packet size:%d, offset:%d\n", stream.tsPktSize, stream.offset);
        rewind(pInFile);

        // read and toss the bytes preceeding the SYNC
        if (stream.offset > 0)
            fread(stream.buffer, 1, stream.offset, pInFile);

        while (!feof(pInFile))
        {
            stream.bufLen = fread(stream.buffer, 1, 4*stream.tsPktSize, pInFile);
            decode_mp2ts(&stream, NULL, NULL);
        }

        codecType = stream.codecType;
    }

    free(stream.buffer);
    rewind(pInFile);
    printf("\n%s [ %s ]\n\n", containerName[containerType], codecName[codecType]);

    if (IfsOpenWriter(".", NULL, 0, &ifsHandle) != IfsReturnCodeNoErrorReported)
    {
        printf("Problems opening ifs writer\n");
    }
    else if (IfsSetContainer(ifsHandle, containerType, pktSize)
                          != IfsReturnCodeNoErrorReported)
    {
        printf("Problems setting ifs container\n");
    }
    else if (IfsSetCodec(ifsHandle, codecType) != IfsReturnCodeNoErrorReported)
    {
        printf("Problems setting ifs codec\n");
        return;
    }
    else if (IfsStart(ifsHandle, stream.videoPID, stream.audioPID)
                          != IfsReturnCodeNoErrorReported)
    {
        printf("Problems starting ifs writer\n");
        return;
    }

    IfsSetMode(IfsIndexDumpModeOff, IfsIndexerSettingDefault);

    // Pretend each packet contains 1 second of data...
    while ((numPackets = fread(&ifsPacket, pktSize, 1, pInFile)) != 0)
    {
        if (IfsWrite(ifsHandle, ++i * NSEC_PER_SEC, numPackets,
                &ifsPacket) != IfsReturnCodeNoErrorReported)
        {
            printf("Problems writing packet\n");
            fclose(pInFile);
            return;
        }
    }

    fclose(pInFile);
    printf("Successfully indexed %s...\n", inputFile);
}

int
main(int argc, char *argv[])
{
    const time_t now = time(NULL);
    IfsHandle handle = { 0 };

    printf("\n%s, version %d, at %ld %s\n", argv[0], INTF_RELEASE_VERSION, now,
            asctime(gmtime(&now)));

    if (argc == 2)
    {
        Index(argv[1], handle);
    }
    else
    {
        printf("Usage:  indexVideo <input>\n");
        printf("   Where <input> is one of the following two cases:\n");
        printf("      1) an MPEG2TS with H.261, H.262, H.263, or H.264 video\n");
        printf("      2) an MPEG2PS file\n");
    }

    return IfsClose(handle);
}

