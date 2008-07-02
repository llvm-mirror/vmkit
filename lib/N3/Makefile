##===- lib/Makefile ----------------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

include $(LEVEL)/Makefile.config

DIRS = VMCore

ifeq ($(WITH_N3_MONO), 1) 
    DIRS += Mono
endif

ifeq ($(WITH_N3_PNETLIB), 1) 
    DIRS += PNetLib
endif


include $(LEVEL)/Makefile.common
