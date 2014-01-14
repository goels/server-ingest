
The 'server-ingest' indexing tool is used to generate trick streams and associated index files for random access and server side play speed support. It is a stand alone tool which is currently used by the CableLabs CVP2 rygel project for server side content preparation. The tool supports indexing of content in MPEG TS container format with H262/H264/AVC encoding (188 as well as 192 byte packets with and without time stamps), and MPEG PS container format (H262 encoding). Indexing of MP4 container format is not supported.
The ‘indexVideo’ binary is built as part of ‘server-ingest’ set of tools.  In addition to indexVideo, other tools built are ‘NdxDump’ and ‘IfsSim’. This project is hosted on Github (cablelabs/server-ingest).

This section briefly describes ‘indexVideo’ which is used to generate trick stream files and associated index files for various DLNA media formats. Alternately, the script 'itemGen.sh' can be run to automate the process of index and trick file generation. The tool can be used with any suitable content conforming to DLNA prescribed media formats.
Getting the source and building

The indexing tool source should be placed under CVP2 GIT (~/cvp2/git) directory.  Make sure to source the 'env_setup' script present in ~/cvp2 directory. This will set all the environment variables such as CVP2_GIT ((e.g: /home/user/cvp2/git), CVP2_ROOT (e.g: /home/user/cvp2/root) etc.

> cd $CVP2_GIT 

Clone the ‘server-ingest’ git repo.

> git clone git@github.com:cablelabs/server-ingest.git

'server-ingest' directory should be created

> cd ‘server-ingest’

Run the ‘./autogen.sh’ script with the correct prefix, followed by 'make' and 'make install'

>./autogen.sh --prefix=$CVP2_ROOT

> make

> make install

This will copy the binaries to '$CVP2_ROOT/bin' directory
Generating trick streams and index files
Using itemGen script

The script itemGen.sh is located in ‘server-ingest’ directory. It creates trick streams, index files and associated resource info files for use by rygel media engine in the format that the media engine requires.

    ./itemGen.sh

Usage: ./itemGen.sh src_dir dest_dir [-p ./profiles.txt] <speed> <speed> <speed>

where:

src_dir  is the directory path where stream files are located

dest_dir is the directory path where media directories/files will be generated

option [-p ./profiles.txt]

if the option -p is specified, then './profiles.txt' must be provided
'./profile.txt' is the text file containing the valid profiles and mime types information.  It is located in 'server-ingest' directory.              

<speed> can be multiple speeds in range <-100 to -1> or <1 to 100>

Note: if -p option is not specified then only the media files located in src_dir will be processed.
if -p option is specified then only the media files contained in sub-directories with valid profile name
as sub-directory name will be processed. Valid profile names are contained in the 'profile.txt' file.

example (using profile.txt)
           
Media file 'B-MP2PS_N-70.mpg' is located in 'streams/MPEG_PS_NTSC'
(sub-directory name should be a valid profile listed in profile.txt)                                                                 

        > ./itemGen.sh ./streams ./odid -p ./profile.txt 1 2 4 -2 -4

It results in the following trick stream and index file generation.

odid/media/MPEG_PS_NTSC_B-MP2PS_N-70
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC_B-MP2PS_N-70.item
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.-2_1.mpg
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.-2_1.mpg.index
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.1_1.mpg
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.-2_1.mpg.info
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.-4_1.mpg
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.-4_1.mpg.index
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/resource.info
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.1_1.mpg.index
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.2_1.mpg
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.-4_1.mpg.info
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.2_1.mpg.index
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.2_1.mpg.info
odid/media/MPEG_PS_NTSC_B-MP2PS_N-70/MPEG_PS_NTSC.0/B-MP2PS_N-70.1_1.mpg.info

example (without using profile.txt)

If ‘streams’ directory contains media files ‘B-MP2PS_N-70.mpg’ and 'O-MP2TS_SN_I-1.mpg'

When the following script is run

    ./itemGen.sh streams odid 1 2

It results in the following files being created. 

odid/media/item_B-MP2PS_N-70
odid/media/item_B-MP2PS_N-70/Resource.0
odid/media/item_B-MP2PS_N-70/Resource.0/B-MP2PS_N-70.1_1.mpg
odid/media/item_B-MP2PS_N-70/Resource.0/resource.info
odid/media/item_B-MP2PS_N-70/Resource.0/B-MP2PS_N-70.1_1.mpg.index
odid/media/item_B-MP2PS_N-70/Resource.0/B-MP2PS_N-70.2_1.mpg
odid/media/item_B-MP2PS_N-70/Resource.0/B-MP2PS_N-70.2_1.mpg.index
odid/media/item_B-MP2PS_N-70/Resource.0/B-MP2PS_N-70.2_1.mpg.info
odid/media/item_B-MP2PS_N-70/Resource.0/B-MP2PS_N-70.1_1.mpg.info
odid/media/item_B-MP2PS_N-70/item_B-MP2PS_N-70.item

odid/media/item_O-MP2TS_SN_I-1
odid/media/item_O-MP2TS_SN_I-1/Resource.0
odid/media/item_O-MP2TS_SN_I-1/Resource.0/O-MP2TS_SN_I-1.2_1.mpg.index
odid/media/item_O-MP2TS_SN_I-1/Resource.0/O-MP2TS_SN_I-1.2_1.mpg
odid/media/item_O-MP2TS_SN_I-1/Resource.0/resource.info
odid/media/item_O-MP2TS_SN_I-1/Resource.0/O-MP2TS_SN_I-1.1_1.mpg.index
odid/media/item_O-MP2TS_SN_I-1/Resource.0/O-MP2TS_SN_I-1.1_1.mpg
odid/media/item_O-MP2TS_SN_I-1/Resource.0/O-MP2TS_SN_I-1.1_1.mpg.info
odid/media/item_O-MP2TS_SN_I-1/Resource.0/O-MP2TS_SN_I-1.2_1.mpg.info
odid/media/item_O-MP2TS_SN_I-1/item_O-MP2TS_SN_I-1.item

The resulting destination 'odid/media' directory is in a format expected by the odid media engine. It is referred to in the rygel.conf file under the odid section (e.g: uris=/home/prasanna/cvp2/server-ingest/odid/media)
Using  ‘indexVideo’

Trick streams and index files can also be generated by running ‘indexVideo’ independently.

Usage1:  indexVideo <input>

                        Generates the index file

                        Where:

<input> is one of the following three cases:

                        1) an MPEG2TS file such as plain.mpg

                        2) an MPEG2PS file such as departing_earth.ps

                        3) an MPEG4 file such as mp4_video_in_mp2ts.ts

 

Usage2:  indexVideo <index_filename> <format>

Dumps the binary index file entries in text format

Where:

<index_filename> is file name of the index file to be dumped in text format

<format> is '2' for mpeg-2, '4' for h.264 file format of the index file

 

Usage3:  indexVideo <stream file name> <index_filename> <trick_speed>

Generates the trick file using the index file name, stream file name, and trick speed

Where:

<stream file name> is the file name of the associated stream file used with the index file

<index_filename>  is  the file name of the index file to be used for generating the trick file

<trick_speed> is the number '1 to 100'  used for generating specific speed Fast Forward trick file 

                      or the number '-2 to -100'  used for generating specific speed Fast Reverse trick file

                      the number '1' is used for generating 1x speed index text file only for the normal speed

Example:

To generate index files (1X, 2X, -2X speeds) for MPEG program stream file B-MP2PS_N-70.mpg do the following steps:

1.    indexVideo ~/Videos/B-MP2TS_SN-70.mpg

At this point the tool creates a unique directory xxxxxxxx_0000/

which includes a binary index file 00000000.ndx, which will be used in the subsequent steps to generate text index files and trick streams for consumption by rygel media engine.

2. Generate index file for normal speed (timeseek support)

indexVideo ~/Videos/B-MP2TS_SN-70.mpg xxxxxxxx_0000/0000000000.ndx 1

This should generate a B-MP2PS_N-70.1_1.mpg.index index file

3. Generate index file for 2X speed

indexVideo ~/Videos/B-MP2TS_SN-70.mpg xxxxxxxx_0000/0000000000.ndx 2

This should generate a 2X trick stream B-MP2TS_SN-70.2_1.mpg and associated index file B-MP2TS_SN-70.2_1.mpg.index

4. Generate index file for -2X speed

indexVideo ~/Videos/B-MP2TS_SN-1.mpg xxxxxxxx_0000/0000000000.ndx -2

This should generate a -2X trick stream B-MP2TS_SN-70.-2_1.mpg and associated index file B-MP2TS_SN-70.-2_1.mpg.index.

Move these trick files and index files to location referred to by the rygel configuration (rygel.conf) file.
