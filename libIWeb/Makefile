# libIweb is a web server/client library originally based
# off Intel's UPnP MicroStack/1.0.1298
#
# (c) 2012 Brad Love
# Next Dimension Innovations
# http://nextdimension.cc
# http://b-rad.cc
#

# List all object files here
OFILES = \
	ILibParsers.o\
	ILibAsyncSocket.o\
	ILibAsyncServerSocket.o\
	ILibWebClient.o\
	ILibWebServer.o

# Compiler command name
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

# Compiler flags applied to all files
CFLAGS ?= -Os -Wall -D_POSIX -D_DEBUG -D_VERBOSE -pthread 
 
LIBNAME = libIWeb

all: libIWeb

$(LIBNAME): $(OFILES)
	$(AR) q $(LIBNAME).a $(OFILES)

# Macro rule for all object files.
$(OFILES) :

# Clean up
clean :
	rm -f $(OFILES) $(LIBNAME).a
