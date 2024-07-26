#################################################################
#  Makefile for sample of adif 
#  Copyright (c) 2003-2024 Ke Hengzhong <kehengzhong@hotmail.com>
#  All rights reserved. See MIT LICENSE for redistribution.
#################################################################

SUBDIR := $(shell find . -maxdepth 1 -type d)
SUBDIR := $(basename $(patsubst ./%, %, $(SUBDIR)))
MFILE := $(shell find $(SUBDIR) -name Makefile)
MFDIR := $(patsubst %/Makefile, %, $(MFILE))

all: 
	@for xx in $(MFDIR); do echo "" && make -C $$xx ; done
	@echo ""

debug: 
	@for xx in $(MFDIR); do echo "" && make debug -C $$xx ; done
	@echo ""

clean:
	@for xx in $(MFDIR); do echo "" && make -C $$xx clean; done
	@echo ""

