AM_CFLAGS = -g -O0 \
            $(GLIB_CFLAGS) 
AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4

hardware_platform = i386-linux-gnu

lib_LTLIBRARIES = libifs.la
bin_PROGRAMS = NdxDump indexVideo 

libifs_ladir = $(includedir)/ifs

libifs_la_CPPFLAGS = \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS) 

libifs_la_SOURCES =	\
	ifs_mpeg2ts_parse.c \
	ifs_mpeg2ps_parse.c \
	ifs_mpeg4_parse.c \
	ifs_h262_parse.c \
	ifs_h264_parse.c \
	ifs_h265_parse.c \
	ifs_file.c \
	ifs_operations.c \
	ifs_utils.c \
	ifs_impl.c \
	ifs_mpeg2ts_parse.h \
	ifs_mpeg2ps_parse.h \
	ifs_mpeg4_parse.h \
	ifs_h262_parse.h \
	ifs_h262_impl.h \
	ifs_h264_parse.h \
	ifs_h264_impl.h \
	ifs_h265_parse.h \
	ifs_h265_impl.h \
	ifs_file.h \
	ifs_operations.h \
	ifs_utils.h \
	ifs_impl.h \
	IfsIntf.h \
	ifs_streamdefs.h 

libifs_la_LIBADD = \
                $(GLIB_LIBS)

NdxDump_CPPFLAGS = \
	-I../remap \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS)

NdxDump_LDADD = \
	../remap/libremap.la

NdxDump_SOURCES =	\
	getMpegTsPsi.c \
	ifs_mpeg2ts_parse.c \
	ifs_mpeg2ps_parse.c \
	ifs_mpeg4_parse.c \
	ifs_h262_parse.c \
	ifs_h264_parse.c \
	ifs_h265_parse.c \
	ifs_impl.c \
	ifs_file.c \
	ifs_operations.c \
	ifs_utils.c \
	NdxDump.c \
	getMpegTsPsi.h \
	ifs_mpeg2ts_parse.h \
	ifs_mpeg2ps_parse.h \
	ifs_mpeg4_parse.h \
	ifs_h262_parse.h \
	ifs_h262_impl.h \
	ifs_h264_parse.h \
	ifs_h264_impl.h \
	ifs_h265_parse.h \
	ifs_h265_impl.h \
	ifs_file.h \
	ifs_operations.h \
	ifs_utils.h \
	ifs_impl.h \
	IfsIntf.h \
	ifs_streamdefs.h 

indexVideo_LDADD = \
	../ifs/libifs.la

indexVideo_CPPFLAGS = \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS)

indexVideo_SOURCES = 	\
	ifs_GenTrickPlay.c \
        getMpegTsPsi.c \
	indexVideo.c \
	getMpegTsPsi.h \
	ifs_GenTrickPlay.h 

