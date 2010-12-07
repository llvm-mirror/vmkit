##===- lib/JnJVM/Classpath/Makefile ------------------------*- Makefile -*-===##
# 
#                     The vmkit project
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..


EXTRA_DIST = ClasspathConstructor.inc ClasspathField.inc Classpath.inc ClasspathMethod.inc \
	     ClasspathVMClass.inc ClasspathVMClassLoader.inc ClasspathVMObject.inc \
	     ClasspathVMRuntime.inc ClasspathVMStackWalker.inc ClasspathVMSystem.inc \
	     ClasspathVMSystemProperties.inc ClasspathVMThread.inc ClasspathVMThrowable.inc

include $(LEVEL)/Makefile.config

ifeq ($(WITH_LLVM_GCC), 1)
  MODULE_NAME = Classpath
else
  LIBRARYNAME = Classpath
endif

include $(LEVEL)/Makefile.common

CXX.Flags += -I$(PROJ_SRC_DIR)/../VMCore $(CLASSPATH_FLAGS)

