//===-------- JavaClass.h - Java class representation -------------------===//
//
//                              JnJVM
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_CLASS_H
#define JNJVM_JAVA_CLASS_H

#include <map>
#include <vector>

#include "types.h"

#include "mvm/JIT.h"
#include "mvm/Method.h"
#include "mvm/Object.h"
#include "mvm/PrintBuffer.h"
#include "mvm/Threads/Cond.h"
#include "mvm/Threads/Locks.h"

#include "JavaAccess.h"
#include "JnjvmClassLoader.h"

namespace jnjvm {

class ArrayUInt8;
class AssessorDesc;
class Enveloppe;
class Class;
class JavaConstantPool;
class JavaField;
class JavaJIT;
class JavaMethod;
class JavaObject;
class Signdef;
class Typedef;
class UTF8;


/// JavaState - List of states a Java class can have. A class is ready to be
/// used (i.e allocating instances of the class, calling methods of the class
/// and accessing static fields of the class) when it is in the ready state.
///
typedef enum JavaState {
  hashed = 0,   /// The class is hashed in a class loader table.
  loaded,       /// The .class file has been found.
  classRead,    /// The .class file has been read.
  prepared,     /// The parents of this class has been resolved.
  resolved,     /// The class has been resolved.
  clinitParent, /// The class is cliniting its parents.
  inClinit,     /// The class is cliniting.
  ready         /// The class is ready to be used.
}JavaState;


/// Attribut - This class represents JVM attributes to Java class, methods and
/// fields located in the .class file.
///
class Attribut {
public:
  
  /// name - The name of the attribut. These are specified in the JVM book.
  /// Experimental attributes exist, but the JnJVM does nor parse them.
  ///
  const UTF8* name;

  /// start - The offset in the class of this attribut.
  ///
  unsigned int start;

  /// nbb - The size of the attribut.
  ///
  unsigned int  nbb;

  /// Attribut - Create an attribut at the given length and offset.
  ///
  Attribut(const UTF8* name, uint32 length, uint32 offset);
  
  /// codeAttribut - The "Code" JVM attribut. This is a method attribut for
  /// finding the bytecode of a method in the .class file.
  //
  static const UTF8* codeAttribut;

  /// exceptionsAttribut - The "Exceptions" attribut. This is a method
  /// attribut for finding the exception table of a method in the .class
  /// file.
  ///
  static const UTF8* exceptionsAttribut;

  /// constantAttribut - The "ConstantValue" attribut. This is a field attribut
  /// when the field has a static constant value.
  ///
  static const UTF8* constantAttribut;

  /// lineNumberTableAttribut - The "LineNumberTable" attribut. This is used
  /// for corresponding JVM bytecode to source line in the .java file.
  ///
  static const UTF8* lineNumberTableAttribut;

  /// innerClassAttribut - The "InnerClasses" attribut. This is a class attribut
  /// for knowing the inner/outer informations of a Java class.
  ///
  static const UTF8* innerClassesAttribut;

  /// sourceFileAttribut - The "SourceFile" attribut. This is a class attribut
  /// and gives the correspondance between a class and the name of its Java
  /// file.
  ///
  static const UTF8* sourceFileAttribut;
  
};

/// CommonClass - This class is the root class of all Java classes. It is
/// currently GC-allocated in JnJVM, but will be permanently allocated when the
/// class loader finalizer method will be defined.
///
class CommonClass : public mvm::Object {
private:


/// FieldCmp - Internal class for field and method lookup in a class.
///
class FieldCmp {
public:
  
  /// name - The name of the field/method
  ///
  const UTF8* name;

  /// type - The type of the field/method.
  ///
  const UTF8* type;

  FieldCmp(const UTF8* n, const UTF8* t) : name(n), type(t) {}
  
  inline bool operator<(const FieldCmp &cmp) const;
};

public:
  
//===----------------------------------------------------------------------===//
//
// Do not reorder these fields or add new ones! the LLVM runtime assumes that
// classes have the following beginning layout.
//
//===----------------------------------------------------------------------===//

  
  /// virtualSize - The size of instances of this class. Array classes do
  /// not need this information, but to simplify accessing this field in
  /// the JIT, we put this in here.
  /// 
  uint32 virtualSize;

  /// virtualVT - The virtual table of instances of this class. Like the
  /// virtualSize field, array classes do not need this information. But we
  /// simplify JIT generation to set it here.
  ///
  VirtualTable* virtualVT;
  
  /// display - The class hierarchy of supers for this class. Array classes
  /// do not need it.
  ///
  CommonClass** display;
  
  /// depth - The depth of this class in its class hierarchy. 
  /// display[depth] contains the class. Array classes do not need it.
  ///
  uint32 depth;


//===----------------------------------------------------------------------===//
//
// New fields can be added from now, or reordered.
//
//===----------------------------------------------------------------------===//
  

  /// virtualTableSize - The size of the virtual table of this class.
  ///
  uint32 virtualTableSize;
   
  /// access - {public, private, protected}.
  ///
  uint32 access;
  
  /// isArray - Is the class an array class?
  ///
  bool isArray;
  
  /// isPrimitive - Is the class a primitive class?
  ///
  bool isPrimitive;
  
  /// name - The name of the class.
  ///
  const UTF8* name;
  
  /// status - The loading/resolve/initialization state of the class.
  ///
  JavaState status;
  
  /// super - The parent of this class.
  ///
  CommonClass * super;

  /// superUTF8 - The name of the parent of this class.
  ///
  const UTF8* superUTF8;
  
  /// interfaces - The interfaces this class implements.
  ///
  std::vector<Class*> interfaces;

  /// interfacesUTF8 - The names of the interfaces this class implements.
  ///
  std::vector<const UTF8*> interfacesUTF8;
  
  /// lockVar - When multiple threads want to load/resolve/initialize a class,
  /// they must be synchronized so that these steps are only performned once
  /// for a given class.
  mvm::Lock* lockVar;

  /// condVar - Used to wake threads waiting on the load/resolve/initialize
  /// process of this class, done by another thread.
  mvm::Cond* condVar;
  
  /// classLoader - The Jnjvm class loader that loaded the class.
  ///
  JnjvmClassLoader* classLoader;
  
#ifndef MULTIPLE_VM
  /// delegatee - The java/lang/Class object representing this class
  ///
  JavaObject* delegatee;
#endif
  

  typedef std::map<const FieldCmp, JavaField*, std::less<FieldCmp> >::iterator
    field_iterator;
  
  typedef std::map<const FieldCmp, JavaField*, std::less<FieldCmp> > 
    field_map;
  
  /// virtualFields - List of all the virtual fields defined in this class.
  /// This does not contain non-redefined super fields.
  field_map virtualFields;
  
  /// staticFields - List of all the static fields defined in this class.
  ///
  field_map staticFields;
  
  typedef std::map<const FieldCmp, JavaMethod*, std::less<FieldCmp> >::iterator
    method_iterator;
  
  typedef std::map<const FieldCmp, JavaMethod*, std::less<FieldCmp> > 
    method_map;
  
  /// virtualMethods - List of all the virtual methods defined by this class.
  /// This does not contain non-redefined super methods.
  method_map virtualMethods;
  
  /// staticMethods - List of all the static methods defined by this class.
  ///
  method_map staticMethods;
  
  /// constructMethod - Add a new method in this class method map.
  ///
  JavaMethod* constructMethod(const UTF8* name, const UTF8* type,
                              uint32 access);
  
  /// constructField - Add a new field in this class field map.
  ///
  JavaField* constructField(const UTF8* name, const UTF8* type,
                            uint32 access);

  /// printClassName - Adds a string representation of this class in the
  /// given buffer.
  ///
  static void printClassName(const UTF8* name, mvm::PrintBuffer* buf);
  
  /// acquire - Acquire this class lock.
  ///
  void acquire() {
    lockVar->lock();
  }
  
  /// release - Release this class lock.
  ///
  void release() {
    lockVar->unlock();  
  }

  /// waitClass - Wait for the class to be loaded/initialized/resolved.
  ///
  void waitClass() {
    condVar->wait(lockVar);
  }
  
  /// broadcastClass - Unblock threads that were waiting on the class being
  /// loaded/initialized/resolved.
  ///
  void broadcastClass() {
    condVar->broadcast();  
  }

  /// ownerClass - Is the current thread the owner of this thread?
  ///
  bool ownerClass() {
    return mvm::Lock::selfOwner(lockVar);    
  }
  
  /// lookupMethodDontThrow - Lookup a method in the method map of this class.
  /// Do not throw if the method is not found.
  ///
  JavaMethod* lookupMethodDontThrow(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse);
  
  /// lookupMethod - Lookup a method and throw an exception if not found.
  ///
  JavaMethod* lookupMethod(const UTF8* name, const UTF8* type, bool isStatic,
                           bool recurse);
  
  /// lookupFieldDontThrow - Lookup a field in the field map of this class. Do
  /// not throw if the field is not found.
  ///
  JavaField* lookupFieldDontThrow(const UTF8* name, const UTF8* type,
                                  bool isStatic, bool recurse);
  
  /// lookupField - Lookup a field and throw an exception if not found.
  ///
  JavaField* lookupField(const UTF8* name, const UTF8* type, bool isStatic,
                         bool recurse);

  /// print - Print the class for debugging purposes.
  ///
  virtual void print(mvm::PrintBuffer *buf) const;
  
  /// tracer - The tracer of this GC-allocated class.
  ///
  virtual void TRACER;

  /// inheritName - Does this class in its class hierarchy inherits
  /// the given name? Equality is on the name. This function does not take
  /// into account array classes.
  ///
  bool inheritName(const UTF8* Tname);

  /// isOfTypeName - Does this class inherits the given name? Equality is on
  /// the name. This function takes into account array classes.
  ///
  bool isOfTypeName(const UTF8* Tname);

  /// implements - Does this class implement the given class? Returns true if
  /// the class is in the interface class hierarchy.
  ///
  bool implements(CommonClass* cl);
  
  /// instantationOfArray - If this class is an array class, does its subclass
  /// implements the given array class subclass?
  ///
  bool instantiationOfArray(ClassArray* cl);
  
  /// subclassOf - If this class is a regular class, is it a subclass of the
  /// given class?
  ///
  bool subclassOf(CommonClass* cl);

  /// isAssignableFrom - Is this class assignable from the given class? The
  /// classes may be of any type.
  ///
  bool isAssignableFrom(CommonClass* cl);

  /// getClassDelegatee - Return the java/lang/Class representation of this
  /// class.
  ///
  JavaObject* getClassDelegatee(JavaObject* pd = 0);

  /// resolveClass - If the class has not been resolved yet, resolve it.
  ///
  void resolveClass();

#ifndef MULTIPLE_VM
  /// getStatus - Get the resolution/initialization status of this class.
  ///
  JavaState* getStatus() {
    return &status;
  }
  /// isReady - Has this class been initialized?
  ///
  bool isReady() {
    return status >= inClinit;
  }
#else
  JavaState* getStatus();
  bool isReady() {
    return *getStatus() >= inClinit;
  }
#endif

  /// isResolved - Has this class been resolved?
  ///
  bool isResolved() {
    return status >= resolved;
  }
  
  /// CommonClass - Create a class with th given name.
  ///
  CommonClass(JnjvmClassLoader* loader, const UTF8* name, bool isArray);
  
  /// ~CommonClass - Free memory used by this class, and remove it from
  /// metadata.
  ///
  ~CommonClass();

  /// CommonClass - Default constructor.
  ///
  CommonClass();
  
  mvm::JITInfo* JInfo;
  template<typename Ty> 
  Ty *getInfo() {
    if (!JInfo) {
      JInfo = new Ty(this);
    }   

    assert((void*)dynamic_cast<Ty*>(JInfo) == (void*)JInfo &&
           "Invalid concrete type or multiple inheritence for getInfo");
    return static_cast<Ty*>(JInfo);
  }

#ifdef MULTIPLE_VM
  bool isSharedClass() {
    return classLoader == JnjvmClassLoader::sharedLoader;
  }
#endif

};

/// ClassPrimitive - This class represents internal classes for primitive
/// types, e.g. java/lang/Integer.TYPE.
///
class ClassPrimitive : public CommonClass {
public:
  ClassPrimitive(JnjvmClassLoader* loader, const UTF8* name);
};


/// Class - This class is the representation of Java regular classes (i.e not
/// array or primitive). Theses classes have a constant pool.
///
class Class : public CommonClass {
public:
  
  /// VT - The virtual table of this class.
  ///
  static VirtualTable* VT;

  /// minor - The minor version of this class.
  ///
  unsigned int minor;

  /// major - The major version of this class.
  ///
  unsigned int major;

  /// bytes - The .class file of this class.
  ///
  ArrayUInt8* bytes;

  /// ctpInfo - The constant pool info of this class.
  ///
  JavaConstantPool* ctpInfo;

  /// attributs - JVM attributes of this class.
  ///
  std::vector<Attribut*> attributs;
  
  /// innerClasses - The inner classes of this class.
  ///
  std::vector<Class*> innerClasses;

  /// outerClass - The outer class, if this class is an inner class.
  ///
  Class* outerClass;

  /// innerAccess - The access of this class, if this class is an inner class.
  ///
  uint32 innerAccess;

  /// innerOuterResolved - Is the inner/outer resolution done?
  ///
  bool innerOuterResolved;
  
  /// staticSize - The size of the static instance of this class.
  ///
  uint32 staticSize;

  /// staticVT - The virtual table of the static instance of this class.
  ///
  VirtualTable* staticVT;

  /// doNew - Allocates a Java object whose class is this class.
  ///
  JavaObject* doNew(Jnjvm* vm);

  /// print - Prints a string representation of this class in the buffer.
  ///
  virtual void print(mvm::PrintBuffer *buf) const;

  /// tracer - Tracer function of instances of Class.
  ///
  virtual void TRACER;
  
  ~Class();
  Class();
  
  /// lookupAttribut - Look up a JVM attribut of this class.
  ///
  Attribut* lookupAttribut(const UTF8* key);
  
  /// staticInstance - Get the static instance of this class. A static instance
  /// is inlined when the vm is in a single environment. In a multiple
  /// environment, the static instance is in a hashtable.
  ///
#ifndef MULTIPLE_VM
  JavaObject* _staticInstance;
  JavaObject* staticInstance() {
    return _staticInstance;
  }

  /// createStaticInstance - Create the static instance of this class. This is
  /// a no-op in a single environment because it is created only once when
  /// creating the static type of the class. In a multiple environment, it is
  /// called on each initialization of the class.
  ///
  void createStaticInstance() { }
#else
  JavaObject* staticInstance();
  void createStaticInstance();
#endif
  
  /// Class - Create a class in the given virtual machine and with the given
  /// name.
  Class(JnjvmClassLoader* loader, const UTF8* name);
  
  /// readParents - Reads the parents, i.e. super and interfaces, of the class.
  ///
  void readParents(Reader& reader);

  /// loadParents - Loads and resolves the parents, i.e. super and interfarces,
  /// of the class.
  ///
  void loadParents();

  /// readAttributs - Reads the attributs of the class.
  ///
  void readAttributs(Reader& reader, std::vector<Attribut*> & attr);

  /// readFields - Reads the fields of the class.
  ///
  void readFields(Reader& reader);

  /// readMethods - Reads the methods of the class.
  ///
  void readMethods(Reader& reader);
  
  /// readClass - Reads the class.
  ///
  void readClass();
};

/// ClassArray - This class represents Java array classes.
///
class ClassArray : public CommonClass {
public:
  
  /// VT - The virtual table of array classes.
  ///
  static VirtualTable* VT;

  /// _baseClass - The base class of the array, or null if not resolved.
  ///
  CommonClass*  _baseClass;

  /// _funcs - The type of the base class of the array (primitive or
  /// reference). Null if not resolved.
  ///
  AssessorDesc* _funcs;

  /// resolveComponent - Resolve the array class. The base class and the
  /// AssessorDesc are resolved.
  ///
  void resolveComponent();
  
  /// baseClass - Get the base class of this array class. Resolve the array
  /// class if needed.
  ///
  CommonClass* baseClass() {
    if (_baseClass == 0)
      resolveComponent();
    return _baseClass;
  }

  /// funcs - Get the type of the base class/ Resolve the array if needed.
  AssessorDesc* funcs() {
    if (_funcs == 0)
      resolveComponent();
    return _funcs;
  }
  
  /// ClassArray - Empty constructor for VT.
  ///
  ClassArray() {}

  /// ClassArray - Construct a Java array class with the given name.
  ///
  ClassArray(JnjvmClassLoader* loader, const UTF8* name);
  

  /// arrayLoader - Return the class loader of the class with the name 'name'.
  /// If the class has not been loaded, load it with the given loader and
  /// return the real class loader that loaded this class.
  ///
  static JnjvmClassLoader* arrayLoader(const UTF8* name,
                                       JnjvmClassLoader* loader,
                                       unsigned int start,
                                       unsigned int end);

  /// print - Print a string representation of this array class. Used for
  /// debugging purposes.
  ///
  virtual void print(mvm::PrintBuffer *buf) const;

  /// tracer - Tracer of array classes.
  ///
  virtual void TRACER;

  /// SuperArray - The super class of array classes.
  ///
  static CommonClass* SuperArray;

  /// InterfacesArray - The interfaces that array classes implement.
  ///
  static std::vector<Class*> InterfacesArray;
};

/// JavaMethod - This class represents Java methods.
///
class JavaMethod {
  friend class CommonClass;
private:

  /// _signature - The signature of this method. Null if not resolved.
  ///
  Signdef* _signature;

public:
  
  /// compiledPtr - Return a pointer to the compiled code of this Java method,
  /// compiling it if necessary.
  ///
  void* compiledPtr();

  /// JavaMethod - Delete the method as well as the cache enveloppes and
  /// attributes of the method.
  ///
  ~JavaMethod();

  /// access - Java access type of this method (e.g. private, public...).
  ///
  unsigned int access;

  /// attributs - List of Java attributs of this method.
  ///
  std::vector<Attribut*> attributs;

  /// caches - List of caches in this method. For all invokeinterface bytecode
  /// there is a corresponding cache.
  ///
  std::vector<Enveloppe*> caches;
  
  /// classDef - The Java class where the method is defined.
  ///
  Class* classDef;

  /// name - The name of the method.
  ///
  const UTF8* name;

  /// type - The UTF8 signature of the method.
  ///
  const UTF8* type;

  /// canBeInlined - Can the method be inlined?
  ///
  bool canBeInlined;

  /// code - Pointer to the compiled code of this method.
  ///
  void* code;
  
  /// offset - The index of the method in the virtual table.
  ///
  uint32 offset;

  /// lookupAttribut - Look up an attribut in the method's attributs. Returns
  /// null if the attribut is not found.
  ///
  Attribut* lookupAttribut(const UTF8* key);

  /// getSignature - Get the signature of thes method, resolving it if
  /// necessary.
  ///
  Signdef* getSignature() {
    if(!_signature)
      _signature = classDef->classLoader->constructSign(type);
    return _signature;
  }
  
  /// printString - Output a string representation of the method.
  ///
  const char* printString() const;

//===----------------------------------------------------------------------===//
//
// Upcalls from JnJVM code to Java code. 
//
//===----------------------------------------------------------------------===//
  
  /// This class of methods takes a variable argument list.
  uint32 invokeIntSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  float invokeFloatSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  double invokeDoubleSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  sint64 invokeLongSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  JavaObject* invokeJavaObjectSpecialAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  
  uint32 invokeIntVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  float invokeFloatVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  double invokeDoubleVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  sint64 invokeLongVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  JavaObject* invokeJavaObjectVirtualAP(Jnjvm* vm, JavaObject* obj, va_list ap);
  
  uint32 invokeIntStaticAP(Jnjvm* vm, va_list ap);
  float invokeFloatStaticAP(Jnjvm* vm, va_list ap);
  double invokeDoubleStaticAP(Jnjvm* vm, va_list ap);
  sint64 invokeLongStaticAP(Jnjvm* vm, va_list ap);
  JavaObject* invokeJavaObjectStaticAP(Jnjvm* vm, va_list ap);

  /// This class of methods takes a buffer which contain the arguments of the
  /// call.
  uint32 invokeIntSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  float invokeFloatSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  double invokeDoubleSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  sint64 invokeLongSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  JavaObject* invokeJavaObjectSpecialBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  
  uint32 invokeIntVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  float invokeFloatVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  double invokeDoubleVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  sint64 invokeLongVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  JavaObject* invokeJavaObjectVirtualBuf(Jnjvm* vm, JavaObject* obj, void* buf);
  
  uint32 invokeIntStaticBuf(Jnjvm* vm, void* buf);
  float invokeFloatStaticBuf(Jnjvm* vm, void* buf);
  double invokeDoubleStaticBuf(Jnjvm* vm, void* buf);
  sint64 invokeLongStaticBuf(Jnjvm* vm, void* buf);
  JavaObject* invokeJavaObjectStaticBuf(Jnjvm* vm, void* buf);

  /// This class of methods is variadic.
  uint32 invokeIntSpecial(Jnjvm* vm, JavaObject* obj, ...);
  float invokeFloatSpecial(Jnjvm* vm, JavaObject* obj, ...);
  double invokeDoubleSpecial(Jnjvm* vm, JavaObject* obj, ...);
  sint64 invokeLongSpecial(Jnjvm* vm, JavaObject* obj, ...);
  JavaObject* invokeJavaObjectSpecial(Jnjvm* vm, JavaObject* obj, ...);
  
  uint32 invokeIntVirtual(Jnjvm* vm, JavaObject* obj, ...);
  float invokeFloatVirtual(Jnjvm* vm, JavaObject* obj, ...);
  double invokeDoubleVirtual(Jnjvm* vm, JavaObject* obj, ...);
  sint64 invokeLongVirtual(Jnjvm* vm, JavaObject* obj, ...);
  JavaObject* invokeJavaObjectVirtual(Jnjvm* vm, JavaObject* obj, ...);
  
  uint32 invokeIntStatic(Jnjvm* vm, ...);
  float invokeFloatStatic(Jnjvm* vm, ...);
  double invokeDoubleStatic(Jnjvm* vm, ...);
  sint64 invokeLongStatic(Jnjvm* vm, ...);
  JavaObject* invokeJavaObjectStatic(Jnjvm* vm, ...);
  
  mvm::JITInfo* JInfo;
  template<typename Ty> 
  Ty *getInfo() {
    if (!JInfo) {
      JInfo = new Ty(this);
    }   

    assert((void*)dynamic_cast<Ty*>(JInfo) == (void*)JInfo &&
           "Invalid concrete type or multiple inheritence for getInfo");
    return static_cast<Ty*>(JInfo);
  }
  
};

/// JavaField - This class represents a Java field.
///
class JavaField  {
  friend class CommonClass;
private:
  /// _signature - The signature of the field. Null if not resolved.
  ///
  Typedef* _signature;
public:

  /// ~JavaField - Destroy the field as well as its attributs.
  ///
  ~JavaField();

  /// access - The Java access type of this field (e.g. public, private).
  ///
  unsigned int access;

  /// name - The name of the field.
  ///
  const UTF8* name;

  /// type - The UTF8 type name of the field.
  ///
  const UTF8* type;

  /// attributs - List of Java attributs for this field.
  ///
  std::vector<Attribut*> attributs;

  /// classDef - The class where the field is defined.
  ///
  Class* classDef;

  /// ptrOffset - The offset of the field when the object containing
  /// the field is casted to an array of bytes.
  ///
  uint64 ptrOffset;
  
  /// num - The index of the field in the field list.
  ///
  uint32 num;
  
  /// getSignature - Get the signature of this field, resolving it if
  /// necessary.
  ///
  Typedef* getSignature() {
    if(!_signature)
      _signature = classDef->classLoader->constructType(type);
    return _signature;
  }

  /// initField - Init the value of the field in the given object. This is
  /// used for static fields which have a default value.
  ///
  void initField(JavaObject* obj);

  /// lookupAttribut - Look up the attribut in the field's list of attributs.
  ///
  Attribut* lookupAttribut(const UTF8* key);

  /// printString - Output a string representation of the field.
  ///
  const char* printString() const;

  /// getVritual*Field - Get a virtual field of an object.
  ///
  #define GETVIRTUALFIELD(TYPE, TYPE_NAME) \
  TYPE getVirtual##TYPE_NAME##Field(JavaObject* obj) { \
    assert(*(classDef->getStatus()) >= inClinit); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    return ((TYPE*)ptr)[0]; \
  }

  /// getStatic*Field - Get a static field in the defining class.
  ///
  #define GETSTATICFIELD(TYPE, TYPE_NAME) \
  TYPE getStatic##TYPE_NAME##Field() { \
    assert(*(classDef->getStatus()) >= inClinit); \
    JavaObject* obj = classDef->staticInstance(); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    return ((TYPE*)ptr)[0]; \
  }

  /// setVirtual*Field - Set a virtual of an object.
  ///
  #define SETVIRTUALFIELD(TYPE, TYPE_NAME) \
  void setVirtual##TYPE_NAME##Field(JavaObject* obj, TYPE val) { \
    assert(*(classDef->getStatus()) >= inClinit); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    ((TYPE*)ptr)[0] = val; \
  }

  /// setStatic*Field - Set a static field in the defining class.
  #define SETSTATICFIELD(TYPE, TYPE_NAME) \
  void setStatic##TYPE_NAME##Field(TYPE val) { \
    assert(*(classDef->getStatus()) >= inClinit); \
    JavaObject* obj = classDef->staticInstance(); \
    void* ptr = (void*)((uint64)obj + ptrOffset); \
    ((TYPE*)ptr)[0] = val; \
  }

  #define MK_ASSESSORS(TYPE, TYPE_NAME) \
    GETVIRTUALFIELD(TYPE, TYPE_NAME) \
    SETVIRTUALFIELD(TYPE, TYPE_NAME) \
    GETSTATICFIELD(TYPE, TYPE_NAME) \
    SETSTATICFIELD(TYPE, TYPE_NAME) \

  MK_ASSESSORS(float, Float);
  MK_ASSESSORS(double, Double);
  MK_ASSESSORS(JavaObject*, Object);
  MK_ASSESSORS(uint8, Int8);
  MK_ASSESSORS(uint16, Int16);
  MK_ASSESSORS(uint32, Int32);
  MK_ASSESSORS(sint64, Long);
  
  mvm::JITInfo* JInfo;
  template<typename Ty> 
  Ty *getInfo() {
    if (!JInfo) {
      JInfo = new Ty(this);
    }   

    assert((void*)dynamic_cast<Ty*>(JInfo) == (void*)JInfo &&
           "Invalid concrete type or multiple inheritence for getInfo");
    return static_cast<Ty*>(JInfo);
  }

};


} // end namespace jnjvm

#endif
