# ----- Makefile ------------------------------------------------
# Makefile for the PalmOS specific glue under Lua

CFLAGS += $(EXTRA_CFLAGS)
CFLAGS += -I../palmapp

all: libosglue.a

libosglue.a: osglue.o pdouble.o codesections.o
	$(AR) rcu $@ $^
	$(RANLIB) $@

pdouble.o: ../contrib/pdouble/pdouble.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o *.a *.bin

# ----- E O F ---------------------------------------------------
