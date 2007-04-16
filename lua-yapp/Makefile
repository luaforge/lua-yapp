# ----- Makefile ------------------------------------------------
# Makefile for lua-yapp ("Yet Another PalmOS Port of Lua")

ifneq ($(DEBUG),no)
  DEBUG=yes
endif

AR = m68k-palmos-ar
RANLIB = m68k-palmos-ranlib
STRIP = m68k-palmos-strip
CC = m68k-palmos-gcc

EXTRA_CFLAGS = 
ifeq ($(DEFINTLONG),yes)
  EXTRA_CFLAGS += -Dint=long
endif
ifeq ($(DEBUG),yes)
  EXTRA_CFLAGS += -g
endif
ifeq (0,1)
  # this option unfortunately is not available with prc-tools
  EXTRA_CFLAGS += -msep-data
endif
ifeq (1,1)
  EXTRA_CFLAGS += -Wimplicit
  EXTRA_CFLAGS += -Wmissing-declarations
endif

TOPDIR = $(shell pwd)
EXTRA_CFLAGS += -include $(TOPDIR)/osglue/oswrapper.h
EXTRA_CFLAGS += -I$(TOPDIR)/osglue

SDKDIR = /opt/palmdev/sdk-5r3
EXTRA_CFLAGS += -I$(SDKDIR)/include
EXTRA_CFLAGS += -I$(SDKDIR)/include/Core
EXTRA_CFLAGS += -I$(SDKDIR)/include/Core/Hardware
EXTRA_CFLAGS += -I$(SDKDIR)/include/Core/System
EXTRA_CFLAGS += -I$(SDKDIR)/include/Core/UI
EXTRA_CFLAGS += -I$(SDKDIR)/include/Dynamic
EXTRA_CFLAGS += -I$(SDKDIR)/include/Libraries

PRCDIR = /usr/share/prc-tools
EXTRA_CFLAGS += -I$(PRCDIR)/include

EXTRA_CFLAGS += -fno-builtin
EXTRA_CFLAGS += -Wall
EXTRA_CFLAGS += -Wno-unknown-pragmas
EXTRA_CFLAGS += -U__OPTIMIZE__
EXTRA_CFLAGS += -DLUA_USE_PALMOS

export AR RANLIB STRIP CC
export EXTRA_CFLAGS
export TOPDIR SDKDIR PRCDIR

all:
	cd libc && $(MAKE)
	cd osglue && $(MAKE)
	cd lua && $(MAKE) palmos
	cd palmbind && $(MAKE)
	cd palmapp && $(MAKE)

clean:
	cd libc && $(MAKE) $@
	cd osglue && $(MAKE) $@
	cd lua && $(MAKE) $@
	cd palmbind && $(MAKE) $@
	cd palmapp && $(MAKE) $@
	$(RM) tags cscope.out

tags: ex-tags cscope.out

ex-tags::
	@echo -n "building tags ... "
	@exuberant-ctags -R .
	@echo "done."

CSINC += -I$(SDKDIR)/include
CSINC += -I$(SDKDIR)/include/Core
CSINC += -I$(SDKDIR)/include/Core/Hardware
CSINC += -I$(SDKDIR)/include/Core/System
CSINC += -I$(SDKDIR)/include/Core/UI
CSINC += -I$(SDKDIR)/include/Dynamic
CSINC += -I$(SDKDIR)/include/Libraries
CSINC += -I$(PRCDIR)/include

cscope.out::
	@echo -n "building cscope database ... "
	@cscope -b -R -k $(CSINC)
	@echo "done."

diff:
	svn diff --diff-cmd diff -x -uwbp 2>&1 | $${PAGER:-less}

debug run sync: all
	cd palmapp && $(MAKE) $@

# ----- E O F ---------------------------------------------------
