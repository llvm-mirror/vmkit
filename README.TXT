=============
Requirements:
=============

* Install required packages (Debian):
apt-get install ant libgconf2-dev libgtk2.0-dev sharutils

* Install required packages (SUSE):
zypper install ant gconf2-devel gtk2-devel

* If you want to use OpenJDK:
apt-get install default-jre default-jdk

==============
GNU Classpath:
==============

* For Macos users
sudo port install antlr
wget http://ftp.gnu.org/gnu/classpath/classpath-0.99.tar.gz
tar zxf classpath-0.99.tar.gz
patch -p1 < patches/classpath-0.99.macos.patch
cd classpath-0.99
./configure --disable-plugin --disable-examples --disable-Werror --with-antlr-jar=/opt//local/share/java/antlr.jar
make -j12
cd lib
ln -s ../native/jni/gtk-peer/.libs/libgtkpeer.dylib;
ln -s ../native/jni/gconf-peer/.libs/libgconfpeer.dylib;
ln -s ../native/jni/java-io/.libs/libjavaio.dylib;
ln -s ../native/jni/java-lang/.libs/libjavalangreflect.dylib;
ln -s ../native/jni/java-lang/.libs/libjavalang.dylib;
ln -s ../native/jni/java-net/.libs/libjavanet.dylib;
ln -s ../native/jni/java-nio/.libs/libjavanio.dylib;
ln -s ../native/jni/java-util/.libs/libjavautil.dylib;

* For Linux users:
wget http://ftp.gnu.org/gnu/classpath/classpath-0.99.tar.gz
tar zxf classpath-0.99.tar.gz
cd classpath-0.99
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

=====
LLVM:
=====

* Notice that installing clang is not mandatory on a recent MACOS

* Get it (git):

git clone http://llvm.org/git/llvm.git
cd llvm/tools
git clone http://llvm.org/git/clang.git
cd clang
git config branch.master.rebase true
cd ../..
git config branch.master.rebase true

* You can also get it via svn:

svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
cd llvm/tools
svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
cd ../projects
svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt
cd ..

* Patch it if you want to have a better debug support:

cd llvm
patch -p1 < ../vmkit2/patches/llvm.patch

* Make it (release mode):

./configure --enable-optimized --enable-assertions --enable-jit --enable-threads --enable-pthreads --enable-pic --enable-targets=x86,cpp --disable-docs --disable-doxygen
make -j12 BUILD_EXAMPLES=0 ENABLE_DOCS=0

* You can also make it in debug mode, but this is is not necessary to debug compiled programs:

./configure --disable-optimized --enable-assertions --enable-debug-runtime --enable-debug-symbols --enable-keep-symbols --enable-jit --enable-threads --enable-pthreads --enable-pic --enable-targets=x86,cpp --disable-docs --disable-doxygen

make -j12 BUILD_EXAMPLES=0 ENABLE_DOCS=0

======
VMKit:
======

* Get it (git):

git clone git://scm.gforge.inria.fr/vmkit2/vmkit2.git

* or

git clone git+ssh://YOUR_USER_NAME@gforge.inria.fr//gitroot/vmkit2/vmkit2.git

* Make it (debug mode):

./configure --with-llvmsrc=@PATH_TO_LLVM@ --with-llvmobj=@PATH_TO_LLVM@ --with-gnu-classpath-libs=@PATH_TO_GNUCLASSPATH@ --with-gnu-classpath-glibj=@PATH_TO_GNUCLASSPATH@/lib/glibj.zip

* Buil it:
make -j12

