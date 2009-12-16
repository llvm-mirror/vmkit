##===- lib/JnJVM/Makefile ----------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../..

DIRS = LLVMRuntime VMCore Classpath Compiler

include $(LEVEL)/Makefile.config

ifeq ($(ISOLATE_SHARING_BUILD), 1) 
    DIRS += Isolate
endif



include $(LEVEL)/Makefile.common

