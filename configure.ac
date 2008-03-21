dnl Process this file with autoconf to produce a configure script

AC_INIT([micro-vm],[0.0.1])
AC_PREREQ([2.59])
AC_CONFIG_SRCDIR([lib/Mvm/Main.cc])


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

echo $host
dnl **************************************************************************
dnl Check system name
dnl **************************************************************************
case $host in
	*linux*) dlsyms_name='dlsyms';;
	*darwin*) dlsyms_name='dlsyms';;
	*) dlsyms_name='dlsyms-unknow';;
esac
AC_SUBST([dlsyms_name])

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

AC_ARG_WITH(readline,
  [AS_HELP_STRING([--without-readline],
    [disable readline support])],
    [with_readline=$enableval],[with_readline=yes]
)

AS_IF([test "x$with_readline" = xyes],
  [AC_CHECK_HEADER([readline/readline.h],
	  [AC_DEFINE(HAVE_READLINE_H,1,[readline found])],
	  [AC_MSG_WARN(readline NOT found)])
    AC_CHECK_LIB(readline, history_truncate_file, [],
     [AC_MSG_ERROR([You need to install readline version 5.])])
    ]
)

AC_ARG_WITH(bare,
  [AS_HELP_STRING([--with-bare], [bare VVMos support])],
    [with_bare=$enableval],[with_bare=no]
)

bare_header=""
AM_CONDITIONAL([BARE], [test "x$vvmthreadtype" != "xno"])
if test "x$with_bare" != xno; then
  AC_DEFINE([BARE], [1], [bare VVMos support])
  bare_header=-I$PWD/../bare/include/
fi

AC_SUBST([bare_header])
AC_SUBST([mksyms])
AC_SUBST([rdynamic])
AC_SUBST([LLVMDYLIB])


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
dnl VVM thread type
dnl **************************************************************************
AC_ARG_WITH(vvm-thread-type,
       [AS_HELP_STRING(--with-vvm-thread-type=something,
           [VVM Thread type ('common' or 'no')])],
       [vvmthreadtype=$withval],[vvmthreadtype=common]
)

AS_IF([test "x$vvmthreadtype" != "xno"],
  [AC_CHECK_HEADER([pthread.h],,
    [AC_MSG_WARN(phtread include NOT found)])
  AC_CHECK_LIB(pthread, pthread_create, [],
     [AC_MSG_ERROR([pthread library not found])])
  ]
)

AM_CONDITIONAL([HAVE_PTHREAD], [test "x$vvmthreadtype" != "xno"])
if test "x$vvmthreadtype" = xcommon; then
  AC_DEFINE([HAVE_PTHREAD], [1], [Using pthread library])
fi

AC_SUBST([vvmthreadtype])
GCTHREAD_SUBDIRS="$GCTHREAD_SUBDIRS $vvmthreadtype-thread"

dnl **************************************************************************
dnl VVM GC type 
dnl **************************************************************************
AC_ARG_WITH(vvm-gc-type,
       [AS_HELP_STRING(--with-vvm-gc-type=something,
           [VVM GC type ('gen2', 'mmap2' or 'simple2')])],
       [[vvmgctype=$withval]],
       [[ echo Using mmap2 as vvm gc type.
        	vvmgctype=mmap2
       ]]
)
AC_SUBST([vvmgctype])
GCTHREAD_SUBDIRS="$GCTHREAD_SUBDIRS gc-$vvmgctype"
AC_SUBST([GCTHREAD_SUBDIRS])

GC_DEFINE=""
GC_LIBS=""

dnl TODO find the libgc.a
if test "x$vvmgctype" = "xboehm";  then
  GC_DEFINE="-DGC_BOEHM"
  GC_LIBS="/usr/lib/libgc.a"
fi

AC_SUBST([GC_DEFINE])
AC_SUBST([GC_LIBS])

dnl **************************************************************************
dnl GNU CLASSPATH version
dnl **************************************************************************
AC_ARG_WITH(gnu-classpath-version,
       [AS_HELP_STRING(--with-gnu-classpath-version=something,
           [GNU CLASSPATH VERSION (default is '0.93')])],
       [[gnuclasspathversion=$withval]],
       [[       echo Using '0.93' as GNU CLASSPATH version.    
                                gnuclasspathversion=0.93
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
else
  echo Using ${gnuclasspathlocalprefix} as GNU CLASSPATH local prefix;
  classpathglibj=${gnuclasspathlocalprefix}/lib/;
  classpathlibs=${gnuclasspathlocalprefix}/lib/;
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
