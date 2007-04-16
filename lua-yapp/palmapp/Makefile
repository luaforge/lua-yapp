# ----- Makefile ------------------------------------------------
# Makefile for the PalmOS application using Lua

MULTIGEN = m68k-palmos-multigen
BLDPRC = build-prc
PILRC = pilrc
POSE = pose
PGDB = m68k-palmos-gdb

CFLAGS += $(EXTRA_CFLAGS)

# # try to set a bigger stack size, but does not work
# LDFLAGS += -Wl,--stack,32768

ifeq ($(NOPARSER),yes)
  NOPARSER_OBJ = ../lua/etc/noparser.o
  CFLAGS += -DNODUMP
endif

CFLAGS += -I../palmbind
LIBS += ../palmbind/libpalmos.a

CFLAGS += -I../lua/src
LIBS += ../lua/src/liblua.a

CFLAGS += -I../osglue
LIBS += ../osglue/libosglue.a

LDFLAGS += -nodefaultlibs
LIBS += ../libc/libc.a
LIBS += $(shell $(CC) -print-libgcc-file-name)

BIN = lua-yapp
LIB = libpalmapp.a
APP = $(BIN).prc
BMP_FILES = 
# BMP_FILES += logo-01-small.bmp
# BMP_FILES += logo-02-small.bmp
# BMP_FILES += logo-04-small.bmp
BMP_FILES += logo-08-small.bmp
# BMP_FILES += logo-16-small.bmp
BMP_FILES += logo-01-large.bmp
BMP_FILES += logo-02-large.bmp
BMP_FILES += logo-04-large.bmp
BMP_FILES += logo-08-large.bmp
BMP_FILES += logo-16-large.bmp

all: $(APP)

$(APP): $(BIN) tver0001.bin
	$(BLDPRC) -o $@ lua-yapp.def $(BIN) *.bin

tver0001.bin: palmapp.rcp palmapp.h $(BMP_FILES)
	$(PILRC) -allowEditID -q palmapp.rcp

$(BIN): $(LIB) $(NOPARSER_OBJ) $(LIBS) lua-yapp-sections.o lua-yapp-sections.ld
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(LIB): palmapp.o
	m68k-palmos-ar rcu $@ $^
	m68k-palmos-ranlib $@

lua-yapp-sections.o: lua-yapp-sections.s
	$(CC) -c $<

lua-yapp-sections.s lua-yapp-sections.ld: lua-yapp.def
	$(MULTIGEN) $<

# ./noparser.o: ../lua/etc/noparser.o
# 	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) a.out *.o *.a *.bin *.grc
	$(RM) *-sections.* *-dispatch.*
	$(RM) $(BIN)

clear: clean
	$(RM) $(APP)

debug: $(BIN) $(APP)
	$(POSE) -psf posession -load_apps $(APP) &
	sleep 2
	$(PGDB) $(BIN)

run: $(APP)
	$(POSE) -psf posession -load_apps $(APP) &

sync: $(APP)
	pilot-xfer -p /dev/ttyUSB1 -i $(APP)

-include Makefile.depend

# ----- E O F ---------------------------------------------------
