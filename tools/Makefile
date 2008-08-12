##===- tools/Makefile --------------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ..

PARALLEL_DIRS = vmkit

include $(LEVEL)/Makefile.config

ifeq ($(WITH_JNJVM), 1)
  PARALLEL_DIRS += jnjvm
endif

ifeq ($(WITH_N3_MONO), 1) 
  PARALLEL_DIRS += n3-mono
endif

ifeq ($(WITH_N3_PNETLIB), 1) 
  PARALLEL_DIRS += n3-pnetlib
endif


include $(LEVEL)/Makefile.common

