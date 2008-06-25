//===---------------------------------------------------------------------===//
// General notes
//===---------------------------------------------------------------------===//

VMKit is the composition of three libraries:
1) MVM: mostly a garbage collector
2) JnJVM: a Java Virtual Machine implemented with MVM and LLVM
3) N3: a CLI implementation with MVM and LLVM

JnJVM and N3 work on Linux/x86 (note that you may have to disable SSE on some 
architecture), and mostly work on Linux/PPC (there are some errors with
floating points). Portage to Darwin needs to investigate why exceptions
do not work.

JnJVM and N3 use GCC's unwinding library (libgcc_s.so).

There are mainly two options in the ./configure script
--with-gnu-classpath-local-prefix: the local build of GNU classpath
--with-pnet-local-prefix: the local build of PNET
--with-llvmsrc: the local build of LLVM
--with-gc: user either boehm or single-mmap

Running make on the root tree will produce two "tools":
1) Debug|Release/bin/jnjvm: running the JnJVM like any other JVM
2) Debug|Release/bin/n3: running N3 like CLR

JnJVM and N3 have their own README notes.


//===---------------------------------------------------------------------===//
// Target specific LLVM patches (not required)
//===---------------------------------------------------------------------===//

If you experience problems, you can apply the patches for your corresponding 
arch. I'd be interested to get some info on your specific problem if the 
patch fixes it.

Patch for PowerPC:
- Bugfix for handling some external symbols (FIXME: does this still happen?)
- Thread safety when doing native code patching (TODO: submit to llvm-dev)
- CFI instructions for callbacks (TODO: find a correct macro to enable this)
- Emit a hand-made frame table for stubs (TODO: submit to llvm-dev)

Patch for X86
- Disable SSE (It can sigsegv on some x86 archs. Having test cases would be 
  nice)
- Enable CFI instructions for callbacks (TODO: find a correct macro to enable
  by default)


//===---------------------------------------------------------------------===//
// TODOs
//===---------------------------------------------------------------------===//

- A compiler/system dependency interface
- Port to Darwin
