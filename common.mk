
#
# Compile options
#
CFLAGS += -g -O0
CXXFLAGS = $(CFLAGS)
CPPFLAGS = -Wall -Werror
OBJ_SUFFIX = .o
SO_SUFFIX = .so
EXE_SUFFIX =
COPTS = $(CPPFLAGS)
LDOPTS =
LIBGEN := ar
LIBGEN_OPTS := -cr
SHARED_LIBGEN := $(CC)
SHARED_LIBGEN_OPTS := -shared
EXEGEN := $(CC)
EXEGEN_OPTS := -mwindow
BUILD := i686-pc-linux-gnu
HOST := i686-pc-linux-gnu
hardware_platform = i386-linux-gnu

CC     = gcc
CXX    = g++
CD     = cd 
CP     = cp
CPTREE = cp -r
CHMOD  = chmod
ECHO   = echo
MKDIR  = mkdir -p
PERL   = perl
RM     = rm -f
RMTREE = rm -rf
SED    = sed
TOUCH  = touch
XARGS  = xargs

#
# Compile C or CXX source file into an intermediate file
#
# $@ = The name of the object (.o) file to create
# $< = The name of the source (.c or .cpp) file (must be first prerequisite listed in rule)
# $1 = Additional compile options
#
define COMPILE
    @$(ECHO) Compiling $< into $@
    @$(MKDIR) $(dir $@)
    @$(RM) $@
    $(if $(filter .m,$(suffix $<)), gcc -c -ObjC $(CFLAGS) $1 $< -o $@,)
    $(if $(filter .c,$(suffix $<)), $(CC) -c $(CFLAGS) $1 $< -o $@,)
    $(if $(filter .cpp,$(suffix $<)), $(CXX) -c $(CXXFLAGS) $1 $< -o $@,)
endef

#
# Build a dependency file from a C or CXX source file
#
# $@ = The name of the dependency (.d) file to create
# $< = The name of the source (.c or .cpp) file (must be first prerequisite listed in rule)
# $1 = Additional compile options
#
define BUILD_DEPENDS
    @$(ECHO) Building dependency file for $<
    @$(MKDIR) $(dir $@)
    @$(RM) $@
    $(if $(filter .m,$(suffix $<)), $(CC) -M $1 $< > $@.tmp,)
    $(if $(filter .c,$(suffix $<)), $(CC) -M $(CFLAGS) $1 $< > $@.tmp,)
    $(if $(filter .cpp,$(suffix $<)), $(CXX) -M $(CXXFLAGS) $1 $< > $@.tmp,)
    $(SED) 's,.*\.o[ :]*,$(@:.d=.o) $@ : ,g' < $@.tmp > $@
    @$(RM) $@.tmp
endef

#
# Build a library from a list of object files
#
# $@ = The name of the library (.a) file to create
# $1 = The list of all object (.o) files to put into the library
#
define BUILD_LIBRARY
    @$(ECHO) Building library $@
    @$(MKDIR) $(dir $@)
    @$(RM) $@
    @$(MKDIR) $(dir $@)
    $(LIBGEN) $(LIBGEN_OPTS) $@ $1
endef

#
# Build a shared library from a list of object files
#
# $@ = The name of the library (.so) file to create
# $1 = The list of all object (.o) files to put into the library
#
define BUILD_SHARED_LIBRARY
    @$(ECHO) Building shared library $@
    @$(MKDIR) $(dir $@)
    @$(RM) $@
    @$(MKDIR) $(dir $@)
    $(SHARED_LIBGEN) $(SHARED_LIBGEN_OPTS) -o $@ $1
endef

#
# Build an executable from a list of object files
#
# $@ = The name of the executable file to create
# $1 = The list of all object (.o) files to put into the library
#
define BUILD_EXECUTABLE
    @$(ECHO) Building executable $@
    @$(MKDIR) $(dir $@)
    @$(RM) $@
    @$(MKDIR) $(dir $@)
    $(EXEGEN) $(EXEGEN_OPTS) -o $@ $1
endef

#
# Remove a library
#
# $1 = The name of the library (.so) to remove
#
define RM_LIBRARY
    @$(ECHO) Removing library $1
    @$(RM) $1
endef


