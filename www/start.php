<?php
include("root.php");
include($ROOT."common.php");
preambule("Get Started", "start");
?>

<p>If you would like to check out and build the project, the current scheme
is:</p>

<ol>
  <li>Checkout <a href="http://www.llvm.org/docs/GettingStarted.html#checkout">
    LLVM</a> and 
    <a href="http://clang.llvm.org/get_started.html">Clang</a> from SVN head.
  </li>

  <ul>
    <li><tt>svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm</tt></li>
    <li><tt>cd llvm/tools</li></tt>
    <li><tt>svn co http://llvm.org/svn/llvm-project/cfe/trunk clang</tt></li>
    <li><tt>cd ..</tt></li>
    <li><tt>./configure; make ENABLE_OPTIMIZED=1</tt></li>
  </ul>

  
  <li><a href="ftp://ftp.gnu.org/gnu/classpath/classpath-0.97.2.tar.gz">Download
   GNU Classpath 0.97.2</a>:</li>

  <ul>
    <li><tt>tar zxvf classpath-0.97.2.tar.gz</tt></li>
    <li><tt>cd classpath-0.97.2</tt></li>
    <li><tt>./configure --disable-plugin --disable-examples --disable-Werror; make</tt></li>
    <li><tt>cd lib</li></tt>
    <li><tt>If you are running on Linux:</li></tt>
    <ul>
      <li><tt>ln -s ../native/jni/gtk-peer/.libs/libgtkpeer.so;</li></tt>
      <li><tt>ln -s ../native/jni/gconf-peer/.libs/libgconfpeer.so;</li></tt>
      <li><tt>ln -s ../native/jni/java-io/.libs/libjavaio.so;</li></tt>
      <li><tt>ln -s ../native/jni/java-lang/.libs/libjavalangreflect.so;</li></tt>
      <li><tt>ln -s ../native/jni/java-lang/.libs/libjavalang.so;</li></tt>
      <li><tt>ln -s ../native/jni/java-net/.libs/libjavanet.so;</li></tt>
      <li><tt>ln -s ../native/jni/java-nio/.libs/libjavanio.so;</li></tt>
      <li><tt>ln -s ../native/jni/java-util/.libs/libjavautil.so;</li></tt>
    </ul>
    <li><tt>If you are running on MacOS:</li></tt>
    <ul>
      <li><tt>ln -s ../native/jni/gtk-peer/.libs/libgtkpeer.dylib;</li></tt>
      <li><tt>ln -s ../native/jni/gconf-peer/.libs/libgconfpeer.dylib;</li></tt>
      <li><tt>ln -s ../native/jni/java-io/.libs/libjavaio.dylib;</li></tt>
      <li><tt>ln -s ../native/jni/java-lang/.libs/libjavalangreflect.dylib;</li></tt>
      <li><tt>ln -s ../native/jni/java-lang/.libs/libjavalang.dylib;</li></tt>
      <li><tt>ln -s ../native/jni/java-net/.libs/libjavanet.dylib;</li></tt>
      <li><tt>ln -s ../native/jni/java-nio/.libs/libjavanio.dylib;</li></tt>
      <li><tt>ln -s ../native/jni/java-util/.libs/libjavautil.dylib;</li></tt>
    </ul>
  </ul>
  
  <li>Checkout vmkit2:</li>
  <ul>
     <li><tt>git clone git://scm.gforge.inria.fr/vmkit2/vmkit2.git</tt></li>

  </ul>
  <li>Configure vmkit:</li>
  <ul>
  <li><tt>./configure </tt></li>
  <dl>
    <dt><tt>--with-llvmsrc=&lt;directory&gt;</tt></dt>
    <dd>Tell vmkit where the LLVM source tree is located.</dd>
    <dt><br/><tt>--with-llvmobj=&lt;directory&gt;</tt></dt>
    <dd>Tell vmkit where the LLVM object tree is located.</dd>
    <dt><br/><tt>--with-gnu-classpath-glibj=&lt;file or directory&gt;</tt></dt>
    <dd>Tell vmkit where GNU Classpath glibj.zip is located.</dd>
    <dt><br/><tt>--with-gnu-classpath-libs=&lt;directory&gt;</tt></dt>
    <dd>Tell vmkit where GNU Classpath libs are located.</dd>
    <dt><br/><tt>--with-mmtk-plan=</tt> </dt>
      <dd>
      <ul>
      <li><tt>org.mmtk.plan.marksweep.MS (default)</tt></li>
      <li><tt>org.mmtk.plan.copyms.CopyMS</tt></li>
      <li><tt>org.mmtk.plan.semispace.SS</tt></li>
      <li><tt>org.mmtk.plan.immix.Immix</tt></li>
      <li><tt>org.mmtk.plan.generational.marksweep.GenMS</tt></li>
      <li><tt>org.mmtk.plan.generational.copying.GenCopy</tt></li>
      <li><tt>org.mmtk.plan.generational.immix.GenImmix</tt></li>
      </ul>
      </dd>
  </dl>

  </ul>

  <li>Build vmkit:</li>
  <ul>
    <li><tt>cd vmkit</tt></li>
    <li><tt>make ENABLE_OPTIMIZED=1</tt> (this will give you a release build)</li>
  </ul>

  <li>Try it out: (assuming vmkit/Release+Asserts/bin is in your path)</li>
  <ul>
    <li><tt>j3 --help</tt></li>
    <li><tt>j3 HelloWorld</tt></li>
  </ul>
</ol>


<?php epilogue() ?>
