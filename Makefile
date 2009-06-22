##############################################################################
#
# Keyspace Makefile
#
##############################################################################

all: release

##############################################################################
#
# Directories
#
##############################################################################

BASE_DIR = .
KEYSPACE_DIR = .
BIN_DIR = $(BASE_DIR)/bin

SCRIPT_DIR = $(BASE_DIR)/script
DIST_DIR = $(BASE_DIR)/dist
VERSION = `$(BASE_DIR)/script/version.sh 3 $(SRC_DIR)/Version.h`
VERSION_MAJOR = `$(BASE_DIR)/script/version.sh 1 $(SRC_DIR)/Version.h`
VERSION_MAJMIN = `$(BASE_DIR)/script/version.sh 3 $(SRC_DIR)/Version.h`
PACKAGE_NAME = keyspace
PACKAGE_DIR = $(BASE_DIR)/packages
PACKAGE_FILE = keyspace-$(VERSION).deb
PACKAGE_REPOSITORY = /

BUILD_ROOT = $(BASE_DIR)/build
SRC_DIR = $(BASE_DIR)/src
DEB_DIR = $(BUILD_ROOT)/deb

BUILD_DEBUG_DIR = $(BUILD_ROOT)/debug
BUILD_RELEASE_DIR = $(BUILD_ROOT)/release

INSTALL_BIN_DIR = /usr/local/bin
INSTALL_LIB_DIR = /usr/local/lib
INSTALL_INCLUDE_DIR = /usr/local/include/


##############################################################################
#
# Platform selector
#
##############################################################################
ifeq ($(shell uname),Darwin)
PLATFORM=Darwin
else
ifeq ($(shell uname),FreeBSD)
PLATFORM=Darwin
else
ifeq ($(shell uname),OpenBSD)
PLATFORM=Darwin 
else
PLATFORM=Linux
endif
endif
endif

ARCH=$(shell arch)
export PLATFORM
PLATFORM_UCASE=$(shell echo $(PLATFORM) | tr [a-z] [A-Z])

export PLATFORM_UCASE
include Makefile.$(PLATFORM)


##############################################################################
#
# Compiler/linker options
#
##############################################################################

CC = gcc
CXX = g++
RANLIB = ranlib

DEBUG_CFLAGS = -g #-pg
DEBUG_LDFLAGS = #-pg -lc_p

RELEASE_CFLAGS = -O2 #-g

LIBNAME = libkeyspaceclient
SONAME = $(LIBNAME).$(SOEXT).$(VERSION_MAJOR)
SOLIB = $(LIBNAME).$(SOEXT)
ALIB = $(LIBNAME).a

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
# Build rules
#
##############################################################################

include Makefile.dirs
include Makefile.objects
include Makefile.clientlib

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

TEST_OBJECTS = \
	$(BUILD_DIR)/Test/KeyspaceClientTest.o

CLIENTLIBS = \
	$(BIN_DIR)/$(ALIB) \
	$(BIN_DIR)/$(SOLIB)

EXECUTABLES = \
	$(BIN_DIR)/keyspace \
	$(BIN_DIR)/clienttest \
	$(BIN_DIR)/timechecktest
	
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(BIN_DIR)/%.so: $(BIN_DIR)/%.a
	$(CC) $(SOLINK) -o $@ $< $(SOLIBS)


# libraries
$(BIN_DIR)/$(ALIB): $(BUILD_DIR) $(CLIENTLIB_OBJECTS)
	$(AR) -r $@ $(CLIENTLIB_OBJECTS)
	$(RANLIB) $@

$(BIN_DIR)/$(SOLIB): $(BUILD_DIR) $(CLIENTLIB_OBJECTS)
	$(CC) $(SOLINK) -o $@ $(CLIENTLIB_OBJECTS) $(LDFLAGS)

# executables	
$(BIN_DIR)/keyspace: $(BUILD_DIR) $(LIBS) $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(BIN_DIR)/clienttest: $(BUILD_DIR) $(TEST_OBJECTS) $(BIN_DIR)/$(ALIB)
	$(CXX) $(LDFLAGS) -o $@ $(TEST_OBJECTS) $(LIBS) $(BIN_DIR)/$(ALIB)

$(BIN_DIR)/timechecktest: $(BUILD_DIR) $(LIBS) $(ALL_OBJECTS) $(BUILD_DIR)/Test/TimeCheckTest.o
	$(CXX) $(LDFLAGS) -o $@ $(ALL_OBJECTS) $(LIBS) $(BUILD_DIR)/Test/TimeCheckTest.o


##############################################################################
#
# Targets
#
##############################################################################

debug:
	$(MAKE) targets BUILD="debug"

release:
	$(MAKE) targets BUILD="release"

clienttest:
	$(MAKE) test_targets BUILD="debug"

clientlib:
	$(MAKE) clientlib_targets BUILD="debug"

timechecktest:
	$(MAKE) timechecktest_targets BUILD="release"
	
targets: $(BUILD_DIR) executables clientlibs

clientlibs: $(BUILD_DIR) $(CLIENTLIBS)

executables: $(BUILD_DIR) $(EXECUTABLES)

install: release
	-cp -fr $(BIN_DIR)/$(ALIB) $(INSTALL_LIB_DIR)
	-cp -fr $(BIN_DIR)/$(SOLIB) $(INSTALL_LIB_DIR)/$(SOLIB).$(VERSION)
	-ln -sf $(INSTALL_LIB_DIR)/$(SOLIB).$(VERSION) $(INSTALL_LIB_DIR)/$(LIBNAME).$(SOEXT).$(VERSION_MAJOR)
	-ln -sf $(INSTALL_LIB_DIR)/$(SOLIB).$(VERSION) $(INSTALL_LIB_DIR)/$(LIBNAME).$(SOEXT)
	-cp -fr $(SRC_DIR)/Application/Keyspace/Client/keyspace_client.h $(INSTALL_INCLUDE_DIR)
	-cp -fr $(BIN_DIR)/keyspace $(INSTALL_BIN_DIR)
	-cp -fr $(SCRIPT_DIR)/safe_keyspace $(INSTALL_BIN_DIR)

uninstall:
	-rm $(INSTALL_LIB_DIR)/$(ALIB)
	-rm $(INSTALL_LIB_DIR)/$(SOLIB).$(VERSION)
	-rm $(INSTALL_LIB_DIR)/$(SOLIB)
	-rm $(INSTALL_INCLUDE_DIR)/Application/Keyspace/Client/keyspace_client.h
	-rm $(INSTALL_BIN_DIR)/keyspace
	-rm $(INSTALL_BIN_DIR)/safe_keyspace

##############################################################################
#
# Clean
#
##############################################################################

clean: clean-debug clean-release clean-libs clean-executables
	-rm -rf $(BUILD_ROOT)
	-rm -rf $(DEB_DIR)

clean-debug:
	-rm -f $(BASE_DIR)/keyspace
	-rm -r -f $(BUILD_DEBUG_DIR)
	
clean-release:
	-rm -f $(BASE_DIR)/keyspace
	-rm -r -f $(BUILD_RELEASE_DIR)
	
clean-libs:
	-rm $(CLIENTLIBS)

clean-executables:
	-rm $(EXECUTABLES)

clean-clientlib:
	-rm $(TEST_OBJECTS)
	-rm $(BIN_DIR)/clienttest
	-rm $(BIN_DIR)/clientlib.a

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
	-$(SCRIPT_DIR)/mkcontrol.sh $(SCRIPT_DIR)/DEBIAN/control $(PACKAGE_NAME) $(VERSION) $(shell dpkg-architecture -qDEB_BUILD_ARCH)
	-mkdir -p $(DEB_DIR)/etc/init.d
	-mkdir -p $(DEB_DIR)/etc/default
	-mkdir -p $(DEB_DIR)/usr/bin
	-cp -fr $(SCRIPT_DIR)/DEBIAN $(DEB_DIR)
	-cp -fr $(SCRIPT_DIR)/keyspace.conf $(DEB_DIR)/etc
	-cp -fr $(SCRIPT_DIR)/keyspace $(DEB_DIR)/etc/init.d
	-cp -fr $(SCRIPT_DIR)/default $(DEB_DIR)/etc/default/keyspace
	-cp -fr $(SCRIPT_DIR)/safe_keyspace $(DEB_DIR)/usr/bin
	-cp -fr $(BIN_DIR)/keyspace $(DEB_DIR)/usr/bin
	-rm -f $(BUILD_ROOT)/.*
	-mkdir -p $(DIST_DIR)
	-rm -f $(DIST_DIR)/$(PACKAGE_FILE)
	-cd $(DIST_DIR)
	-fakeroot dpkg -b $(DEB_DIR) $(DIST_DIR)/$(PACKAGE_FILE)

deb-install: deb
	-sudo reprepro -Vb $(PACKAGE_REPOSITORY) includedeb etch $(DIST_DIR)/$(PACKAGE_FILE)
