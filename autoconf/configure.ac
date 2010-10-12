dnl === configure.ac --------------------------------------------------------===
dnl                     The vmkit project
dnl
dnl This file is distributed under the University of Illinois Open Source
dnl License. See LICENSE.TXT for details.
dnl
dnl===-----------------------------------------------------------------------===
dnl This is the VMKit configuration script. It is processed by the autoconf
dnl program to produce a script named configure. This script contains the
dnl configuration checks that VMKit needs in order to support multiple platforms.
dnl This file is composed of 10 sections per the recommended organization of
dnl autoconf input defined in the autoconf documentation. As this file evolves,
dnl please keep the various types of checks within their sections. The sections
dnl are as follows:
dnl
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
AC_INIT([vmkit],[0.29svn],[nicolas.geoffray@gmail.com])

dnl Provide a copyright substitution and ensure the copyright notice is included
dnl in the output of --version option of the generated configure script.
AC_SUBST(VMKIT_COPYRIGHT,["Copyright (c) 2003-2010 Universite Pierre et Marie Curie."])
AC_COPYRIGHT([Copyright (c) 2003-2010 Universite Pierre et Marie Curie.])

dnl Indicate that we require autoconf 2.59 or later. Ths is needed because we
dnl use some autoconf macros only available in 2.59.
AC_PREREQ(2.59)

dnl Verify that the source directory is valid. This makes sure that we are
dnl configuring VMKit and not some other package (it validates --srcdir argument)
AC_CONFIG_SRCDIR([lib/Mvm/Runtime/Object.cpp])


dnl Quit if the source directory has already been configured.
dnl NOTE: This relies upon undocumented autoconf behavior.
if test ${srcdir} != "." ; then
  if test -f ${srcdir}/include/mvm/Config/config.h ; then
    AC_MSG_ERROR([Already configured in ${srcdir}])
  fi
fi

dnl Place all of the extra autoconf files into the config subdirectory. Tell
dnl various tools where the m4 autoconf macros are.
AC_CONFIG_AUX_DIR([$LLVM_SRC_ROOT/autoconf])

dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options
LLVM_CONFIG_PROJECT($LLVM_SRC_ROOT,$LLVM_OBJ_ROOT)


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 2: Architecture, target, and host checks
dnl===
dnl===-----------------------------------------------------------------------===

dnl Check the target for which we're compiling and the host that will do the
dnl compilations. This will tell us which LLVM compiler will be used for
dnl compiling SSA into object code. This needs to be done early because
dnl following tests depend on it.
AC_CANONICAL_TARGET

dnl Determine the platform type and cache its value. This helps us configure
dnl the System library to the correct build platform.
AC_CACHE_CHECK([type of operating system we're going to host on],
               [vmkit_cv_os_type],
[case $host in
  *-*-aix*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="AIX"
    vmkit_cv_platform_type="Unix" ;;
  *-*-irix*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="IRIX"
    vmkit_cv_platform_type="Unix" ;;
  *-*-cygwin*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="Cygwin"
    vmkit_cv_platform_type="Unix" ;;
  *-*-darwin*)
    DYLIB_EXTENSION="dylib"
    vmkit_cv_os_type="Darwin"
    vmkit_cv_platform_type="Unix" ;;
  *-*-freebsd*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="FreeBSD"
    vmkit_cv_platform_type="Unix" ;;
  *-*-openbsd*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="OpenBSD"
    vmkit_cv_platform_type="Unix" ;;
  *-*-netbsd*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="NetBSD"
    vmkit_cv_platform_type="Unix" ;;
  *-*-hpux*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="HP-UX"
    vmkit_cv_platform_type="Unix" ;;
  *-*-interix*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="Interix"
    vmkit_cv_platform_type="Unix" ;;
  *-*-linux*)
    DYLIB_EXTENSION="so"
    vmkit_cv_os_type="Linux"
    vmkit_cv_platform_type="Unix" ;;
  *-*-solaris*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="SunOS"
    vmkit_cv_platform_type="Unix" ;;
  *-*-win32*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="Win32"
    vmkit_cv_platform_type="Win32" ;;
  *-*-mingw*)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="MingW"
    vmkit_cv_platform_type="Win32" ;;
  *)
    AC_MSG_ERROR([Good luck porting vmkit to your host!])
    vmkit_cv_os_type="Unknown"
    vmkit_cv_platform_type="Unknown" ;;
esac])

dnl Make sure we aren't attempting to configure for an unknown system

AC_SUBST([DYLIB_EXTENSION])

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 3: Command line arguments for the configure script.
dnl===
dnl===-----------------------------------------------------------------------===

AC_ARG_WITH(llvmgcc,
       [AS_HELP_STRING(--with-llvmgcc,
           [Compile with llvm-gcc])],
       [[withllvm=yes]],
       [[withllvm=no]],
)

if test "x$withllvm" = "xyes";  then
  WITH_LLVM_GCC=1
  LLVM_FLAGS=-DWITH_LLVM_GCC
fi

AC_SUBST([WITH_LLVM_GCC])
AC_SUBST([LLVM_FLAGS])

AC_ARG_WITH(thread,
       [AS_HELP_STRING(--with-thread=something,
           [Thread type ('common' or 'no')])],
       [thread=$withval],[thread=common]
)

AS_IF([test "x$thread" != "xno"],
  [AC_CHECK_HEADER([pthread.h],,
    [AC_MSG_WARN(phtread include NOT found)])
  AC_CHECK_LIB(pthread, pthread_create, [], 
     [AC_MSG_ERROR([pthread library not found])])
  ]
)

if test "x$thread" = xcommon; then
  AC_DEFINE([HAVE_PTHREAD], [1], [Using pthread library])
fi

AC_ARG_WITH(finalizer,
       [AS_HELP_STRING(--with-finalizer,
           [Compile with finalizer (default yes)])],
       [[withfinalizer=$withfinalizer]],
       [[withfinalizer=yes]],
)

if test ! "x$withfinalizer" = "xyes";  then
  VM_FLAGS="$VM_FLAGS -DWITHOUT_FINALIZER"
fi

dnl **************************************************************************
dnl Architecture
dnl **************************************************************************

case $target_cpu in
  powerpc) 
    AC_DEFINE([WITH_64], [0], [Building on a platform without 64bit cas])
    AC_SUBST([WITH_64], [0]);;
  *) 
    AC_DEFINE([WITH_64], [1], [Building on a platform with 64bit cas])
    AC_SUBST([WITH_64], [1]);;
esac



dnl **************************************************************************
dnl GC type 
dnl **************************************************************************
AC_ARG_WITH(gc,
       [AS_HELP_STRING(--with-gc=something,
           [GC type ('mmtk' (requires llvm-gcc) 'single-mmap' 'multi-mmap' or 'boehm')])],
       [[gc=$withval]],
       [[ echo Using mmap2 as vvm gc type.
                gc=single-mmap
       ]]
)

AC_ARG_WITH(mmtk-plan,
       [AS_HELP_STRING(--with-mmtk-plan=something,
           [MMTk plan type ('marksweep')])],
       [[MMTK_PLAN=$withval]],
       [[MMTK_PLAN=marksweep]]
)

if test "x$gc" = "xboehm";  then
  GC_FLAGS="-I\$(PROJ_SRC_ROOT)/lib/Mvm/BoehmGC -DGC_THREADS"
  AC_DEFINE([USE_GC_BOEHM], [1], [Using the boehm gc])
  AC_SUBST(GC_MMAP2, [0])
  AC_SUBST(GC_BOEHM, [1])
  AC_SUBST(GC_MMTK, [0])
  GC_LIBS=BoehmGC
  case $target_os in
    *linux*)
      GC_FLAGS="-I\$(PROJ_SRC_ROOT)/lib/Mvm/BoehmGC -DUSE_GC_BOEHM -DGC_THREADS -DGC_LINUX_THREADS"
    ;;
  esac
else
  if test "x$gc" = "xmmtk";  then
    AC_SUBST([GC_MULTI_MMAP], [0])
    AC_SUBST([GC_SINGLE_MMAP], [1])
    AC_SUBST(GC_MMAP2, [0])
    AC_SUBST(GC_BOEHM, [0])
    AC_SUBST(GC_MMTK, [1])
    GC_LIBS=MMTk
    GC_FLAGS="-I\$(PROJ_SRC_ROOT)/lib/Mvm/MMTk -DWITH_MMTK -I\$(PROJ_SRC_ROOT)/mmtk/config/\$(MMTK_PLAN)"
  else
    GC_LIBS=GCMmap2
    if test "x$gc" = "xmulti-mmap"; then
      GC_FLAGS="-I\$(PROJ_SRC_ROOT)/lib/Mvm/GCMmap2 -DWITH_TRACER -DMULTIPLE_GC -I\$(PROJ_SRC_ROOT)/lib/Mvm/Allocator"
      AC_SUBST([GC_MULTI_MMAP], [1])
      AC_SUBST([GC_SINGLE_MMAP], [0])
    else
      GC_FLAGS="-I\$(PROJ_SRC_ROOT)/lib/Mvm/GCMmap2 -DWITH_TRACER -I\$(PROJ_SRC_ROOT)/lib/Mvm/Allocator"
      AC_SUBST([GC_MULTI_MMAP], [0])
      AC_SUBST([GC_SINGLE_MMAP], [1])
    fi
    AC_DEFINE([USE_GC_MMAP2], [1], [Using the gcmmap2])
    AC_SUBST(GC_MMAP2, [1])
    AC_SUBST(GC_BOEHM, [0])
    AC_SUBST(GC_MMTK, [0])
  fi
fi
  
AC_SUBST([GC_FLAGS])
AC_SUBST([GC_LIBS])
AC_SUBST([MMTK_PLAN])

dnl **************************************************************************
dnl Virtual Machine type
dnl **************************************************************************
AC_ARG_WITH(vm-type,
       [AS_HELP_STRING(--with-vm-type=something,
           [VM type ('single' 'isolate' 'isolate-sharing' or 'service')])],
       [[vmtype=$withval]],
       [[ echo Using single as vm type.
                vmtype=single
       ]]
)

if test "x$vmtype" = "xisolate";  then
  VM_FLAGS="-DISOLATE"
  ISOLATE_BUILD=1
  SERVICE_BUILD=0
  SINGLE_BUILD=0
else 
  if test "x$vmtype" = "xservice"; then
    VM_FLAGS="-DISOLATE -DSERVICE"
    SERVICE_BUILD=1
    ISOLATE_BUILD=1
    SINGLE_BUILD=0
  else
    SINGLE_BUILD=1
    ISOLATE_BUILD=0
    SERVICE_BUILD=0
  fi
fi

AC_SUBST([SINGLE_BUILD])
AC_SUBST([SERVICE_BUILD])
AC_SUBST([ISOLATE_BUILD])
AC_SUBST([VM_FLAGS])

dnl **************************************************************************
dnl Exception type
dnl **************************************************************************
AC_ARG_WITH(exception-type,
       [AS_HELP_STRING(--with-exception-type=something,
           [Exception type ('check' or 'dwarf')])],
       [[exceptiontype=$withval]],
       [[ echo Using check as exception type.
                exceptiontype=check
       ]]
)

AC_ARG_WITH(runtime-exception-type,
       [AS_HELP_STRING(--with-runtime-exception-type=something,
           [Runtime exception type ('setjmp' or 'dwarf')])],
       [[runtimeexceptiontype=$withval]],
       [[ echo Using setjmp as exception type.
                exceptiontype=setjmp
       ]]
)

if test "x$exceptiontype" = "xdwarf";  then
  EXCEPTION_FLAGS="-DDWARF_EXCEPTIONS -fexceptions"
fi

if test "x$runtimeexceptiontype" = "xdwarf";  then
  EXCEPTION_FLAGS+="-DRUNTIME_DWARF_EXCEPTIONS -fexceptions"
fi

if test "x$runtimeexceptiontype" != "xdwarf"; then
  if test "x$exceptiontype" != "xdwarf"; then
    EXCEPTION_FLAGS="-fno-exceptions"
  fi
fi

AC_SUBST([EXCEPTION_FLAGS])

dnl **************************************************************************
dnl GNU CLASSPATH installation prefix
dnl **************************************************************************

classpathversion=0.97.2;

AC_ARG_WITH(gnu-classpath-libs,
       [AS_HELP_STRING(--with-gnu-classpath-libs=something,
           [GNU CLASSPATH libraries (default is /usr/lib/classpath)])],
       [[classpathlibs=$withval]],
       [[classpathlibs=/usr/lib/classpath]]
)

AC_ARG_WITH(gnu-classpath-glibj,
       [AS_HELP_STRING(--with-gnu-classpath-glibj,
           [Build J3 with GNU Classpath install (default is '/usr/share/classpath/glibj.zip')])],
       [[classpathglibj=$withval]],
       [[classpathglibj=/usr/share/classpath/glibj.zip]]
)

AC_ARG_WITH(j3,
       [AS_HELP_STRING(--with-j3=yes|no,
        [Build J3 (default is yes)])],
       [[WITH_J3=$withval]],
       [[WITH_J3=yes]]
)

classpathinclude="-I${classpathlibs}/../include -I/usr/include/classpath";

if test "x${WITH_J3}" = "xyes"; then
  WITH_J3=1;
  AC_DEFINE([WITH_J3], [1], [With J3])
else
  WITH_J3=0;
fi 

AC_SUBST([classpathglibj])
AC_SUBST([classpathlibs])
AC_SUBST([classpathinclude])
AC_SUBST([classpathversion])
AC_SUBST([WITH_J3])

dnl **************************************************************************
dnl Local PNet directory
dnl **************************************************************************
AC_ARG_WITH(pnet-local-prefix,
       [AS_HELP_STRING(--with-pnet-local-prefix=something,
           [PNET local prefix (no default)])],
       [[pnetlocalprefix=$withval]],
       [[       echo Not using PNETlocal prefix.    
                                pnetlocalprefix=''
       ]]
)

AC_ARG_WITH(pnetlib,
       [AS_HELP_STRING(--with-pnetlib=something,
           [Pnetlib's mscorlib.dll location (default is /usr/lib/cscc/lib/)])],
       [[pnetlibpath=$withval]],
       [[pnetlibpath=/usr/lib/cscc/lib/]]
)

WITH_N3=0

if test "x$pnetlocalprefix" != x; then
  echo Using ${pnetlocalprefix} as PNET local prefix;
  WITH_N3_PNETLIB=1;
  WITH_N3=1;
  AC_DEFINE([WITH_N3], [1], [With N3])
else
  WITH_N3_PNETLIB=0;
fi

AC_SUBST([pnetlocalprefix])
AC_SUBST([WITH_N3_PNETLIB])

N3_LIB=PNetLib
AC_SUBST([N3_LIB])
AC_SUBST([pnetlibpath])

AC_ARG_WITH(mono,
       [AS_HELP_STRING(--with-mono=something,
         [Mono's mscorlib.dll location (no default)])],
       [[monopath=$withval]],
       [[echo Not using Mono]]
)

if test "x$monopath" != x; then
  echo Building N3 with Mono;
  WITH_N3_MONO=1;
  WITH_N3=1;
else
  WITH_N3_MONO=0;
fi

AC_SUBST([WITH_N3_MONO])
AC_SUBST([monopath])

AC_SUBST([WITH_N3])

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 4: Check for programs we need and that they are the right version
dnl===
dnl===-----------------------------------------------------------------------===

dnl Check for compilation tools
AC_PROG_CPP
AC_PROG_CC(gcc)
AC_PROG_CXX(g++)

AC_PROG_NM
AC_SUBST(NM)

dnl Check for the tools that the makefiles require
AC_PROG_LN_S
AC_PATH_PROG(CMP, [cmp], [cmp])
AC_PATH_PROG(CP, [cp], [cp])
AC_PATH_PROG(DATE, [date], [date])
AC_PATH_PROG(FIND, [find], [find])
AC_PATH_PROG(GREP, [grep], [grep])
AC_PATH_PROG(MKDIR,[mkdir],[mkdir])
AC_PATH_PROG(MV,   [mv],   [mv])
AC_PROG_RANLIB
AC_PATH_PROG(RM,   [rm],   [rm])
AC_PATH_PROG(SED,  [sed],  [sed])
AC_PATH_PROG(TAR,  [tar],  [gtar])
AC_PATH_PROG(BINPWD,[pwd],  [pwd])
AC_PATH_PROG(CAT,[cat],  [cat])

AC_PATH_PROG(LLVMAS, [llvm-as],  [llvm-as])
AC_PATH_PROG(LLC, [llc],  [llc])

if test "x$gc" = "xmmtk";  then
  AC_PATH_PROG(ANT, [ant])
  if test -z "$ANT"; then
    AC_MSG_ERROR([Apache ANT required for MMTk, but not found])
  fi
fi

dnl Find the install program
AC_PROG_INSTALL

if test "$WITH_LLVMGCCDIR" = "default" ; then
  LLVMGCC="llvm-gcc${EXEEXT}"
  LLVMGXX="llvm-g++${EXEEXT}"
  AC_PATH_PROG(LLVMGCC, $LLVMGCC, [])
  AC_PATH_PROG(LLVMGXX, $LLVMGXX, [])
else
  if test -z "$LLVMGCC"; then
    LLVMGCC="$WITH_LLVMGCCDIR/bin/llvm-gcc${EXEEXT}"
  fi
  if test -z "$LLVMGXX"; then
    LLVMGXX="$WITH_LLVMGCCDIR/bin/llvm-g++${EXEEXT}"
  fi
  AC_SUBST(LLVMGCC,$LLVMGCC)
  AC_SUBST(LLVMGXX,$LLVMGXX)
fi

AC_MSG_CHECKING([tool compatibility])

dnl Ensure that compilation tools are GCC or a GNU compatible compiler such as
dnl ICC; we use GCC specific options in the makefiles so the compiler needs
dnl to support those options.
dnl "icc" emits gcc signatures
dnl "icc -no-gcc" emits no gcc signature BUT is still compatible
ICC=no
IXX=no
case $CC in
  icc*|icpc*)
    ICC=yes
    IXX=yes
    ;;
   *)
    ;;
esac

if test "$GCC" != "yes" && test "$ICC" != "yes"
then
  AC_MSG_ERROR([gcc|icc required but not found])
fi

dnl Ensure that compilation tools are GCC; we use GCC specific extensions
if test "$GXX" != "yes" && test "$IXX" != "yes"
then
  AC_MSG_ERROR([g++|icc required but not found])
fi

dnl Verify that GCC is version 3.0 or higher
if test "$GCC" = "yes"
then
  AC_COMPILE_IFELSE([[#if !defined(__GNUC__) || __GNUC__ < 3
#error Unsupported GCC version
#endif
]], [], [AC_MSG_ERROR([gcc 3.x required, but you have a lower version])])
fi

dnl Tool compatibility is okay if we make it here.
AC_MSG_RESULT([ok])


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 5: Check for libraries
dnl===
dnl===-----------------------------------------------------------------------===

AC_CHECK_LIB(z, inflate, [], \
  [AC_MSG_ERROR([You need to install the zlib package (z).])]
)
  
if test "x$gc" = "xboehm";  then
AC_CHECK_LIB(gc, GC_malloc, [], \
    [AC_MSG_ERROR([You need to install the boehm-gc package (gc).])]
)
fi

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 6: Check for header files
dnl===
dnl===-----------------------------------------------------------------------===

AC_CHECK_HEADER([zlib.h], [], \
  AC_MSG_ERROR([You need to install the zlib devel package (zlib.h).])
)
  
if test "x$gc" = "xboehm";  then
  AC_CHECK_HEADER([gc/gc.h], [], \
    AC_MSG_ERROR([You need to install the boehm-gc devel package (gc/gc.h).])
  )
fi


nl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 7: Check for types and structures
dnl===
dnl===-----------------------------------------------------------------------===

AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_CHECK_TYPES([int64_t],,AC_MSG_ERROR([Type int64_t required but not found]))
AC_CHECK_TYPES([uint64_t],,
         AC_CHECK_TYPES([u_int64_t],,
         AC_MSG_ERROR([Type uint64_t or u_int64_t required but not found])))

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 8: Check for specific functions needed
dnl===
dnl===-----------------------------------------------------------------------===

AC_CHECK_FUNCS([setjmp longjmp])


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
dnl WARNING: dnl If you add or remove any of the following config headers, then
dnl you MUST also update Makefile.rules so that the variable FilesToConfig
dnl contains the same list of files as AC_CONFIG_HEADERS below. This ensures the
dnl files can be updated automatically when their *.in sources change.
AC_CONFIG_HEADERS([include/mvm/Config/config.h])

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["Makefile.common.in"])

dnl Configure a common Makefile
AC_CONFIG_FILES(Makefile.common)
AC_CONFIG_FILES(Makefile.config)
AC_CONFIG_FILES([lib/J3/Classpath/Classpath.h])
AC_CONFIG_FILES([tools/llcj/LinkPaths.h])
AC_CONFIG_FILES([mmtk/java/build.xml])

dnl Do special configuration of Makefiles
AC_CONFIG_MAKEFILE(Makefile)
AC_CONFIG_MAKEFILE(lib/Makefile)


AC_OUTPUT
