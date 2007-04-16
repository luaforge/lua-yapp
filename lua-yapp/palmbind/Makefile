# ----- Makefile ------------------------------------------------
# Makefile for the PalmOS binding

CFLAGS += $(EXTRA_CFLAGS)
CFLAGS += -I../palmapp	# in case we want to communicate to the app
CFLAGS += -I../lua/src
CFLAGS += -I../osglue

LIB = libpalmos.a

$(LIB): libpalmos.o
	$(AR) rcu $@ $^
	$(RANLIB) $@

clean:
	$(RM) *.o *.a

# ----- E O F ---------------------------------------------------
