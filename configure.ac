dnl Process this file with autoconf to produce a configure script

AC_INIT([vmkit],[0.0.1])
AC_PREREQ([2.59])
AC_CONFIG_SRCDIR([lib/Mvm/Main.cpp])


dnl **************************************************************************
dnl configure date, in version.cc.in
dnl **************************************************************************
configure_date=`date '+%Y-%m-%d %H:%M:%S'`
AC_SUBST([configure_date])

dnl **************************************************************************
dnl Initialize target_cpu, target_os etc ...
dnl **************************************************************************
AC_CANONICAL_TARGET

dnl **************************************************************************
dnl Add some vars
dnl **************************************************************************
AM_CONDITIONAL(ARCH_IS_PPC,   [test x"$target_cpu" = xpowerpc])
AM_CONDITIONAL(ARCH_IS_I686,  [test x"$target_cpu" = xi686])

case $host in
        *linux*) is_linux=true;;
        *) is_linux=false;;
esac
AM_CONDITIONAL(IS_LINUX,      [test x$is_linux = xtrue])

dnl **************************************************************************
dnl Initialize automake with a package version 
dnl **************************************************************************
AM_INIT_AUTOMAKE

dnl Use config.h instad of -D macros
AM_CONFIG_HEADER(config.h)

dnl **************************************************************************
dnl LLVM Installation Prefix 
dnl **************************************************************************
AC_ARG_WITH(llvm,
       [AS_HELP_STRING(--with-llvm=something,
           [LLVM installation  prefix (default is /usr/local/)])],
       [[llvmprefix=$withval]],
       [[ echo Using /usr/local/ as llvm installation prefix.
        	llvmprefix=/usr/local/
       ]]
)
LLVMDYLIB="`$llvmprefix/Release/bin/llvm-config --ldflags all` `$llvmprefix/Release/bin/llvm-config --libs all`"

dnl Force some compilation flags
CXXFLAGS="$CXXFLAGS -fsigned-char -felide-constructors -fno-keep-static-consts -D_REENTRANT -I$PWD/include -I$llvmprefix/include  -D__STDC_LIMIT_MACROS -rdynamic"
CFLAGS="$CFLAGS -D_REENTRANT"


dnl **************************************************************************
dnl Checks for programs.
dnl **************************************************************************
AC_PROG_CXX
AM_PROG_CC_C_O

dnl **************************************************************************
dnl check for lex and yacc or bison 
dnl **************************************************************************
AM_PROG_LEX
AC_PROG_YACC

dnl **************************************************************************
dnl check for as
dnl **************************************************************************
AM_PROG_AS

dnl **************************************************************************
dnl Checks for header files.
dnl **************************************************************************
AC_HEADER_STDC

dnl **************************************************************************
dnl Checks for header files.
dnl **************************************************************************
AC_HEADER_STDC

case $target_os in
  *linux*)
  dnl libiberty, libopcode and libbfd are part of binutils
  AC_CHECK_HEADER([bfd.h], [], \
    AC_MSG_ERROR([You need to install the binutils devel package (bfd.h).])
  )
  AC_CHECK_HEADER([dis-asm.h], [], \
    AC_MSG_ERROR([You need to install the binutils devel package (dis-asm.h).])
  )
  ;;
esac

AC_CHECK_HEADER([zlib.h], [], \
  AC_MSG_ERROR([You need to install the zlib devel package (zlib.h).])
)

dnl **************************************************************************
dnl Checks for libraries    
dnl **************************************************************************
case $target_os in
  *darwin*)
    DYLIB_EXTENSION="dylib"
    rdynamic=""
    ;;
  *linux*)
    AC_CHECK_LIB(iberty, xexit, [], \
      [AC_MSG_ERROR([You need to install the binutils package (iberty).])]
    )
    AC_CHECK_LIB(bfd, bfd_get_arch, [], \
      [AC_MSG_ERROR([You need to install the binutils package (bfd).])],
    -liberty
    )
    AC_CHECK_LIB(opcodes, disassembler, [], \
      [AC_MSG_ERROR([You need to install the binutils package (opcodes).])],
    -lbfd -liberty
    )
    
    rdynamic="-rdynamic"
    DYLIB_EXTENSION="so"
    AC_DEFINE([HAVE_DISASSEMBLER], [1], [Using libopcodes])

    ;;
  *) AC_MSG_ERROR([$target_os target is not supported.]);;
esac

AC_CHECK_LIB(z, inflate, [], \
  [AC_MSG_ERROR([You need to install the zlib package (z).])]
)

AC_SUBST([rdynamic])
AC_SUBST([LLVMDYLIB])
AC_SUBST([DYLIB_EXTENSION])

dnl **************************************************************************
dnl VVM thread type
dnl **************************************************************************
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

AM_CONDITIONAL([HAVE_PTHREAD], [test "x$thread" != "xno"])
if test "x$thread" = xcommon; then
  AC_DEFINE([HAVE_PTHREAD], [1], [Using pthread library])
fi

GCTHREAD_LIBS="$PWD/lib/Mvm/CommonThread/libuvm_common_thread.a"
AC_SUBST([GCTHREAD_LIBS])


dnl **************************************************************************
dnl Checks for typedefs, structures, and compiler characteristics.
dnl **************************************************************************
AC_C_CONST

dnl **************************************************************************
dnl
dnl **************************************************************************
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL


dnl **************************************************************************
dnl VVM GC type 
dnl **************************************************************************
AC_ARG_WITH(gc,
       [AS_HELP_STRING(--with-gc=something,
           [GC type ('single-mmap' 'multi-mmap' or 'boehm')])],
       [[gc=$withval]],
       [[ echo Using mmap2 as vvm gc type.
                gc=single-mmap
       ]]
)

dnl TODO find the libgc.a
if test "x$gc" = "xboehm";  then
  AC_CHECK_LIB(gccpp, GC_malloc, [], \
    [AC_MSG_ERROR([You need to install the boehm-gc package (gccpp).])]
  )
  AC_CHECK_HEADER([gc/gc.h], [], \
    AC_MSG_ERROR([You need to install the boehm-gc devel package (gc/gc.h).])
  )
  GC_LIBS="-lgccpp -gc"
  CFLAGS="$CFLAGS -I$PWD/lib/Mvm/BoehmGC -DGC_THREADS"
  CXXFLAGS="$CXXFLAGS -I$PWD/lib/Mvm/BoehmGC -DGC_THREADS"
  AC_DEFINE([USE_GC_BOEHM], [1], [Using the boehm gc])
  case $target_os in
    *linux*)
      CFLAGS="-DGC_LINUX_THREADS"
      CXXFLAGS="$CXXFLAGS -DGC_LINUX_THREADS"
    ;;
  esac
else
  if test "x$gc" = "xmulti-mmap"; then
    CFLAGS="$CFLAGS -I$PWD/lib/Mvm/GCMmap2 -DWITH_TRACER -DMULTIPLE_GC"
    CXXFLAGS="$CXXFLAGS -I$PWD/lib/Mvm/GCMmap2 -DWITH_TRACER -DMULTIPLE_GC"
  fi
  GC_LIBS="$PWD/lib/Mvm/GCMmap2/libuvm_gc_mmap2.a \
           $PWD/lib/Mvm/Allocator/libuvm_alloc.a"
  CFLAGS="$CFLAGS -I$PWD/lib/Mvm/GCMmap2 -DWITH_TRACER"
  CXXFLAGS="$CXXFLAGS -I$PWD/lib/Mvm/GCMmap2 -DWITH_TRACER"
  AC_DEFINE([USE_GC_MMAP2], [1], [Using the gcmmap2])
fi

AC_SUBST([GC_LIBS])

dnl **************************************************************************
dnl Virtual Machine type
dnl **************************************************************************
AC_ARG_WITH(vm-type,
       [AS_HELP_STRING(--with-vm-type=something,
           [VM type ('single' 'multi' or 'service')])],
       [[vmtype=$withval]],
       [[ echo Using single as vm type.
                vmtype=single
       ]]
)

if test "x$vmtype" = "xmulti";  then
  CFLAGS="$CFLAGS -DMULTIPLE_VM"
  CXXFLAGS="$CXXFLAGS -DMULTIPLE_VM"
else 
  if test "x$vmtype" = "xservice"; then
    CFLAGS="$CFLAGS -DMULTIPLE_VM -DSERVICE_GC -DMULTIPLE_GC -DSERVICE_VM"
    CXXFLAGS="$CXXFLAGS -DMULTIPLE_VM -DSERVICE_GC -DMULTIPLE_GC -DSERVICE_VM"
  fi
fi
  
dnl **************************************************************************
dnl GNU CLASSPATH version
dnl **************************************************************************
AC_ARG_WITH(gnu-classpath-version,
       [AS_HELP_STRING(--with-gnu-classpath-version=something,
           [GNU CLASSPATH VERSION (default is '0.97.1')])],
       [[gnuclasspathversion=$withval]],
       [[       echo Using '0.97.1' as GNU CLASSPATH version.    
                                gnuclasspathversion=0.97.1
       ]]
)
gnuclasspathversionuvm=`echo $gnuclasspathversion | $SED s/\\\./-/`
AC_SUBST([gnuclasspathversion])
AC_SUBST([gnuclasspathversionuvm])

dnl **************************************************************************
dnl GNU CLASSPATH installation prefix
dnl **************************************************************************
AC_ARG_WITH(gnu-classpath-local-prefix,
       [AS_HELP_STRING(--with-gnu-classpath-local-prefix=something,
           [GNU CLASSPATH local prefix (no default)])],
       [[gnuclasspathlocalprefix=$withval]],
       [[       echo Not using GNU CLASSPATH local prefix.    
                                gnuclasspathlocalprefix=''
       ]]
)

AC_ARG_WITH(gnu-classpath-installation-prefix,
       [AS_HELP_STRING(--with-gnu-classpath-installation-prefix=something,
           [GNU CLASSPATH installation prefix (default is '/usr/local/classpath')])],
       [[gnuclasspathinstallationprefix=$withval]],
       [[gnuclasspathinstallationprefix=/usr/local/classpath]]
)

if test "x${gnuclasspathlocalprefix}" = x; then
  echo Using ${gnuclasspathinstallationprefix} as GNU CLASSPATH installation prefix;
  classpathglibj=${gnuclasspathinstallationprefix}/share/classpath/glibj.zip;
  classpathlibs=${gnuclasspathinstallationprefix}/lib/classpath/;
  CFLAGS="$CFLAGS -I$gnuclasspathinstallationprefix/include"
  CXXFLAGS="$CXXFLAGS -I$gnuclasspathinstallationprefix/include"
else
  echo Using ${gnuclasspathlocalprefix} as GNU CLASSPATH local prefix;
  classpathglibj=${gnuclasspathlocalprefix}/lib/;
  classpathlibs=${gnuclasspathlocalprefix}/lib/;
  CFLAGS="$CFLAGS -I$gnuclasspathlocalprefix/include"
  CXXFLAGS="$CXXFLAGS -I$gnuclasspathlocalprefix/include"
fi


AC_SUBST([classpathglibj])
AC_SUBST([classpathlibs])



AC_CONFIG_FILES([
  Makefile
  lib/N3/Makefile
  lib/N3/VMCore/Makefile
  lib/JnJVM/Makefile
  lib/JnJVM/Classpath/Makefile
  lib/JnJVM/VMCore/Makefile
  lib/JnJVM/Classpath/Classpath.h
  lib/Mvm/Makefile
  lib/Mvm/GCMmap2/Makefile
  lib/Mvm/CommonThread/Makefile
  lib/Mvm/Allocator/Makefile
])
AC_OUTPUT
