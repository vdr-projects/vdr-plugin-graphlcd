#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.

PLUGIN = graphlcd

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' plugin.c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(DESTDIR)$(call PKGCFG,libdir)
LOCDIR = $(DESTDIR)$(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags) $(call PKGCFG,plugincflags)

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

ifeq ($(VDRDIR),)
    VDRDIR ?= ../../..
endif
ifeq ($(PLGCFG),)
    PLGCFG = $(VDRDIR)/Make.config
endif
-include $(PLGCFG)

# fallback for building against old VDR versions (like 1.4.7) in the VDR source tree, where you should have Make.config
ifeq ($(LIBDIR),)
    LIBDIR = ../../lib
endif
INCDIR ?= $(VDRDIR)
PREFIX ?= /usr

# make sure to have a correct RESDIR
MYRESDIR = $(call PKGCFG,resdir)
ifeq ($(MYRESDIR),)
    MYRESDIR = /usr/share/vdr
endif
RESDIR := $(DESTDIR)$(MYRESDIR)/plugins/$(PLUGIN)

# make sure to always have a correct APIVERSION
ifeq ($(strip $(APIVERSION)),)
  APIVERSION = $(shell grep 'define APIVERSION ' $(VDRDIR)/config.h | cut -d'"' -f2)
endif

# define this if you built graphlcd-base with freetype:
HAVE_FREETYPE2 ?= 1

# define this if femon-plugin <= 1.7.7 is used AND which has already been patched (see README)
# either define this setting here or in $VDRDIR/Make.config or in $VDRDIR/Make.global
HAVE_VALID_FEMON ?= 0

# define the path to the graphlcd.conf file. if not defined, path = "/etc/graphlcd.conf"
#PLUGIN_GRAPHLCDCONF = "$(CONFDIR)/plugins/$(PLUGIN)/graphlcd.conf"

# defines if installing of TTF should be omitted (default is to install)
SKIP_INSTALL_TTF ?= 0

# defines if installing of documentation should be omitted (default is to install)
SKIP_INSTALL_DOC ?= 0

# if we install the documentation ourselves, do it here:
export INSTALLDOCDIR = $(DESTDIR)$(PREFIX)/share/doc/vdr-$(PLUGIN)-$(VERSION)


### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

INCLUDES += -I./graphlcd-base/ -I$(INCDIR)/include -I$(PREFIX)/include

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

ifneq ($(HAVE_FREETYPE2), 0)
    INCLUDES += -I$(PREFIX)/include/freetype2
    DEFINES += -DHAVE_FREETYPE2
endif

# if a valid and/or fixed femon-plugin is available
ifneq ($(HAVE_VALID_FEMON), 0)
    DEFINES += -DGRAPHLCD_SERVICE_FEMON_VALID
endif

# if a graphlcd.conf different than the default one in /etc is provided
ifdef PLUGIN_GRAPHLCDCONF
    DEFINES += -DPLUGIN_GRAPHLCDCONF='${PLUGIN_GRAPHLCDCONF}'
endif

# if we add TTF to additional install targets
ifeq ($(SKIP_INSTALL_TTF), 0)
    INS_TARGET_TTF = ttf-fonts
endif

# if we add documentations to additional install targets
ifeq ($(SKIP_INSTALL_DOC), 0)
    INS_TARGET_DOCS = docs
endif

### The object files (add further files here):

OBJS = alias.o common.o display.o menu.o plugin.o setup.o skinconfig.o state.o strfct.o service.o extdata.o

ifneq ($(shell grep -l 'Phrases' $(VDRDIR)/i18n.c),$(VDRDIR)/i18n.c)
    I18NTARGET = i18n
else
    OBJS += i18n.o
endif

### The main target:

all: $(SOFILE) $(I18NTARGET)

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

$(SOFILE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) -lglcddrivers -lglcdgraphics -lglcdskin -lstdc++ -o $@

install-lib: $(SOFILE)
	install -D $^ $(LIBDIR)/$^.$(APIVERSION)

install: install-lib install-i18n resources $(INS_TARGET_TTF) $(INS_TARGET_DOCS)

docs:
	@install -d $(INSTALLDOCDIR)
	@install -m 644 README $(INSTALLDOCDIR)
	@install -m 644 HISTORY $(INSTALLDOCDIR)

ttf-fonts:
	@install -d $(RESDIR)/fonts
	@install -m 644 $(PLUGIN)/fonts/*.ttf $(RESDIR)/fonts

resources:
	@install -d $(RESDIR)/fonts
	@install -m 644 $(PLUGIN)/channels.alias $(RESDIR)
	@cp -a $(PLUGIN)/logos $(RESDIR)
	@cp -a $(PLUGIN)/skins $(RESDIR)
	@install -m 644 $(PLUGIN)/fonts/*.fnt $(RESDIR)/fonts

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude .git --exclude *.cbp --exclude *.layout -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~

uninstall:
	@rm -rf $(INSTALLDOCDIR)
	@rm -rf $(RESDIR)
