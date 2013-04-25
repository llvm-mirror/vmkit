http://llvm.org/docs/GettingStarted.html#checkout

===================
Prepare the system:
===================
apt-get install libgconf2-dev libgtk2.0-dev

# To use OpenJDK:
# apt-get install default-jre default-jdk

==============
GNU Classpath:
==============
wget http://ftp.gnu.org/gnu/classpath/classpath-0.97.2.tar.gz
tar zxvf classpath-0.97.2.tar.gz
ln -s classpath-0.97.2 classpath
cd classpath
./configure --disable-plugin --disable-examples --disable-Werror
make -j12
cd lib
ln -sf ../native/jni/gtk-peer/.libs/libgtkpeer.so
ln -sf ../native/jni/gconf-peer/.libs/libgconfpeer.so
ln -sf ../native/jni/java-io/.libs/libjavaio.so
ln -sf ../native/jni/java-lang/.libs/libjavalangreflect.so
ln -sf ../native/jni/java-lang/.libs/libjavalang.so
ln -sf ../native/jni/java-net/.libs/libjavanet.so
ln -sf ../native/jni/java-nio/.libs/libjavanio.so
ln -sf ../native/jni/java-util/.libs/libjavautil.so

# Go root
# make install

=====
LLVM:
=====

Notice:

LLVM revision 158308 is known to work.

Get it (git):

git clone http://llvm.org/git/llvm.git
cd llvm/tools
git clone http://llvm.org/git/clang.git
cd clang
git config branch.master.rebase true
cd ../..
git config branch.master.rebase true

Get it (svn):

svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
cd llvm/tools
svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
cd ../projects
svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt
cd ..

Patch it:

cd llvm
patch -p0 < ../llvm.patch

Make it (release mode):

./configure --enable-optimized --enable-assertions --enable-jit --enable-threads --enable-pthreads --enable-pic --enable-targets=x86,cpp --disable-docs --disable-doxygen
make -j12 ENABLE_OPTIMIZED=1 BUILD_EXAMPLES=0 ENABLE_DOCS=0
ln -s Release+Asserts Debug+Asserts

Enable debug compilation:

nano Makefile.config
# Comment out:
# ENABLE_OPTIMIZED=1
# Uncomment:
# DEBUG_RUNTIME = 1
# DEBUG_SYMBOLS = 1
# Insert:
# KEEP_SYMBOLS=1

Make it (debug mode):

./configure --disable-optimized --enable-assertions --enable-debug-runtime --enable-debug-symbols --enable-keep-symbols --enable-jit --enable-threads --enable-pthreads --enable-pic --enable-targets=x86,cpp --disable-docs --disable-doxygen
make -j12 ENABLE_OPTIMIZED=0 BUILD_EXAMPLES=0 ENABLE_DOCS=0

======
VMKit:
======

Get it (svn):

svn co http://llvm.org/svn/llvm-project/vmkit/trunk vmkit

Get it (git):

git clone git+ssh://koutheir@scm.gforge.inria.fr//gitroot/vmkit2/vmkit2.git

Make it (debug mode):

./configure --with-llvmsrc=/home/koutheir/PhD/VMKit/llvm --with-llvmobj=/home/koutheir/PhD/VMKit/llvm --with-gnu-classpath-libs=/home/koutheir/PhD/VMKit/classpath/lib --with-gnu-classpath-glibj=/usr/local/classpath/share/classpath/glibj.zip
make -j12 ENABLE_OPTIMIZED=0 REQUIRES_FRAME_POINTER=1 KEEP_SYMBOLS=1 DEBUG_RUNTIME=1 DEBUG_SYMBOLS=1
