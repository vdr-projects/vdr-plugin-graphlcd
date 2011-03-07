#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
#
PLUGIN = graphlcd

# define this if you built graphlcd-base with freetype:
HAVE_FREETYPE2 = 1

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' plugin.c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses 

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

export INSTALLPREFIX = /usr
export INSTALLDOCDIR = $(INSTALLPREFIX)/share/doc

### Make sure that necessary options are included:

-include $(VDRDIR)/Make.global

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR (taken from VDR's "config.h"):

VDRVERSION = $(shell grep 'define VDRVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')
APIVERSION = $(shell grep 'define APIVERSION ' $(VDRDIR)/config.h | awk '{ print $$3 }' | sed -e 's/"//g')
ifeq ($(strip $(APIVERSION)),)
  APIVERSION = $(VDRVERSION)
endif


### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)


### Includes and Defines (add further entries here):

INCLUDES += -I./graphlcd-base/ -I$(VDRDIR)/include -I$(INSTALLPREFIX)/include

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

ifdef HAVE_FREETYPE2
    INCLUDES += -I$(INSTALLPREFIX)/include/freetype2
    DEFINES += -DHAVE_FREETYPE2
endif

### The object files (add further files here):

OBJS = display.o layout.o logo.o logolist.o menu.o plugin.o setup.o state.o strfct.o widgets.o

### The main target:
TARGETS = libvdr-$(PLUGIN).so
ifneq ($(shell grep -l 'Phrases' $(VDRDIR)/i18n.c),$(VDRDIR)/i18n.c)
TARGETS += i18n
endif


all: $(TARGETS)

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<


# Dependencies:

MAKEDEP = g++ -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)


### Internationalization (I18N):

PODIR     = po
LOCALEDIR = $(VDRDIR)/locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Ndirs  = $(notdir $(foreach file, $(I18Npo), $(basename $(file))))
I18Npot   = $(PODIR)/$(PLUGIN).pot


%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --msgid-bugs-address='<nobody@domain.com>' -o $@ $^

$(I18Npo): $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q $@ $<
	@touch $@

i18n: $(I18Nmo)
	@mkdir -p $(LOCALEDIR)
	for i in $(I18Ndirs); do\
	    mkdir -p $(LOCALEDIR)/$$i/LC_MESSAGES;\
	    cp $(PODIR)/$$i.mo $(LOCALEDIR)/$$i/LC_MESSAGES/vdr-$(PLUGIN).mo;\
	    done

### Targets:

libvdr-$(PLUGIN).so: $(OBJS)
	$(CXX) $(CXXFLAGS) -L$(INSTALLPREFIX)/lib -L./graphlcd-base/glcddrivers/ -L./graphlcd-base/glcdgraphics/ -shared $(OBJS) -lglcddrivers -lglcdgraphics -lstdc++ -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude .svn --exclude .git --exclude *.cbp --exclude *.layout -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f *.o $(DEPFILE) *.so *.tgz core* *~

install: all
	@install -d $(INSTALLDOCDIR)/$(PLUGIN)
	@install -m 644 README $(INSTALLDOCDIR)/$(PLUGIN)

uninstall:
	@rm -rf $(INSTALLDOCDIR)/$(PLUGIN)
