//===- ClasspathVMClassLoader.cpp - GNU classpath java/lang/VMClassLoader -===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "types.h"

#include "Classpath.h"
#include "JavaArray.h"
#include "JavaClass.h"
#include "JavaObject.h"
#include "JavaString.h"
#include "JavaThread.h"
#include "JavaUpcalls.h"
#include "Jnjvm.h"
#include "Reader.h"

using namespace jnjvm;

extern "C" {

JNIEXPORT jobject JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jchar byteId) {
  
  jobject res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  UserClassPrimitive* prim = 
    UserClassPrimitive::byteIdToPrimitive(byteId, vm->upcalls);
  if (!prim)
    vm->unknownError("unknown byte primitive %c", byteId);
  
  res = (jobject)prim->getClassDelegatee(vm);

  END_NATIVE_EXCEPTION

  return res;
  
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_findLoadedClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject loader, 
jobject _name) {
  
  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaString* name = (JavaString*)_name;
  const UTF8* utf8 = name->strToUTF8(vm);
  const UTF8* internalName = utf8->javaToInternal(vm, 0, utf8->size);
  JnjvmClassLoader* JCL = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)loader, vm);
  UserCommonClass* cl = JCL->lookupClass(internalName);

  if (cl) res = (jclass)(cl->getClassDelegatee(vm));

  END_NATIVE_EXCEPTION

  return res;
  
  return 0;
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_loadClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jobject _str, 
jboolean doResolve) {

  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
  JavaString* str = (JavaString*)_str;

  JnjvmClassLoader* JCL = vm->bootstrapLoader;
  UserCommonClass* cl = JCL->lookupClassFromJavaString(str, vm, doResolve,
                                                       false);

  if (cl != 0)
    res = (jclass)cl->getClassDelegatee(vm);
  
  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_defineClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz, 
#endif
jobject loader, 
jobject _str, 
jobject bytes, 
jint off, 
jint len, 
jobject pd) {
  
  jclass res = 0;

  BEGIN_NATIVE_EXCEPTION(0)

  Jnjvm* vm = JavaThread::get()->getJVM();
 
  // Before creating a class, do a check on the bytes.  
  Reader reader((ArrayUInt8*)bytes);
  uint32 magic = reader.readU4();
  if (magic != Jnjvm::Magic) {
    JavaThread::get()->getJVM()->classFormatError("bad magic number");
  }

  JnjvmClassLoader* JCL = 
    JnjvmClassLoader::getJnjvmLoaderFromJavaObject((JavaObject*)loader, vm);
  
  JavaString* str = (JavaString*)_str;
  const UTF8* name = str->value->javaToInternal(vm, str->offset, str->count);
  UserCommonClass* cl = JCL->lookupClass(name);
  
  if (!cl) {
    UserClass* cl = JCL->constructClass(name, (ArrayUInt8*)bytes);
    cl->resolveClass();

    res = (jclass)(cl->getClassDelegatee(vm, (JavaObject*)pd));
  } else {
    JavaObject* obj = vm->CreateLinkageError("duplicate class definition");
    JavaThread::get()->throwException(obj);
  }

  END_NATIVE_EXCEPTION

  return res;
}

JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(
#ifdef NATIVE_JNI
JNIEnv *env,
jclass clazz,
#endif
jclass Cl) {
  
  BEGIN_NATIVE_EXCEPTION(0)
  
  verifyNull(Cl);
  Jnjvm* vm = JavaThread::get()->getJVM();
  UserCommonClass::resolvedImplClass(vm, (JavaObject*)Cl, false);

  END_NATIVE_EXCEPTION
}

#define NUM_BOOT_PACKAGES 168

static const char* bootPackages[NUM_BOOT_PACKAGES] = {
				"java.applet",
				"java.awt",
				"java.awt.color",
				"java.awt.datatransfer",
				"java.awt.dnd",
				"java.awt.dnd.peer",
				"java.awt.event",
				"java.awt.font",
				"java.awt.geom",
				"java.awt.im",
				"java.awt.im.spi",
				"java.awt.image",
				"java.awt.image.renderable",
				"java.awt.peer",
				"java.awt.print",
				"java.beans",
				"java.beans.beancontext",
				"java.io",
				"java.lang",
				"java.lang.annotation",
				"java.lang.instrument",
				"java.lang.management",
				"java.lang.ref",
				"java.lang.reflect",
				"java.math",
				"java.net",
				"java.nio",
				"java.nio.channels",
				"java.nio.channels.spi",
				"java.nio.charset",
				"java.nio.charset.spi",
				"java.rmi",
				"java.rmi.activation",
				"java.rmi.dgc",
				"java.rmi.registry",
				"java.rmi.server",
				"java.security",
				"java.security.acl",
				"java.security.cert",
				"java.security.interfaces",
				"java.security.spec",
				"java.sql",
				"java.text",
				"java.util",
				"java.util.concurrent",
				"java.util.concurrent.atomic",
				"java.util.concurrent.locks",
				"java.util.jar",
				"java.util.logging",
				"java.util.prefs",
				"java.util.regex",
				"java.util.zip",
				"javax.accessibility",
				"javax.activity",
				"javax.crypto",
				"javax.crypto.interfaces",
				"javax.crypto.spec",
				"javax.imageio",
				"javax.imageio.event",
				"javax.imageio.metadata",
				"javax.imageio.plugins.bmp",
				"javax.imageio.plugins.jpeg",
				"javax.imageio.spi",
				"javax.imageio.stream",
				"javax.management",
				"javax.management.loading",
				"javax.management.modelmbean",
				"javax.management.monitor",
				"javax.management.openmbean",
				"javax.management.relation",
				"javax.management.remote",
				"javax.management.remote.rmi",
				"javax.management.timer",
				"javax.naming",
				"javax.naming.directory",
				"javax.naming.event",
				"javax.naming.ldap",
				"javax.naming.spi",
				"javax.net",
				"javax.net.ssl",
				"javax.print",
				"javax.print.attribute",
				"javax.print.attribute.standard",
				"javax.print.event",
				"javax.rmi",
				"javax.rmi.CORBA",
				"javax.rmi.ssl",
				"javax.security.auth",
				"javax.security.auth.callback",
				"javax.security.auth.kerberos",
				"javax.security.auth.login",
				"javax.security.auth.spi",
				"javax.security.auth.x500",
				"javax.security.cert",
				"javax.security.sasl",
				"javax.sound.midi",
				"javax.sound.midi.spi",
				"javax.sound.sampled",
				"javax.sound.sampled.spi",
				"javax.sql",
				"javax.sql.rowset",
				"javax.sql.rowset.serial",
				"javax.sql.rowset.spi",
				"javax.swing",
				"javax.swing.border",
				"javax.swing.colorchooser",
				"javax.swing.event",
				"javax.swing.filechooser",
				"javax.swing.plaf",
				"javax.swing.plaf.basic",
				"javax.swing.plaf.metal",
				"javax.swing.plaf.multi",
				"javax.swing.plaf.synth",
				"javax.swing.table",
				"javax.swing.text",
				"javax.swing.text.html",
				"javax.swing.text.html.parser",
				"javax.swing.text.rtf",
				"javax.swing.tree",
				"javax.swing.undo",
				"javax.transaction",
				"javax.transaction.xa",
				"javax.xml",
				"javax.xml.datatype",
				"javax.xml.namespace",
				"javax.xml.parsers",
				"javax.xml.transform",
				"javax.xml.transform.dom",
				"javax.xml.transform.sax",
				"javax.xml.transform.stream",
				"javax.xml.validation",
				"javax.xml.xpath",
				"org.ietf.jgss",
				"org.omg.CORBA",
				"org.omg.CORBA_2_3",
				"org.omg.CORBA_2_3.portable",
				"org.omg.CORBA.DynAnyPackage",
				"org.omg.CORBA.ORBPackage",
				"org.omg.CORBA.portable",
				"org.omg.CORBA.TypeCodePackage",
				"org.omg.CosNaming",
				"org.omg.CosNaming.NamingContextExtPackage",
				"org.omg.CosNaming.NamingContextPackage",
				"org.omg.Dynamic",
				"org.omg.DynamicAny",
				"org.omg.DynamicAny.DynAnyFactoryPackage",
				"org.omg.DynamicAny.DynAnyPackage",
				"org.omg.IOP",
				"org.omg.IOP.CodecFactoryPackage",
				"org.omg.IOP.CodecPackage",
				"org.omg.Messaging",
				"org.omg.PortableInterceptor",
				"org.omg.PortableInterceptor.ORBInitInfoPackage",
				"org.omg.PortableServer",
				"org.omg.PortableServer.CurrentPackage",
				"org.omg.PortableServer.POAManagerPackage",
				"org.omg.PortableServer.POAPackage",
				"org.omg.PortableServer.portable",
				"org.omg.PortableServer.ServantLocatorPackage",
				"org.omg.SendingContext",
				"org.omg.stub.java.rmi",
				"org.w3c.dom",
				"org.w3c.dom.bootstrap",
				"org.w3c.dom.events",
				"org.w3c.dom.ls",
				"org.xml.sax",
				"org.xml.sax.ext",
				"org.xml.sax.helpers"
};

extern "C" ArrayObject* nativeGetBootPackages() {
  Jnjvm* vm = JavaThread::get()->getJVM();
  ArrayObject* obj = 
    (ArrayObject*)vm->upcalls->ArrayOfString->doNew(NUM_BOOT_PACKAGES, vm);
  for (uint32 i = 0; i < NUM_BOOT_PACKAGES; ++i) {
    obj->elements[i] = vm->asciizToStr(bootPackages[i]);
  }
  return obj;
}


}
