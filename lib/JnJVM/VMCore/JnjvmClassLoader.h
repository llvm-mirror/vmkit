//===-- JnjvmClassLoader.h - Jnjvm representation of a class loader -------===//
//
//                              Jnjvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef JNJVM_CLASSLOADER_H
#define JNJVM_CLASSLOADER_H

#include <vector>

#include "types.h"

#include "mvm/Object.h"

namespace jnjvm {

class ArrayUInt8;
class Attribut;
class Class;
class ClassArray;
class ClassMap;
class CommonClass;
class JavaAllocator;
class JavaObject;
class JavaString;
class Jnjvm;
class JnjvmBootstrapLoader;
class JnjvmModule;
class JnjvmModuleProvider;
class Reader;
class Signdef;
class SignMap;
class Typedef;
class TypeMap;
class UTF8;
class UTF8Map;
class ZipArchive;

#ifdef MULTIPLE_VM
class JnjvmSharedLoader;
#endif

/// JnjvmClassLoader - Runtime representation of a class loader. It contains
/// its own tables (signatures, UTF8, types) which are mapped to a single
/// table for non-isolate environments.
///
class JnjvmClassLoader : public mvm::Object {
private:
   
  
  /// isolate - Which isolate defined me? Null for the bootstrap class loader.
  ///
  Jnjvm* isolate;

  /// javaLoder - The Java representation of the class loader. Null for the
  /// bootstrap class loader.
  ///
  JavaObject* javaLoader;
   
  /// internalLoad - Load the class with the given name.
  ///
  virtual CommonClass* internalLoad(const UTF8* utf8);
  
  /// JnjvmClassLoader - Allocate a user-defined class loader. Called on
  /// first use of a Java class loader.
  ///
  JnjvmClassLoader(JnjvmClassLoader& JCL, JavaObject* loader, Jnjvm* isolate);

protected:

  /// classes - The classes this class loader has loaded.
  ///
  ClassMap* classes;
  
  /// javaTypes - Tables of Typedef defined by this class loader. Shared by all
  /// class loaders in a no isolation configuration.
  ///
  TypeMap* javaTypes;

  /// javaSignatures - Tables of Signdef defined by this class loader. Shared
  /// by all class loaders in a no isolation configuration.
  ///
  SignMap* javaSignatures;

public:
  
  /// VT - The virtual table of this class.
  ///
  static VirtualTable* VT;
  
  /// allocator - Reference to the memory allocator, which will allocate UTF8s,
  /// signatures and types.
  ///
  JavaAllocator* allocator;
   
  
  /// hashUTF8 - Tables of UTF8s defined by this class loader. Shared
  /// by all class loaders in a no isolation configuration.
  ///
  UTF8Map * hashUTF8;
  
  /// TheModule - JIT module for compiling methods.
  ///
  JnjvmModule* TheModule;

  /// TheModuleProvider - JIT module provider for dynamic class loading and
  /// lazy compilation.
  ///
  JnjvmModuleProvider* TheModuleProvider;

  /// tracer - Traces a JnjvmClassLoader for GC.
  ///
  virtual void TRACER;
  
  /// print - String representation of the loader for debugging purposes.
  ///
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("Java class loader<>");
  } 
  
  /// getJnjvmLoaderFromJavaObject - Return the Jnjvm runtime representation
  /// of the given class loader.
  ///
  static JnjvmClassLoader* getJnjvmLoaderFromJavaObject(JavaObject*);
  
  /// getJavaClassLoader - Return the Java representation of this class loader.
  ///
  JavaObject* getJavaClassLoader() {
    return javaLoader;
  }
  
  /// loadName - Loads the class of the given name.
  ///
  CommonClass* loadName(const UTF8* name, bool doResolve, bool doThrow);
  
  /// lookupClassFromUTF8 - Lookup a class from an UTF8 name and load it.
  ///
  CommonClass* lookupClassFromUTF8(const UTF8* utf8, unsigned int start,
                                   unsigned int len, bool doResolve,
                                   bool doThrow);
  
  /// lookupClassFromJavaString - Lookup a class from a Java String and load it.
  ///
  CommonClass* lookupClassFromJavaString(JavaString* str, bool doResolve,
                                         bool doThrow);
   
  /// lookupClass - Finds the class of th given name in the class loader's
  /// table.
  ///
  CommonClass* lookupClass(const UTF8* utf8);

  /// constructArray - Hashes a runtime representation of a class with
  /// the given name.
  ///
  ClassArray* constructArray(const UTF8* name);

  /// constructClass - Hashes a runtime representation of a class with
  /// the given name.
  ///
  Class* constructClass(const UTF8* name, ArrayUInt8* bytes = 0);
  
  /// constructType - Hashes a Typedef, an internal representation of a class
  /// still not loaded.
  ///
  Typedef* constructType(const UTF8 * name);

  /// constructSign - Hashes a Signdef, a method signature.
  ///
  Signdef* constructSign(const UTF8 * name);
  
  /// asciizConstructUTF8 - Hashes an UTF8 created from the given asciiz.
  ///
  const UTF8* asciizConstructUTF8(const char* asciiz);

  /// readerConstructUTF8 - Hashes an UTF8 created from the given Unicode
  /// buffer.
  ///
  const UTF8* readerConstructUTF8(const uint16* buf, uint32 size);
  
  /// bootstrapLoader - The bootstrap loader of the JVM. Loads the base
  /// classes.
  ///
  static JnjvmBootstrapLoader* bootstrapLoader;
  
#ifdef MULTIPLE_VM
  /// sharedLoader - Shared loader when multiple vms are executing.
  ///
  static JnjvmSharedLoader* sharedLoader;
#endif

  /// ~JnjvmClassLoader - Destroy the loader. Depending on the JVM
  /// configuration, this may destroy the tables, JIT module and
  /// module provider.
  ///
  ~JnjvmClassLoader();
  
  /// JnjvmClassLoader - Default constructor, zeroes the field.
  ///
  JnjvmClassLoader() {
    hashUTF8 = 0;
    javaTypes = 0;
    javaSignatures = 0;
    TheModule = 0;
    TheModuleProvider = 0;
    isolate = 0;
  }

};

class JnjvmSharedLoader : public JnjvmClassLoader {
private:
  
  /// internalLoad - Load the class with the given name.
  ///
  virtual CommonClass* internalLoad(const UTF8* utf8) {
    fprintf(stderr, "Don't use me");
    exit(1);
  }

public:
  
  /// VT - The virtual table of this class.
  ///
  static VirtualTable* VT;


  /// constructSharedClass - Create a shared representation of the class.
  /// If two classes have the same name but not the same array of bytes, 
  /// raise an exception.
  ///
  Class* constructSharedClass(const UTF8* name, ArrayUInt8* bytes);

  static JnjvmSharedLoader* createSharedLoader();
};


/// JnjvmBootstrapLoader - This class is for the bootstrap class loader, which
/// loads base classes, ie glibj.zip or rt.jar and -Xbootclasspath.
///
class JnjvmBootstrapLoader : public JnjvmClassLoader {
private:
  /// internalLoad - Load the class with the given name.
  ///
  virtual CommonClass* internalLoad(const UTF8* utf8);
     
  /// bootClasspath - List of paths for the base classes.
  ///
  std::vector<const char*> bootClasspath;

  /// bootArchives - List of .zip or .jar files that contain base classes.
  ///
  std::vector<ZipArchive*> bootArchives;
  
  /// openName - Opens a file of the given name and returns it as an array
  /// of byte.
  ///
  ArrayUInt8* openName(const UTF8* utf8);

public:
  
  /// VT - The virtual table of this class.
  ///
  static VirtualTable* VT;
  
  /// tracer - Traces instances of this class.
  ///
  virtual void TRACER;

  /// print - String representation of the loader, for debugging purposes.
  ///
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("Jnjvm bootstrap loader<>");
  } 
  
  /// libClasspathEnv - The paths for dynamic libraries of Classpath, separated
  /// by ':'.
  ///
  const char* libClasspathEnv;

  /// bootClasspathEnv - The path for base classes, seperated by '.'.
  ///
  const char* bootClasspathEnv;

  /// analyseClasspathEnv - Analyse the paths for base classes.
  ///
  void analyseClasspathEnv(const char*);
  
  /// createBootstrapLoader - Creates the bootstrap loader, first thing
  /// to do before any execution of a JVM.
  ///
  static JnjvmBootstrapLoader* createBootstrapLoader();

  
};

} // end namespace jnjvm

#endif // JNJVM_CLASSLOADER_H
