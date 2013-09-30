hardware_platform = i386-linux-gnu

AM_CFLAGS = $(GLIB_CFLAGS) 

lib_LTLIBRARIES = libremap.la

libremap_ladir = $(includedir)/remap

libremap_la_CPPFLAGS = \
	-I../ifs \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS) 

libremap_la_SOURCES =	\
		RemapImpl.c \
		RemapImpl.h \
		RemapIntf.h \
		../ifs/my_ri_log.h

libremap_la_LIBADD = \
                $(GLIB_LIBS)   


