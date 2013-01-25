#
# Makefile for a Video Disk Recorder plugin
#
# Compliant with: VDR >= 1.7.34 (new Makefile style) but also VDR 1.4.x, VDR 1.6.x, up to VDR 1.7.33 ('old style Makefiles')
#
# $Id$


# The official name of this plugin.

PLUGIN = graphlcd

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' plugin.c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags)

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)


# FLAG_PKGCFG: is pkg-config existing for vdr?
#    'auto'    : via pkg-config vdr,
#    'absolute': via pkg-config $(VDRDIR)/vdr.pc,
#    'relative': via pkg-config ../../../vdr.pc,
#    'no'      : compiling w/o pkg-config
FLAG_PKGCFG=no
ifneq ($(VDRDIR),)
  ifeq ($(shell pkg-config --exists $(VDRDIR)/vdr.pc && echo yes),yes)
    FLAG_PKGCFG=absolute
  endif
else
  ifeq ($(shell pkg-config --exists vdr && echo yes),yes)
    FLAG_PKGCFG=auto
  else
    ifeq ($(shell pkg-config --exists ../../../vdr.pc && echo yes),yes)
      FLAG_PKGCFG=relative
    endif
  endif
endif

ifeq ($(strip $(APIVERSION)),)
  # APIVERSION is empty? either pkg-config was not successful or vdr is too old:
  # assume in-vdr-tree compilation: set VDRDIR Makefile-wide
  VDRDIR ?= ../../..
  APIVERSION = $(shell grep 'define APIVERSION ' $(VDRDIR)/config.h | cut -d'"' -f2)
endif

# still no APIVERSION? bail out
ifeq ($(APIVERSION),)
  $(error no APIVERSION found, bailing out ...)
endif

# get numeric VDR version number for numeric comparisons (eg: 1.7.47 -> 10747)
APIVERSNUM = $(shell printf "%2d%02d%02d" $(subst ., ,$(APIVERSION)))

# non-Makefile-wide VDRDIR only for internal use
ifeq ($(VDRDIR),)
  TEMP_VDRDIR = ../../..
else
  TEMP_VDRDIR = $(VDRDIR)
endif

# post 1.7.33 vdr?
FLAG_NEWSTYLE=false
$(shell [ $(APIVERSNUM) -gt 10733 ] && FLAG_NEWSTYLE=true)

# do some adaptions and defaults for old vdr versions
ifeq ($(FLAG_NEWSTYLE),false)
  CXXFLAGS += $(call PKGCFG,plugincflags)
  export CXXFLAGS

  ifeq ($(LOCDIR),)
    LOCDIR = $(call PKGCFG,localedir)
    ifeq ($(LOCDIR),)
      LOCDIR = $(TEMP_VDRDIR)/locale
    endif
  endif
  ifeq ($(LOCDIR),./locale)
    LOCDIR = $(TEMP_VDRDIR)/locale
  endif

  ifeq ($(PLGCFG),)
    PLGCFG = $(TEMP_VDRDIR)/Make.config
  endif

  # fallbacks
  ifeq ($(LIBDIR),)
    LIBDIR = $(TEMP_VDRDIR)/PLUGINS/lib
  endif
endif
-include $(PLGCFG)

# some paranoia security checks
ifeq ($(LIBDIR),)
  $(error LIBDIR not set, bailing out ...)
endif
ifeq ($(shell [ $(APIVERSNUM) -ge 10500 ] && echo yes),yes)
  ifeq ($(LOCDIR),)
    $(error LOCDIR not set, bailing out ...)
  endif
endif

  
### Allow user defined options to overwrite defaults:

# make sure to have a correct RESDIR
MYRESDIR = $(call PKGCFG,resdir)
ifeq ($(MYRESDIR),)
    MYRESDIR = /usr/share/vdr
endif
RESDIR := $(MYRESDIR)/plugins/$(PLUGIN)


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

# if no prefix defined: use a default one
PREFIX ?= /usr

# if we install the documentation ourselves, do it here:
export INSTALLDOCDIR = $(PREFIX)/share/doc/vdr-$(PLUGIN)-$(VERSION)


### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

#include $(VDRDIR)/include if vdr is not installed via package-systems or the like
ifneq ($(FLAG_PKGCFG),auto)
  INCLUDES += -I$(TEMP_VDRDIR)/include
endif
INCLUDES += -I./graphlcd-base/ -I$(PREFIX)/include

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

ifneq ($(HAVE_FREETYPE2), 0)
    INCLUDES += $(shell pkg-config --cflags freetype2 || echo "-I$(PREFIX)/include/freetype2")
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

# internationalisation: check if new style (> vdr 1.4.x) or fall back to 1.4.x
ifeq ($(shell [ $(APIVERSNUM) -ge 10500 ] && echo yes),yes)
    I18NTARGET = i18n
else
    OBJS += i18n.o
endif

### The main target:

all: $(SOFILE) $(I18NTARGET)

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

$(SOFILE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) -lglcddrivers -lglcdgraphics -lglcdskin -lstdc++ -o $@

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install: install-lib install-i18n resources $(INS_TARGET_TTF) $(INS_TARGET_DOCS)

docs:
	@install -d $(DESTDIR)$(INSTALLDOCDIR)
	@install -m 644 README $(DESTDIR)$(INSTALLDOCDIR)
	@install -m 644 HISTORY $(DESTDIR)$(INSTALLDOCDIR)

ttf-fonts:
	@install -d $(DESTDIR)$(RESDIR)/fonts
	@install -m 644 $(PLUGIN)/fonts/*.ttf $(DESTDIR)$(RESDIR)/fonts

resources:
	@install -d $(DESTDIR)$(RESDIR)/fonts
	@install -m 644 $(PLUGIN)/channels.alias $(DESTDIR)$(RESDIR)
	@cp -a $(PLUGIN)/logos $(DESTDIR)$(RESDIR)
	@cp -a $(PLUGIN)/skins $(DESTDIR)$(RESDIR)
	@install -m 644 $(PLUGIN)/fonts/*.fnt $(DESTDIR)$(RESDIR)/fonts

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
	@-rm -rf $(DESTDIR)$(INSTALLDOCDIR)
	@-rm -rf $(DESTDIR)$(RESDIR)
