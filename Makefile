
#################################################################
#  Makefile for Application Development Interface Fundamental - adif
#  Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
#  All rights reserved. See MIT LICENSE for redistribution.
#################################################################

PKGNAME = adif
PKGLIB = lib$(PKGNAME)
PKG_SO_LIB = $(PKGLIB).so
PKG_A_LIB = $(PKGLIB).a

PREFIX = /usr/local
INSTALL_INC_PATH = $(DESTDIR)$(PREFIX)/include/adif
INSTALL_LIB_PATH = $(DESTDIR)$(PREFIX)/lib

ROOT := .
PKGPATH := $(shell basename `/bin/pwd`)

adif_inc = $(ROOT)/include
adif_src = $(ROOT)/src

inc = $(ROOT)/include
obj = $(ROOT)/obj
dst = $(ROOT)/lib

alib = $(dst)/$(PKG_A_LIB)
solib = $(dst)/$(PKG_SO_LIB)

#################################################################
#  Customization of shared object library (SO)

PKG_VER_MAJOR = 2
PKG_VER_MINOR = 6
PKG_VER_RELEASE = 20
PKG_VER = $(PKG_VER_MAJOR).$(PKG_VER_MINOR).$(PKG_VER_RELEASE)

PKG_VERSO_LIB = $(PKG_SO_LIB).$(PKG_VER)
PKG_SONAME_LIB = $(PKG_SO_LIB).$(PKG_VER_MAJOR)
LD_SONAME = -Wl,-soname,$(PKG_SONAME_LIB)


#################################################################
#  Customization of the implicit rules

CC = gcc

IFLAGS = -I$(adif_inc)

CFLAGS = -Wall -O3 -fPIC
LFLAGS = -L/usr/lib
LIBS = -lnsl -lm -lz -lpthread
SOFLAGS = $(LD_SONAME)

ifeq ($(MAKECMDGOALS), debug)
  DEFS += -D_DEBUG
  CFLAGS += -g
endif

ifeq ($(MAKECMDGOALS), so)
  CFLAGS += 
endif


#################################################################
# Set long and pointer to 64 bits or 32 bits

ifeq ($(BITS),)
  CFLAGS += -m64
else ifeq ($(BITS),64)
  CFLAGS += -m64
else ifeq ($(BITS),32)
  CFLAGS += -m32
else ifeq ($(BITS),default)
  CFLAGS += 
else
  CFLAGS += $(BITS)
endif


#################################################################
# OS-specific definitions and flags

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
  DEFS += -DUNIX -D_LINUX_
endif

ifeq ($(UNAME), FreeBSD)
  DEFS += -DUNIX -D_FREEBSD_
endif

ifeq ($(UNAME), Darwin)
  DEFS += -DOSX

  PKG_VERSO_LIB = $(PKGLIB).$(PKG_VER).dylib
  PKG_SONAME_LIB = $(PKGLIB).$(PKG_VER_MAJOR).dylib
  LD_SONAME=

  SOFLAGS += -install_name $(dst)/$(PKGLIB).dylib
  SOFLAGS += -compatibility_version $(PKG_VER_MAJOR)
  SOFLAGS += -current_version $(PKG_VER)
endif

ifeq ($(UNAME), Solaris)
  DEFS += -DUNIX -D_SOLARIS_
endif
 

#################################################################
# Merge the rules

CFLAGS += $(DEFS)
LIBS += $(APPLIBS)
 

#################################################################
#  Customization of the implicit rules - BRAIN DAMAGED makes (HP)

AR = ar
ARFLAGS = rv
RANLIB = ranlib
RM = /bin/rm -f
COMPILE.c = $(CC) $(CFLAGS) $(IFLAGS) -c
LINK = $(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) -o
SOLINK = $(CC) $(CFLAGS) $(IFLAGS) $(LFLAGS) -shared $(SOFLAGS) -o

#################################################################
#  Modules

cnfs = $(wildcard $(adif_inc)/*.h)
sources = $(wildcard $(adif_src)/*.c)
objs = $(patsubst $(adif_src)/%.c,$(obj)/%.o,$(sources))


#################################################################
#  Standard Rules

.PHONY: all clean debug show cleanlib

all: $(alib) $(solib)
so: $(solib)
debug: $(alib) $(solib)
clean: 
	$(RM) $(objs)
	$(RM) -r $(obj)
cleanlib: 
	@cd $(dst) && $(RM) $(PKG_A_LIB)
	@cd $(dst) && $(RM) $(PKG_SO_LIB)
	@cd $(dst) && $(RM) $(PKG_SONAME_LIB)
	@cd $(dst) && $(RM) $(PKG_VERSO_LIB)
show:
	@echo $(alib)
	ls $(objs)

dist: $(cnfs) $(sources)
	cd $(ROOT)/.. && tar czvf $(PKGNAME)-$(PKG_VER).tar.gz $(PKGPATH)/src \
	    $(PKGPATH)/include $(PKGPATH)/lib $(PKGPATH)/Makefile $(PKGPATH)/README.md \
	    $(PKGPATH)/LICENSE

install: $(alib) $(solib)
	mkdir -p $(INSTALL_INC_PATH) $(INSTALL_LIB_PATH)
	install -s $(dst)/$(PKG_A_LIB) $(INSTALL_LIB_PATH)
	cp -af $(dst)/$(PKG_VERSO_LIB) $(INSTALL_LIB_PATH)
	@cd $(INSTALL_LIB_PATH) && $(RM) $(PKG_SONAME_LIB) && ln -sf $(PKG_VERSO_LIB) $(PKG_SONAME_LIB)
	@cd $(INSTALL_LIB_PATH) && $(RM) $(PKG_SO_LIB) && ln -sf $(PKG_SONAME_LIB) $(PKG_SO_LIB)
	cp -af $(inc)/*.h $(INSTALL_INC_PATH)
	cp -af $(inc)/*.ext $(INSTALL_INC_PATH)

uninstall:
	cd $(INSTALL_LIB_PATH) && $(RM) $(PKG_SO_LIB)
	cd $(INSTALL_LIB_PATH) && $(RM) $(PKG_SONAME_LIB) 
	cd $(INSTALL_LIB_PATH) && $(RM) $(PKG_VERSO_LIB) 
	cd $(INSTALL_LIB_PATH) && $(RM) $(PKG_A_LIB) 
	$(RM) $(INSTALL_INC_PATH)/*
	$(RM) -r $(INSTALL_INC_PATH)

#################################################################
#  Additional Rules
#
#  target1 [target2 ...]:[:][dependent1 ...][;commands][#...]
#  [(tab) commands][#...]
#
#  $@ - variable, indicates the target
#  $? - all dependent files
#  $^ - all dependent files and remove the duplicate file
#  $< - the first dependent file
#  @echo - print the info to console
#
#  SOURCES = $(wildcard *.c *.cpp)
#  OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))
#  CSRC = $(filter %.c,$(files))


$(solib): $(objs) 
	$(SOLINK) $(dst)/$(PKG_VERSO_LIB) $? 
	@cd $(dst) && $(RM) $(PKG_SONAME_LIB) && ln -s $(PKG_VERSO_LIB) $(PKG_SONAME_LIB)
	@cd $(dst) && $(RM) $(PKG_SO_LIB) && ln -s $(PKG_SONAME_LIB) $(PKG_SO_LIB)
     
$(alib): $(objs) 
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $(RANLIBFLAGS) $@

$(obj)/%.o: $(adif_src)/%.c $(cnfs)
	@mkdir -p $(obj)
	$(COMPILE.c) $< -o $@

