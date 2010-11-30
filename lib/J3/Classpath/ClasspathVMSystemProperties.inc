//===- ClasspathVMSystem/Properties.cpp -----------------------------------===//
//===--------------------- GNU classpath gnu/classpath/VMSystemProperties -===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sys/utsname.h>

#include "types.h"

#include "Classpath.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"

using namespace j3;

extern "C" {

void setProperty(Jnjvm* vm, JavaObject* prop, const char* key,
                 const char* val) {

  JavaString* Val = 0; 
  JavaString* Key = 0; 
  llvm_gcroot(prop, 0);
  llvm_gcroot(Val, 0);
  llvm_gcroot(Key, 0);

  Val = vm->asciizToStr(val);
  Key = vm->asciizToStr(key);
                        
  vm->upcalls->setProperty->invokeIntSpecial(
      vm, (UserClass*)JavaObject::getClass(prop), prop, &Key, &Val);
}

void setUnameProp(Jnjvm* vm, JavaObject* prop) {

  llvm_gcroot(prop, 0);

  struct utsname infos;
  uname(&infos);
  setProperty(vm, prop, "os.name", infos.sysname);
  setProperty(vm, prop, "os.arch", infos.machine);
  setProperty(vm, prop, "os.version", infos.release);
  if (!strcmp(infos.machine, "ppc")) {
    setProperty(vm, prop, "gnu.cpu.endian","big");
  } else {
    setProperty(vm, prop, "gnu.cpu.endian","little");
  }
}

JNIEXPORT void JNICALL Java_gnu_classpath_VMSystemProperties_preInit(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
JavaObject* prop) {
  
  llvm_gcroot(prop, 0);
  
  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  const char* tmp;
  setProperty(vm, prop, "java.vm.specification.version", "1.0");
  setProperty(vm, prop, "java.vm.specification.vendor",
              "Sun Microsystems, Inc");
  setProperty(vm, prop, "java.vm.specification.name",
              "Java Virtual Machine Specification");
  setProperty(vm, prop, "java.specification.version", "1.5");
  setProperty(vm, prop, "java.specification.vendor", "Sun Microsystems, Inc");
  setProperty(vm, prop, "java.specification.name",
              "Java Platform API Specification");
  setProperty(vm, prop, "java.version", "1.5");
  setProperty(vm, prop, "java.runtime.version", "1.5");
  setProperty(vm, prop, "java.vendor", "The VMKit Project");
  setProperty(vm, prop, "java.vendor.url", "http://vmkit.llvm.org");
  
  tmp = getenv("JAVA_HOME");
  if (!tmp) tmp = "";
  setProperty(vm, prop, "java.home", tmp);
  
  JnjvmBootstrapLoader* JCL = vm->bootstrapLoader;
  setProperty(vm, prop, "java.class.version", "49.0");
  setProperty(vm, prop, "java.class.path", vm->classpath);
  setProperty(vm, prop, "java.boot.class.path", JCL->bootClasspathEnv);
  setProperty(vm, prop, "sun.boot.class.path", JCL->bootClasspathEnv);
  setProperty(vm, prop, "java.vm.version", "0.28");
  setProperty(vm, prop, "java.vm.vendor", "The VMKit Project");
  setProperty(vm, prop, "java.vm.name", "J3");
  setProperty(vm, prop, "java.specification.version", "1.5");
  setProperty(vm, prop, "java.io.tmpdir", "/tmp");
  
  tmp = getenv("JAVA_COMPILER");
  if (!tmp) tmp = "gcj";
  setProperty(vm, prop, "java.compiler", tmp);
  
  setProperty(vm, prop, "build.compiler", "gcj");
  setProperty(vm, prop, "gcj.class.path", JCL->bootClasspathEnv);
  setProperty(vm, prop, "gnu.classpath.boot.library.path",
              JCL->libClasspathEnv);
  
  setUnameProp(vm, prop);
  
  setProperty(vm, prop, "file.separator", vm->dirSeparator);
  setProperty(vm, prop, "path.separator", vm->envSeparator);
  setProperty(vm, prop, "line.separator", "\n");
  
  tmp = getenv("USERNAME");
  if (!tmp) tmp = getenv("LOGNAME");
  else if (!tmp) tmp = getenv("NAME");
  else if (!tmp) tmp = "";
  setProperty(vm, prop, "user.name", tmp);
  
  tmp  = getenv("HOME");
  if (!tmp) tmp = "";
  setProperty(vm, prop, "user.home", tmp);
  
  tmp = getenv("PWD");
  if (!tmp) tmp = "";
  setProperty(vm, prop, "user.dir", tmp);
 
  // Disable this property. The Classpath iconv implementation is really
  // not optimized (it over-abuses JNI calls).
  //setProperty(vm, prop, "gnu.classpath.nio.charset.provider.iconv", "true"); 
  setProperty(vm, prop, "file.encoding", "ISO8859_1");
  setProperty(vm, prop, "gnu.java.util.zoneinfo.dir", "/usr/share/zoneinfo");

  END_NATIVE_EXCEPTION
}

extern "C" void Java_gnu_classpath_VMSystemProperties_postInit__Ljava_util_Properties_2(JavaObject* prop) {

  llvm_gcroot(prop, 0);

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  for (std::vector<std::pair<char*, char*> >::iterator i = 
       vm->postProperties.begin(), e = vm->postProperties.end(); i!= e; i++) {
    setProperty(vm, prop, i->first, i->second);
  }

  END_NATIVE_EXCEPTION
}

}
