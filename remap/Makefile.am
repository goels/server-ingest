hardware_platform = i386-linux-gnu

lib_LTLIBRARIES = libremap.la

libremap_ladir = $(includedir)/remap

libremap_la_CPPFLAGS = \
	-I../ifs \
	-I/usr/include \
	-I/usr/include/glib-2.0 \
	-I/usr/lib/$(hardware_platform)/glib-2.0/include \
	-I./ \
	-DSTAND_ALONE \
	$(CPPFLAGS) 

libremap_la_SOURCES =	\
		RemapImpl.c \
		RemapImpl.h \
		RemapIntf.h \
		../ifs/my_ri_log.h

libremap_la_LIBADD = \
	-lglib-2.0 
