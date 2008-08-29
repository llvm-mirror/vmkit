##===- lib/JnJVM/Makefile ----------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

DIRS = LLVMRuntime VMCore Classpath

include $(LEVEL)/Makefile.config

ifeq ($(ISOLATE_BUILD), 1) 
    DIRS += Isolate
endif



include $(LEVEL)/Makefile.common

