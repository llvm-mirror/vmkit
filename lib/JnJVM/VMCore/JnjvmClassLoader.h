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

class JnjvmClassLoader : public mvm::Object {
private:
  virtual CommonClass* internalLoad(const UTF8* utf8);

public:
 
  static VirtualTable* VT;
  
  virtual void TRACER;
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("Java class loader<>");
  } 
  
  Jnjvm* isolate;
  JavaObject* javaLoader;

  JavaObject* getJavaClassLoader() {
    return javaLoader;
  }
  
  static JnjvmClassLoader* getJnjvmLoaderFromJavaObject(JavaObject*);

  ClassMap* classes;
  
  JavaAllocator* allocator;
  JnjvmModule* TheModule;
  JnjvmModuleProvider* TheModuleProvider;

    // Loads a Class
  virtual CommonClass* loadName(const UTF8* name, bool doResolve,
                                bool doClinit, bool doThrow);
  
  // Class lookup
  CommonClass* lookupClassFromUTF8(const UTF8* utf8, unsigned int start,
                                   unsigned int len,
                                   bool doResolve, bool doClinit, bool doThrow);
  CommonClass* lookupClassFromJavaString(JavaString* str,
                                         bool doResolve, bool doClinit,
                                         bool doThrow);

  void readParents(Class* cl, Reader& reader);
  void loadParents(Class* cl);
  void readAttributs(Class* cl, Reader& reader, std::vector<Attribut*> & attr);
  void readFields(Class* cl, Reader& reader);
  void readMethods(Class* cl, Reader& reader);
  void readClass(Class* cl);
  void initialiseClass(CommonClass* cl);
  void resolveClass(CommonClass* cl, bool doClinit);
  
  CommonClass* lookupClass(const UTF8* utf8);

  ClassArray* constructArray(const UTF8* name);
  Class*      constructClass(const UTF8* name);
  


  TypeMap* javaTypes;
  SignMap* javaSignatures;
  UTF8Map * hashUTF8;
  
  static JnjvmBootstrapLoader* bootstrapLoader;

#ifdef MULTIPLE_VM
  static JnjvmClassLoader* SharedLoader;
#endif

  Typedef* constructType(const UTF8 * name);
  Signdef* constructSign(const UTF8 * name);
  
  ~JnjvmClassLoader();
  JnjvmClassLoader() {
    hashUTF8 = 0;
    javaTypes = 0;
    javaSignatures = 0;
    TheModule = 0;
    TheModuleProvider = 0;
    isolate = 0;
  }

  JnjvmClassLoader(JnjvmClassLoader& JCL, JavaObject* loader, Jnjvm* isolate);
  static JnjvmBootstrapLoader* createBootstrapLoader();


  const UTF8* asciizConstructUTF8(const char* asciiz);

  const UTF8* readerConstructUTF8(const uint16* buf, uint32 size);
  

};


class JnjvmBootstrapLoader : public JnjvmClassLoader {
private:
  virtual CommonClass* internalLoad(const UTF8* utf8);

public:
  
  static VirtualTable* VT;
  
  virtual void TRACER;
  virtual void print(mvm::PrintBuffer* buf) const {
    buf->write("Jnjvm bootstrap loader<>");
  } 
  
  void analyseClasspathEnv(const char*);
  
  const char* libClasspathEnv;
  const char* bootClasspathEnv;
  std::vector<const char*> bootClasspath;
  std::vector<ZipArchive*> bootArchives;
  
  ArrayUInt8* openName(const UTF8* utf8);
};

} // end namespace jnjvm

#endif // JNJVM_CLASSLOADER_H
