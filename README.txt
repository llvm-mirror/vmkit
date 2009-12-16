//===---------------------------------------------------------------------===//
// General notes
//===---------------------------------------------------------------------===//

VMKit is the composition of three libraries:
1) MVM: mostly a garbage collector
2) J3: a Java Virtual Machine implemented with MVM and LLVM
3) N3: a CLI implementation with MVM and LLVM

J3 and N3 use GCC's unwinding library (libgcc_s.so).

These are the options you should pass to the ./configure script
--with-llvmsrc: the source directory of LLVM
--with-llvmobj: the object directory of LLVM
--with-gnu-classpath-libs: GNU classpath libraries
--with-gnu-classpath-glibj: GNU classpath glibj.zip
--with-pnet-local-prefix: the local build of PNET
--with-pnetlib: PNET's mscorlib.dll

There's also [experimental]:
--with-gc: user either boehm or single-mmap
--with-mono: Mono's mscorlib.dll

Running make on the root tree will produce the following "tools":
1) Debug|Release/bin/j3: running the J3 like any other JVM.
2) Debug|Release/bin/n3-pnetlib: running N3 like CLR.
2) Debug|Release/bin/vmkit: shell-like vm launcher.
2) Debug|Release/bin/vmjc: ahead of time compiler for .class files.

J3 and N3 have their own README notes.
