##===- tools/Makefile --------------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ..

PARALLEL_DIRS += vmkit

include $(LEVEL)/Makefile.config

ifeq ($(WITH_J3), 1)
  PARALLEL_DIRS += j3
  PARALLEL_DIRS += vmjc
  PARALLEL_DIRS += llcj
endif

ifeq ($(WITH_N3_MONO), 1) 
  PARALLEL_DIRS += n3-mono
endif

ifeq ($(WITH_N3_PNETLIB), 1) 
  PARALLEL_DIRS += n3-pnetlib
endif

ifeq ($(WITH_P3), 1) 
  PARALLEL_DIRS += p3
endif

include $(LEVEL)/Makefile.common

