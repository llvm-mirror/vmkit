##===- tools/Makefile --------------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ..

PARALLEL_DIRS = 

include $(LEVEL)/Makefile.config

ifeq ($(WITH_JNJVM), 1)
  PARALLEL_DIRS += jnjvm
endif

ifeq ($(WITH_N3), 1) 
  PARALLEL_DIRS += n3
endif


include $(LEVEL)/Makefile.common

