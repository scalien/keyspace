##############################################################################
#
# Keyspace Makefile
#
##############################################################################


##############################################################################
#
# Directories
#
##############################################################################

BASE_DIR = .
KEYSPACE_DIR = .
BIN_DIR = $(BASE_DIR)/bin

SCRIPT_DIR = $(BASE_DIR)/scripts
DIST_DIR = $(BASE_DIR)/dist
VERSION = `$(BASE_DIR)/script/version.sh $(SRC_DIR)/Version.h`
PACKAGE_DIR = $(BASE_DIR)/packages
PACKAGE_FILE = keyspace-$(VERSION).deb
PACKAGE_REPOSITORY = /

BUILD_ROOT = $(BASE_DIR)/build
SRC_DIR = $(BASE_DIR)/src
DEB_DIR = $(BUILD_ROOT)/deb

BUILD_DEBUG_DIR = $(BUILD_ROOT)/debug
BUILD_RELEASE_DIR = $(BUILD_ROOT)/release


##############################################################################
#
# Platform selector
#
##############################################################################

ifeq ($(shell uname),Darwin)
PLATFORM=Darwin
else
PLATFORM=Linux
endif
export PLATFORM
PLATFORM_UCASE=$(shell echo $(PLATFORM) | tr [a-z] [A-Z])

include Makefile.$(PLATFORM)


##############################################################################
#
# Compiler/linker options
#
##############################################################################

CC = gcc
CXX = g++

DEBUG_CFLAGS = -g #-pg
DEBUG_LDFLAGS = #-pg -lc_p

RELEASE_CFLAGS = -O3 #-g

ifneq ($(BUILD), release)
BUILD_DIR = $(BUILD_DEBUG_DIR)
CFLAGS = $(BASE_CFLAGS) $(DEBUG_CFLAGS)
CXXFLAGS = $(BASE_CXXFLAGS) $(DEBUG_CFLAGS)
LDFLAGS = $(BASE_LDFLAGS) $(DEBUG_LDFLAGS)
else
BUILD_DIR = $(BUILD_RELEASE_DIR)
CFLAGS = $(BASE_CFLAGS) $(RELEASE_CFLAGS)
CXXFLAGS = $(BASE_CXXFLAGS) $(RELEASE_CFLAGS)
LDFLAGS = $(BASE_LDFLAGS) $(RELEASE_LDFLAGS)
endif


##############################################################################
#
# Targets
#
##############################################################################

all: release

debug:
	$(MAKE) targets BUILD="debug"

release:
	$(MAKE) targets BUILD="release"

include Makefile.dirs
	
targets: $(BUILD_DIR) $(BIN_DIR)/keyspace


##############################################################################
#
# Build rules
#
##############################################################################

include Makefile.objects

KEYSPACE_LIBS =
	
SYSTEM_OBJECTS = \
	$(BUILD_DIR)/System/Events/Scheduler.o \
	$(BUILD_DIR)/System/IO/Endpoint.o \
	$(BUILD_DIR)/System/IO/IOProcessor_$(PLATFORM).o \
	$(BUILD_DIR)/System/IO/Socket.o \
	$(BUILD_DIR)/System/Log.o \
	$(BUILD_DIR)/System/Common.o \

FRAMEWORK_OBJECTS = \
	$(BUILD_DIR)/Framework/Transport/TransportUDPReader.o \

OBJECTS = \
	$(ALL_OBJECTS) \
	$(BUILD_DIR)/Main.o

LIBS = \
	$(KEYSPACE_LIBS)

$(KEYSPACE_LIBS):
	cd $(KEYSPACE_DIR); $(MAKE) targets BUILD=$(BUILD)
	
$(BIN_DIR)/keyspace: $(BUILD_DIR) $(LIBS) $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<


##############################################################################
#
# Clean
#
##############################################################################

clean: clean-debug clean-release
	-rm -r -f $(BUILD_ROOT)

clean-debug:
	-rm -f $(BASE_DIR)/keyspace
	-rm -r -f $(BUILD_DEBUG_DIR)
	
clean-release:
	-rm -f $(BASE_DIR)/keyspace
	-rm -r -f $(BUILD_RELEASE_DIR)
	
clean-libs: clean-keyspace

clean-keyspace:
	cd $(KEYSPACE_DIR); $(MAKE) clean

distclean: clean distclean-libs

distclean-libs: distclean-keyspace

distclean-keyspace:
	cd $(KEYSPACE_DIR); $(MAKE) distclean


##############################################################################
#
# Build packages
#
##############################################################################

deb: release
	-mkdir -p $(DEB_DIR)/etc/init.d
	-mkdir -p $(DEB_DIR)/usr/sbin
	-cp -fr $(PACKAGE_DIR)/DEBIAN $(DEB_DIR)
	-cp -fr $(SCRIPT_DIR)/keyspace.cfg $(DEB_DIR)/etc
	-cp -fr $(SCRIPT_DIR)/keyspace $(DEB_DIR)/etc/init.d
	-cp -fr $(SCRIPT_DIR)/safe_keyspace $(DEB_DIR)/usr/sbin
	-cp -fr $(BASE_DIR)/keyspace $(DEB_DIR)/usr/sbin
	-rm -f $(BUILD_ROOT)/.*
	-mkdir -p $(DIST_DIR)
	-rm -f $(DIST_DIR)/$(PACKAGE_FILE)
	-cd $(DIST_DIR)
	-fakeroot dpkg -b $(DEB_DIR) $(DIST_DIR)/$(PACKAGE_FILE)

deb-install: deb
	-sudo reprepro -Vb $(PACKAGE_REPOSITORY) includedeb etch $(DIST_DIR)/$(PACKAGE_FILE)
