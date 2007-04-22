# ----- Makefile ------------------------------------------------
# create a trimmed down version of the PalmOS libc.a library
# (string/memory, ctype; no input/output nor conversion)

# which .a file to build here
LIBRARY = libc.a

# where to extract the .o files from
TEMPLATE = $(M68KDIR)/lib/libc.a

# which .o files to use
OBJECTS = 
OBJECTS += ctype.o \
	isalnum.o isalpha.o isblank.o iscntrl.o isdigit.o isgraph.o \
	islower.o isprint.o ispunct.o isspace.o isupper.o isxdigit.o \
	tolower.o toupper.o
OBJECTS += abs.o div.o
OBJECTS += memcpy.o memmove.o memcmp.o memchr.o memset.o
OBJECTS += strcpy.o strncpy.o strcat.o strncat.o strcmp.o strncmp.o \
	strlen.o strchr.o strrchr.o strstr.o strspn.o strcspn.o strpbrk.o 

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) rcu $@ $^
	$(RANLIB) $@

$(OBJECTS): $(TEMPLATE)
	$(AR) x $< $@

clean:
	$(RM) *.o *.a

# ----- E O F ---------------------------------------------------
