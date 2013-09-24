AM_CFLAGS = -g -O0
AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4

hardware_platform = i386-linux-gnu

lib_LTLIBRARIES = libifs.la
bin_PROGRAMS = NdxDump IfsSim indexVideo 

libifs_ladir = $(includedir)/ifs

libifs_la_CPPFLAGS = \
	-I$(CVP2_ROOT)/include \
	-I$(CVP2_ROOT)/include/glib-2.0 \
        -I$(CVP2_ROOT)/lib/glib-2.0/include \
	-I$(CVP2_ROOT)/lib/$(hardware_platform)/glib-2.0/include \
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
	-lglib-2.0 

NdxDump_CPPFLAGS = \
	-I../remap \
	-I$(CVP2_ROOT)/include \
	-I$(CVP2_ROOT)/include/glib-2.0 \
        -I$(CVP2_ROOT)/lib/glib-2.0/include \
	-I$(CVP2_ROOT)/lib/$(hardware_platform)/glib-2.0/include \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS)

NdxDump_LDADD = \
	-lglib-2.0 \
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

IfsSim_LDADD = \
	-lglib-2.0 \
	../remap/libremap.la

IfsSim_CPPFLAGS = \
	-I../remap \
	-I$(CVP2_ROOT)/include \
	-I$(CVP2_ROOT)/include/glib-2.0 \
        -I$(CVP2_ROOT)/lib/glib-2.0/include \
	-I$(CVP2_ROOT)/lib/$(hardware_platform)/glib-2.0/include \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS)

IfsSim_SOURCES = \
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
	IfsSim.c \
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
	ifs_streamdefs.h 

indexVideo_LDADD = \
	-lglib-2.0 \
	../ifs/libifs.la

indexVideo_CPPFLAGS = \
	-I$(CVP2_ROOT)/include \
	-I$(CVP2_ROOT)/include/glib-2.0 \
        -I$(CVP2_ROOT)/lib/glib-2.0/include \
	-I$(CVP2_ROOT)/lib/$(hardware_platform)/glib-2.0/include \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS)

indexVideo_SOURCES = 	\
	ifs_GenTrickPlay.c \
        getMpegTsPsi.c \
	indexVideo.c \
	getMpegTsPsi.h \
	ifs_GenTrickPlay.h 


