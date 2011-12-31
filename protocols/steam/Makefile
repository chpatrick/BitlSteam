###########################
## Makefile for BitlBee  ##
##                       ##
## Copyright 2002 Lintux ##
###########################

### DEFINITIONS

-include ../../Makefile.settings
ifdef SRCDIR
SRCDIR := $(SRCDIR)protocols/steam/
endif

# [SH] Program variables
objects = steam.o

CFLAGS += `pkg-config --cflags mono-2 glib`
LDFLAGS += `pkg-config --libs mono-2 glib`
LFLAGS += -r

# [SH] Phony targets
all: steam_mod.o
check: all
lcov: check
gcov: 
	gcov *.c
	
.PHONY: all clean distclean

clean:
	rm -f *.o core

distclean: clean
	rm -rf .depend

### MAIN PROGRAM

$(objects): ../../Makefile.settings Makefile

$(objects): %.o: $(SRCDIR)%.c
	@echo '*' Compiling $<
#	@$(CC) -c $(CFLAGS) -DMONOLIBDIR=${DESTDIR}${plugindir} $< -o $@
	@$(CC) -c $(CFLAGS) $< -o $@

steam_mod.o: $(objects)
	@echo '*' Linking steam_mod.o
	@$(LD) $(LFLAGS) $(objects) -o steam_mod.o
	
-include .depend/*.d
