# TV-Now
#
# (c) 2012 - Brad Love
# Next Dimension Innovations
# http://nextdimension.cc
# http://b-rad.cc
#

# Name of the executable
EXENAME = tv-now

# List all object files here
OFILES = \
	CdsMediaClass.o\
	CdsMediaObject.o\
	CdsObjectToDidl.o\
	MicroMediaServer.o\
	UpnpMicroStack.o\
	MyString.o\
	MimeTypes.o\
	PortingFunctions.o\
	tv-now.o 
	
DVBTEE_LIBS = -L./libdvbtee/usr/lib -ldvbtee -ldvbtee_server -ldvbpsi -lstdc++
DVBTEE_LIBS_STATIC = ./libdvbtee/libdvbtee/libdvbtee.a ./libdvbtee/libdvbtee_server/libdvbtee_server.a ./libdvbtee/usr/lib/libdvbpsi.a -lstdc++
DVBTEE_INCLUDES = -I./libdvbtee/usr/include -I./libdvbtee/libdvbtee -I./libdvbtee/libdvbtee_server
DVBTEE_SERVER = dvbteeserver

IWEBLIB = libIWeb

DEBUG = -D_DEBUG -D_VERBOSE -g -ggdb
# Compiler flags applied to all files
# Optional flags: -D_VERBOSE -D_DEBUG -DSPAWN_BROWSE_THREAD -D_TEMPDEBUG
# -g puts debug symbols
# -DSPAWN_BROWSE_THREAD makes it so each browse thread spawns a thread (recommended only if browse request takes a long time)
#
export DEFINES  = -D_POSIX -D_GNU_SOURCE -D__USE_LARGEFILE64 -D__STDC_FORMAT_MACROS -D_FILE_OFFSET_BITS=64

export CPPFLAGS = -Wno-deprecated -Wno-deprecated-declarations -pthread -fPIC $(DEBUG) $(DEFINES)
export CFLAGS   = -O2 -Wall $(DEBUG) $(OPTFLAGS) -pthread -I./$(IWEBLIB) $(DEFINES)
LIBS = ./$(IWEBLIB)/$(IWEBLIB).a

# Compiler command name
CC     =  $(CROSS_COMPILE)gcc
GPP    =  $(CROSS_COMPILE)g++
CXX    =  $(GPP)
STRIP  =  $(CROSS_COMPILE)strip

# Search paths
VPATH = src: ./ ./$(IWEBLIB)


# Builds all object files and executable
$(EXENAME) : $(OFILES) $(IWEBLIB) $(DVBTEE_SERVER)
	$(CC) $(CFLAGS) -o $(EXENAME) $(OFILES) $(DVBTEE_SERVER).o ${LIBS} $(DVBTEE_LIBS)

# Static build
static : $(OFILES) $(IWEBLIB) $(DVBTEE_SERVER)
	$(CC) $(CFLAGS) -o $(EXENAME) $(OFILES) $(DVBTEE_SERVER).o $(LIBS) $(DVBTEE_LIBS_STATIC)

$(IWEBLIB) : $(OFILES)
	make -C $(IWEBLIB)

$(DVBTEE_SERVER) :
	$(GPP) $(DVBTEE_INCLUDES) $(CPPFLAGS) -c $(DVBTEE_SERVER).cpp

# Macro rule for all object files.
$(OFILES) : \
	ILibParsers.h\
	ILibWebClient.h\
	ILibWebServer.h\
	ILibAsyncSocket.h\
	ILibAsyncServerSocket.h\
	UpnpMicroStack.h\
	MicroMediaServer.h\
	MyString.h\
	MimeTypes.h\
	dvbteeserver.h\
	PortingFunctions.h

# Clean up
clean :
	make -C $(IWEBLIB) clean
	rm -f $(OFILES) $(EXENAME) $(DVBTEE_SERVER).o
