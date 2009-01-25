ifeq ($(shell uname),Darwin)
PLATFORM=Darwin
else
PLATFORM=Linux
endif
export PLATFORM

all:
	make -f Makefile.$(PLATFORM)

%:
	make -f Makefile.$(PLATFORM) $@
