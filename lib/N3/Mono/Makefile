##===- lib/N3/Mono/Makefile --------------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..

include $(LEVEL)/Makefile.config

ifeq ($(WITH_LLVM_GCC), 1)
  MODULE_NAME = Mono
else
  LIBRARYNAME = Mono
endif

EXTRA_DIST = MonoPath.inc.in
include $(LEVEL)/Makefile.common
CXX.Flags += -I$(PROJ_SRC_DIR)/../VMCore

CXX.Flags += `pkg-config --cflags glib-2.0 gthread-2.0`
CXX.Flags += `pkg-config --cflags gmodule-2.0`
