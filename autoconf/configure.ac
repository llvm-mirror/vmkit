dnl === configure.ac --------------------------------------------------------===
dnl SECTION 1: Initialization & Setup
dnl SECTION 2: Architecture, target, and host checks
dnl SECTION 3: Command line arguments for the configure script.
dnl SECTION 4: Check for programs we need and that they are the right version
dnl SECTION 5: Check for libraries
dnl SECTION 6: Check for header files
dnl SECTION 7: Check for types and structures
dnl SECTION 8: Check for specific functions needed
dnl SECTION 9: Additional checks, variables, etc.
dnl SECTION 10: Specify the output files and generate it
dnl
dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 1: Initialization & Setup
dnl===
dnl===-----------------------------------------------------------------------===
dnl Initialize autoconf and define the package name, version number and
dnl email address for reporting bugs.
AC_INIT([vmkit],[0.1],[gael.thomas@lip6.fr])

dnl Provide a copyright substitution and ensure the copyright notice is included
dnl in the output of --version option of the generated configure script.
AC_COPYRIGHT([Copyright (c) 2013 Purdue Univeristy/Université Pierre et Marie Curie.])

dnl Indicate that we require autoconf 2.59 or later. Ths is needed because we
dnl use some autoconf macros only available in 2.59.
AC_PREREQ(2.59)

dnl Verify that the source directory is valid. This makes sure that we are
dnl configuring reactor and not some other package (it validates --srcdir argument)
AC_CONFIG_SRCDIR([include/j3/j3config.h.in])

dnl Place all of the extra autoconf files into the config subdirectory. Tell
dnl various tools where the m4 autoconf macros are.
AC_CONFIG_AUX_DIR([autoconf])

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 2: Architecture, target, and host checks
dnl===
dnl===-----------------------------------------------------------------------===
AC_CANONICAL_TARGET

dnl Determine the platform type and cache its value. This helps us configure
dnl the System library to the correct build platform.
AC_CACHE_CHECK([type of operating system we're going to host on],
               [vmkit_cv_os_type],
[case $host in
  *-*-aix*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-irix*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-cygwin*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-darwin*)
		OS="darwin"
		SHLIBEXT=".dylib"
#    SHOPT="-fno-common -Wl,-flat_namespace -Wl,-undefined,suppress" 
 	  SHOPT="-Wl,-all_load -Wl,-compatibility_version,1 -Wl,-current_version,1"
		EXEEXT="" ;;
  *-*-freebsd*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-openbsd*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-netbsd*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-hpux*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-interix*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-linux*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-solaris*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-win32*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *-*-mingw*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
  *)
    AC_MSG_ERROR([Good luck porting vmkit to your host!]) ;;
esac])

dnl Make sure we aren't attempting to configure for an unknown system
AC_SUBST([OS])
AC_SUBST([SHLIBEXT])
AC_SUBST([SHOPT])
AC_SUBST([EXEEXT])

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 3: Command line arguments for the configure script.
dnl===
dnl===-----------------------------------------------------------------------===

dnl **************************************************************************
dnl optimization and debug
dnl **************************************************************************
AC_ARG_ENABLE(optimized,
              AS_HELP_STRING([--enable-optimized],
                             [Build with all optimization flag enable (default is yes)]),,
                             enable_optimized=yes)
case "$enable_optimized" in
  yes) AC_SUBST(OPTIMIZED,[1]) ;;
  no)  AC_SUBST(OPTIMIZED,[0]) ;;
  *) AC_MSG_ERROR([Invalid setting for --enable-optimized. Use "yes" or "no"]) ;;
esac

AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug],
                             [Build with debug flags (default is no)]),,
                             enable_debug=no)
case "$enable_debug" in
  yes) AC_SUBST(DEBUG,[1]) ;;
  no)  AC_SUBST(DEBUG,[0]) ;;
  *) AC_MSG_ERROR([Invalid setting for --enable-debug. Use "yes" or "no"]) ;;
esac

AC_ARG_ENABLE(assert,
              AS_HELP_STRING([--enable-assert],
                             [Build with assert flags (default is yes)]),,
                             enable_assert=yes)

case "$enable_assert" in
  yes) AC_SUBST(ASSERT,[1]) ;;
  no)  AC_SUBST(ASSERT,[0]) ;;
  *) AC_MSG_ERROR([Invalid setting for --enable-assert. Use "yes" or "no"]) ;;
esac

if test "$enable_optimized" = "yes"; then
   BUILD_NAME=Release
   if test "$enable_debug" = "yes"; then
      BUILD_NAME="$BUILD_NAME"+Debug
   fi 
else
   if test "$enable_debug" = "yes"; then
      BUILD_NAME=Debug
   else
      BUILD_NAME=Unoptimized
   fi
fi

if test "$enable_assert" = "yes"; then
   BUILD_NAME="$BUILD_NAME"+Asserts
fi

AC_SUBST([BUILD_NAME], ["$BUILD_NAME"])

dnl **************************************************************************
dnl llvm
dnl **************************************************************************
AC_ARG_WITH(llvm-config-path,
       [AS_HELP_STRING(--with-llvm-config-path=path,
           [llvm-config path (use default path)])],
       [[LLVM_CONFIG=$with_llvm_config_path]],
       [[LLVM_CONFIG="llvm-config"]]
)

if test ! -x "$LLVM_CONFIG"; then
     AC_MSG_ERROR([Cannot find $LLVM_CONFIG (or not executable)])
fi

LLVM_PATH="`$LLVM_CONFIG --bindir`"

llvm_cfg=`$LLVM_CONFIG --cxxflags | sed -e 's/-O3//'`

if ! test -z "`echo $llvm_cfg | grep -- -fno-exceptions`"; then
	 		AC_MSG_ERROR([You have to compile LLVM with exception enabled, please compile it with 'make REQUIRES_EH=1'])
fi

AC_SUBST(LLVM_CXXFLAGS, [$llvm_cfg" "-I`$LLVM_CONFIG --src-root`/include])
AC_SUBST(LLVM_LIBS, [`$LLVM_CONFIG --libs`])
AC_SUBST(LLVM_LDFLAGS, [`$LLVM_CONFIG --ldflags`])

dnl **************************************************************************
dnl clang
dnl **************************************************************************
AC_ARG_WITH(clang-path,
       [AS_HELP_STRING(--with-clang-path=path,
           [clang path (use llvm-config --bindir and then default path)])],
       [[CLANG_PATH=$with_clang_path]]
)

if test -z "$CLANG_PATH" -a ! -z "$LLVM_CONFIG"; then
 	 CLANG_PATH="`$LLVM_CONFIG --bindir`"
fi

dnl **************************************************************************
dnl OpenJDK
dnl **************************************************************************

AC_ARG_WITH(jdkhome,
       [AS_HELP_STRING(--with-jdkhome,
           [Build J3 with OpenJDK JRE install (default is $JAVA_HOME or /usr/lib/java)])],
       [[jdkhome=$with_jdkhome]],
       [[jdkhome=""]]
)

if test -z "$jdkhome"; then
	 if test -z "$JAVA_HOME"; then
	 		jdkhome=/usr/lib/java
	 else
			jdkhome=$JAVA_HOME
	 fi
fi

AC_SUBST([jdkhome])

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 4: Check for programs we need and that they are the right version
dnl===
dnl===-----------------------------------------------------------------------===
dnl Check for compilation tools
AC_PROG_CPP

dnl Find the install program
AC_PROG_INSTALL

dnl AC_SUBST(SHLIBEXT,$libltdl_cv_shlibext)

AC_DEFUN([AX_PATH_PROG_FAIL_OR_SUBST], [
  AC_PATH_PROG([$1], [$2], [no], [$3:$PATH]) 
	  if test x"${$1}" == x"no"; then 
      AC_MSG_ERROR([Unable to find $2, please install $2]) 
		fi
  AC_SUBST([$1])
]) 

AX_PATH_PROG_FAIL_OR_SUBST([LLC],     llc,       $LLVM_PATH)
AX_PATH_PROG_FAIL_OR_SUBST([LLOPT],   opt,       $LLVM_PATH)
AX_PATH_PROG_FAIL_OR_SUBST([LLLINK],  llvm-link, $LLVM_PATH)
AX_PATH_PROG_FAIL_OR_SUBST([LLNM],    llvm-nm,   $LLVM_PATH)
AX_PATH_PROG_FAIL_OR_SUBST([CLANG],   clang,     $CLANG_PATH)
AX_PATH_PROG_FAIL_OR_SUBST([CLANGXX], clang++,   $CLANG_PATH)
AX_PATH_PROG_FAIL_OR_SUBST([GAWK],    gawk,      "")

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 5: Check for libraries
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 6: Check for header files
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 7: Check for types and structures
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 8: Check for specific functions needed
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 9: Additional checks, variables, etc.
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 10: Specify the output files and generate it
dnl===
dnl===-----------------------------------------------------------------------===

dnl Configure header files
AC_CONFIG_FILES([Makefile.config include/j3/j3config.h])

AC_SUBST([ac_config_files])
AC_SUBST([ac_config_headers])

AC_OUTPUT
