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

* Get the compatible release LLVM 3.3:

wget http://llvm.org/releases/3.3/llvm-3.3.src.tar.gz
tar zxf llvm-3.3.src.tar.gz
cd llvm-3.3.src/tools
wget http://llvm.org/releases/3.3/cfe-3.3.src.tar.gz
tar zxf cfe-3.3.src.tar.gz
mv cfe-3.3.src clang
cd ../projects
wget http://llvm.org/releases/3.3/compiler-rt-3.3.src.tar.gz
tar zxf compiler-rt-3.3.src.tar.gz
mv compiler-rt-3.3.src compiler-rt
cd ../..

* Patch LLVM if you want to have a better debug support for VMKit:
  You will have to get VMKit (see the next section) to find the patch.

cd llvm-3.3.src
patch -p1 < ../vmkit2/patches/llvm.patch

NOTICE:
* VMKit does not support building LLVM out of source because of a bug
  in "llvm-config", so please build in the source, and keep different source
  trees for different configurations/architectures of builds.

* Make it (release mode):

./configure --enable-optimized --enable-assertions --enable-jit --enable-cxx11 --enable-threads --enable-pthreads --enable-pic --enable-targets=host,cpp --enable-zlib --disable-docs --disable-doxygen
make -j12 BUILD_EXAMPLES=0 ENABLE_DOCS=0

* You can also make it in debug mode, but this is is not necessary to debug compiled programs:

./configure --disable-optimized --enable-debug-runtime --enable-debug-symbols --enable-keep-symbols --enable-assertions --enable-jit --enable-cxx11 --enable-threads --enable-pthreads --enable-pic --enable-targets=host,cpp --enable-zlib --disable-docs --disable-doxygen

* Build it:

make -j12 BUILD_EXAMPLES=0 ENABLE_DOCS=0

======
VMKit:
======

* Get it anonymously:

svn co http://llvm.org/svn/llvm-project/vmkit/trunk

* or get as an official developer:

svn co https://YOUR_USER_NAME@llvm.org/svn/llvm-project/vmkit/trunk

* Make it (release mode):

./configure --with-llvm-config-path=@PATH_TO_LLVM@/Release+Asserts/bin/llvm-config --with-gnu-classpath-libs=@PATH_TO_GNUCLASSPATH@/lib --with-gnu-classpath-glibj=@PATH_TO_GNUCLASSPATH@/lib/glibj.zip

* Or you can make it in debug mode:

./configure --with-llvm-config-path=@PATH_TO_LLVM@/Release+Asserts/bin/llvm-config --with-gnu-classpath-libs=@PATH_TO_GNUCLASSPATH@/lib --with-gnu-classpath-glibj=@PATH_TO_GNUCLASSPATH@/lib/glibj.zip --enable-assert=yes --disable-optimized --enable-debug=yes

* Or you can enable OSGi Incinerator build:

./configure --with-llvm-config-path=@PATH_TO_LLVM@/Release+Asserts/bin/llvm-config --with-gnu-classpath-libs=@PATH_TO_GNUCLASSPATH@/lib --with-gnu-classpath-glibj=@PATH_TO_GNUCLASSPATH@/lib/glibj.zip --enable-incinerator=yes

* Build it:

make -j12

