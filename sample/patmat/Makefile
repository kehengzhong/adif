
#################################################################
#  Makefile for comparisom of pattern-matching algorithm
#  (c) 2024 Ke Heng Zhong (Beijing, China)
#  Writen by ke hengzhong (kehengzhong@hotmail.com)
#################################################################

PKGNAME = patmat

PKGBIN = $(PKGNAME)

ROOT := $(abspath .)

#PREFIX = /usr/local
PREFIX = $(abspath ../..)


adif_inc = $(PREFIX)/include
adif_lib = $(PREFIX)/lib

main_inc = $(ROOT)
main_src = $(ROOT)

obj = $(ROOT)
dst = $(ROOT)

bin = $(dst)/$(PKGBIN)

RPATH = -Wl,-rpath,/usr/local/lib


#################################################################
#  Customization of the implicit rules

CC = gcc

IFLAGS = -I$(adif_inc)

CFLAGS = -Wall -fPIC
LFLAGS = -L/usr/lib -L$(adif_lib)
LIBS = -lm -lpthread

APPLIBS = -ladif $(RPATH)


ifeq ($(MAKECMDGOALS), debug)
  DEFS += -D_DEBUG
  CFLAGS += -g -O0
else
  CFLAGS += -O3
endif

ifeq ($(MAKECMDGOALS), so)
  CFLAGS += 
endif

ifeq ($(shell test -e /usr/include/openssl/ssl.h && echo 1), 1)
  DEFS += -DHAVE_OPENSSL
  LIBS += -lssl -lcrypto
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
  LIBS += -liconv
endif

ifeq ($(UNAME), Darwin)
  DEFS += -D_OSX_
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

cnfs = $(wildcard $(main_inc)/*.h)
sources = $(wildcard $(main_src)/*.c)
objs = $(patsubst $(main_src)/%.c,$(obj)/%.o,$(sources))


#################################################################
#  Standard Rules

.PHONY: all clean debug show

all: $(bin) 
debug: $(bin)
clean: 
	$(RM) $(objs)
	@cd $(dst) && $(RM) $(PKGBIN)
show:
	@echo $(bin)


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


$(bin): $(objs) 
	$(LINK) $@ $? $(LIBS)

$(obj)/%.o: $(main_src)/%.c $(cnfs)
	@mkdir -p $(obj)
	$(COMPILE.c) $< -o $@

