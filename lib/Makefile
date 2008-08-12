##===- lib/Makefile ----------------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ..

PARALLEL_DIRS = Mvm

include $(LEVEL)/Makefile.config

ifeq ($(WITH_JNJVM), 1)
PARALLEL_DIRS += JnJVM
endif

ifeq ($(WITH_N3_MONO), 1) || ($(WITH_N3_PNETLIB), 1)
PARALLEL_DIRS += N3
endif

include $(LEVEL)/Makefile.common

