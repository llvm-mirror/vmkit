<?php
include("root.php");
include($ROOT."common.php");
preambule("Overview", "overview");
?>

<p>
Current MREs are monolithic. Extending them to propose new
features or reusing them to execute new languages is difficult.
VMKit2 is a library that
eases the development of new MREs and the process of experimenting
with new mechanisms inside MREs. VMKit2
provides the basic components of MREs: a JIT compiler, a GC, 
and a thread manager. VMKit2 is a
fork of the <a href="http://vmkit.llvm.org">VMKit project</a>.

<p>
VMKit2 relies on <a href="http://llvm.org">LLVM</a> for compilation and 
<a href="http://jikesrvm.org/MMTk">MMTk</a> to manage memory. 
Currently, a full Java virtual machine

<p>
For the end user, VMKit2 provides:
<ul>
<li>Precise garbage collection.
<li>Just-in-Time and Ahead-of-Time compilation.
<li>Portable on many architectures (x86, x64, ppc32, ppc64, arm).
</ul>
<p>

For the MRE developer, VMKit2 provides:
<ul>
<li>Relatively small code base (~ 20k loc per VM)
<li>Infrastructure for virtual machine research and development
</ul>

<h2>Current Status</h2>
<p>
VMKit2 currently has a decent implementation of a JVM called J3. It executes large 
projects (e.g. OSGi Felix, Tomcat, Eclipse) and the DaCapo benchmarks. A R virtual machine
is currently under heavy development.

<p>
J3 has been tested on Linux/x64, Linux/x86, Linux/ppc32, MacOSX/x64, MacOSX/x86, MacOSX/ppc32. The JVM may work on ppc64. 
Support for Windows has not been investigated.

<p>
While this work aims to provide a fully functional JVM, it is still early work and is under heavy development.
Some of the common missing pieces in vmkit2/llvm are:
<ul>
<li>Mixed interpretation/compilation.
<li>Adaptive optimization.
</ul>


<?php epilogue() ?>
