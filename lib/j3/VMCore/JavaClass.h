//===-------- JavaClass.h - Java class representation -------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef JNJVM_JAVA_CLASS_H
#define JNJVM_JAVA_CLASS_H


#include "types.h"

#include "vmkit/Allocator.h"
#include "vmkit/MethodInfo.h"
#include "vmkit/Cond.h"
#include "vmkit/Locks.h"

#include "JavaAccess.h"
#include "JavaObject.h"
#include "JnjvmClassLoader.h"
#include "JnjvmConfig.h"
#include "Jnjvm.h"
#include "JavaTypes.h"
#include "JavaThread.h"

#include <cassert>
#include <set>

namespace j3 {

class ArrayUInt8;
class ArrayUInt16;
class Class;
class ClassArray;
class ClassBytes;
class JavaArray;
class JavaConstantPool;
class JavaField;
class JavaMethod;
class JavaObject;
class JavaVirtualTable;
class Reader;
class Signdef;
class Typedef;

template <class T> class TJavaArray;
typedef TJavaArray<JavaObject*> ArrayObject;

/// JavaState - List of states a Java class can have. A class is ready to be
/// used (i.e allocating instances of the class, calling methods of the class
/// and accessing static fields of the class) when it is in the ready state.
///
#define loaded 0       /// The .class file has been found.
#define resolving 1    /// The .class file is being resolved.
#define resolved 2     /// The class has been resolved.
#define vmjc 3         /// The class is defined in a shared library.
#define inClinit 4     /// The class is cliniting.
#define ready 5        /// The class is ready to be used.
#define erroneous 6    /// The class is in an erroneous state.


extern "C" bool IsSubtypeIntrinsic(JavaVirtualTable* obj, JavaVirtualTable* clazz);
extern "C" bool CheckIfObjectIsAssignableToArrayPosition(JavaObject * obj, JavaObject* array);

class AnnotationReader {
public:
  Reader& reader;
  Class* cl;
  uint16 AnnotationNameIndex;

  AnnotationReader(Reader& R, Class* C) : reader(R), cl(C),
    AnnotationNameIndex(0) {}
  void readAnnotation();
  void readElementValue();
  
  
  void readAnnotationElementValues();

  // createAnnotation - search for a specific annotation in annotation 
  // attributes.
  //
  JavaObject* createAnnotationMapValues(JavaObject* type);
  
  // createElementValue - create the Java type associated with the value
  // of the current annotation key.
  //
  JavaObject* createElementValue(bool nextParameterIsTypeOfMethod, JavaObject* type, const UTF8* lastKey);

  void fillArray(JavaObject* res, int numValues, UserClassArray* classArray);
};

/// Attribute - This class represents JVM attributes to Java class, methods and
/// fields located in the .class file.
///
class JavaAttribute : public vmkit::PermanentObject {
public:
  
  /// name - The name of the attribute. These are specified in the JVM book.
  /// Experimental attributes exist, but the JnJVM does nor parse them.
  ///
  const UTF8* name;

  /// start - The offset in the class of this attribute.
  ///
  uint32 start;

  /// nbb - The size of the attribute.
  ///
  uint32 nbb;

  /// Attribute - Create an attribute at the given length and offset.
  ///
  JavaAttribute(const UTF8* name, uint32 length, uint32 offset);
  JavaAttribute() {}

  /// codeAttribute - The "Code" JVM attribute. This is a method attribute for
  /// finding the bytecode of a method in the .class file.
  //
  static const UTF8* codeAttribute;
  
  /// annotationsAttribute - The "RuntimeVisibleAnnotations" JVM attribute.
  /// This is a method attribute for getting the runtime annotations.
  //
  static const UTF8* annotationsAttribute;

  /// exceptionsAttribute - The "Exceptions" attribute. This is a method
  /// attribute for finding the exception table of a method in the .class
  /// file.
  ///
  static const UTF8* exceptionsAttribute;

  /// constantAttribute - The "ConstantValue" attribute. This is a field attribute
  /// when the field has a static constant value.
  ///
  static const UTF8* constantAttribute;

  /// lineNumberTableAttribute - The "LineNumberTable" attribute. This is used
  /// for corresponding JVM bytecode to source line in the .java file.
  ///
  static const UTF8* lineNumberTableAttribute;

  /// innerClassAttribute - The "InnerClasses" attribute. This is a class attribute
  /// for knowing the inner/outer informations of a Java class.
  ///
  static const UTF8* innerClassesAttribute;

  /// sourceFileAttribute - The "SourceFile" attribute. This is a class attribute
  /// and gives the correspondence between a class and the name of its Java
  /// file.
  ///
  static const UTF8* sourceFileAttribute;

  /// signatureAttribute - The "Signature" attribute.  This is used to record
  /// generics information about a class or method.
  ///
  static const UTF8* signatureAttribute;

  /// enclosingMEthodAttribute - The "EnclosingMethod" attribute.  This is a class
  /// attribute that identifies the method defining a local or anonymous class
  ///
  static const UTF8* enclosingMethodAttribute;

  /// paramAnnotationsAttribute - Annotations for parameters attribute
  ///
  static const UTF8* paramAnnotationsAttribute;

  /// annotationDefaultAttribute - The "AnnotationDefault" attribute
  ///
  static const UTF8* annotationDefaultAttribute;
};

/// TaskClassMirror - The isolate specific class information: the initialization
/// state and the static instance. In a non-isolate environment, there is only
/// one instance of a TaskClassMirror per Class.
class TaskClassMirror {
public:
  
  /// status - The state.
  ///
  uint8 status;

  /// initialized - Is the class initialized?
  bool initialized;

  /// staticInstance - Memory that holds the static variables of the class.
  ///
  void* staticInstance;
};

/// CommonClass - This class is the root class of all Java classes. It is
/// GC-allocated because CommonClasses have to be traceable. A java/lang/Class
/// object that stays in memory has a reference to the class. Same for
/// super or interfaces.
///
class CommonClass : public vmkit::PermanentObject {

public:
  
//===----------------------------------------------------------------------===//
//
// If you want to add new fields or modify the types of fields, you must also
// change their LLVM representation in LLVMRuntime/runtime-*.ll, and their
// offsets in JnjvmModule.cpp.
//
//===----------------------------------------------------------------------===//
 
  /// delegatees - The java/lang/Class delegatee.
  ///
  JavaObject* delegatee[NR_ISOLATES];

  /// access - {public, private, protected}.
  ///
  uint32 access;
  
  /// interfaces - The interfaces this class implements.
  ///
  Class** interfaces; 
  uint16 nbInterfaces;
  
  /// name - The name of the class.
  ///
  const UTF8* name;
   
  /// super - The parent of this class.
  ///
  Class * super;
   
  /// classLoader - The Jnjvm class loader that loaded the class.
  ///
  JnjvmClassLoader* classLoader;
  
  /// virtualVT - The virtual table of instances of this class.
  ///
  JavaVirtualTable* virtualVT;
  

//===----------------------------------------------------------------------===//
//
// End field declaration.
//
//===----------------------------------------------------------------------===//

  bool isSecondaryClass() {
    return virtualVT->offset == JavaVirtualTable::getCacheIndex();
  }

  // Assessor methods.
  uint32 getAccess() const      { return access & 0xFFFF; }
  Class** getInterfaces() const { return interfaces; }
  const UTF8* getName() const   { return name; }
  Class* getSuper() const       { return super; }
  
  std::string& getName(std::string& nameBuffer, bool linkageName = false) const;

  /// isArray - Is the class an array class?
  ///
  bool isArray() const {
    return j3::isArray(access);
  }
  
  /// isPrimitive - Is the class a primitive class?
  ///
  bool isPrimitive() const {
    return j3::isPrimitive(access);
  }
  
  /// isInterface - Is the class an interface?
  ///
  bool isInterface() const {
    return j3::isInterface(access);
  }
  
  /// isClass - Is the class a real, instantiable class?
  ///
  bool isClass() const {
    return j3::isClass(access);
  }

  /// asClass - Returns the class as a user-defined class
  /// if it is not a primitive or an array.
  ///
  UserClass* asClass() {
    if (isClass()) return (UserClass*)this;
    return 0;
  }
  
  /// asClass - Returns the class as a user-defined class
  /// if it is not a primitive or an array.
  ///
  const UserClass* asClass() const {
    if (isClass()) return (const UserClass*)this;
    return 0;
  }
  
  /// asPrimitiveClass - Returns the class if it's a primitive class.
  ///
  UserClassPrimitive* asPrimitiveClass() {
    if (isPrimitive()) return (UserClassPrimitive*)this;
    return 0;
  }
  
  const UserClassPrimitive* asPrimitiveClass() const {
    if (isPrimitive()) return (const UserClassPrimitive*)this;
    return 0;
  }
  
  /// asArrayClass - Returns the class if it's an array class.
  ///
  UserClassArray* asArrayClass() {
    if (isArray()) return (UserClassArray*)this;
    return 0;
  }
  
  const UserClassArray* asArrayClass() const {
    if (isArray()) return (const UserClassArray*)this;
    return 0;
  }

  /// tracer - The tracer of this GC-allocated class.
  ///
  void tracer(word_t closure);
  
  /// inheritName - Does this class in its class hierarchy inherits
  /// the given name? Equality is on the name. This function does not take
  /// into account array classes.
  ///
  bool inheritName(const uint16* buf, uint32 len);

  /// isOfTypeName - Does this class inherits the given name? Equality is on
  /// the name. This function takes into account array classes.
  ///
  bool isOfTypeName(const UTF8* Tname);

  /// isSubclassOf - Is this class assignable from the given class? The
  /// classes may be of any type.
  ///
  bool isSubclassOf(CommonClass* cl);

  /// getClassDelegatee - Return the java/lang/Class representation of this
  /// class.
  ///
  JavaObject* getClassDelegatee(Jnjvm* vm, JavaObject* pd = NULL);
  
  /// getClassDelegateePtr - Return a pointer on the java/lang/Class
  /// representation of this class. Used for JNI.
  ///
  JavaObject* const* getClassDelegateePtr(Jnjvm* vm, JavaObject* pd = NULL);
  
  /// CommonClass - Create a class with th given name.
  ///
  CommonClass(JnjvmClassLoader* loader, const UTF8* name);
  
  /// ~CommonClass - Free memory used by this class, and remove it from
  /// metadata.
  ///
  ~CommonClass();

  /// setInterfaces - Set the interfaces of the class.
  ///
  void setInterfaces(Class** I) {
    interfaces = I;
  }
 
  /// toPrimitive - Returns the primitive class which represents
  /// this class, ie void for java/lang/Void.
  ///
  UserClassPrimitive* toPrimitive(Jnjvm* vm) const;
 
  /// getInternal - Return the class.
  ///
  CommonClass* getInternal() {
    return this;
  }
 
  /// setDelegatee - Set the java/lang/Class object of this class.
  ///
  JavaObject* setDelegatee(JavaObject* val);

  /// getDelegatee - Get the java/lang/Class object representing this class.
  ///
  JavaObject* getDelegatee() const {
    return delegatee[0];
  }
  
  /// getDelegatee - Get a pointer on the java/lang/Class object
  /// representing this class.
  ///
  JavaObject* const* getDelegateePtr() const {
    return delegatee;
  }

  /// resolvedImplClass - Return the internal representation of the
  /// java.lang.Class object. The class must be resolved.
  //
  static UserCommonClass* resolvedImplClass(Jnjvm* vm, JavaObject* delegatee,
                                            bool doClinit);

  void dump() const __attribute__((noinline));
  friend std::ostream& operator << (std::ostream& os, const CommonClass& ccl);
};

/// ClassPrimitive - This class represents internal classes for primitive
/// types, e.g. java/lang/Integer.TYPE.
///
class ClassPrimitive : public CommonClass {
public:
  
  /// logSize - The log size of this class, eg 2 for int.
  ///
  uint32 logSize;
  
  
  /// ClassPrimitive - Constructs a primitive class. Only called at boot
  /// time.
  ///
  ClassPrimitive(JnjvmClassLoader* loader, const UTF8* name, uint32 nb);

  /// byteIdToPrimitive - Get the primitive class from its byte representation,
  /// ie int for I.
  ///
  static UserClassPrimitive* byteIdToPrimitive(char id, Classpath* upcalls);
  
};


/// Class - This class is the representation of Java regular classes (i.e not
/// array or primitive). Theses classes have a constant pool.
///
class Class : public CommonClass {

public:
  
  /// virtualSize - The size of instances of this class.
  /// 
  uint32 virtualSize;

  /// aligment - Alignment of instances of this class.
  ///
  uint32 alignment;

  /// IsolateInfo - Per isolate informations for static instances and
  /// initialization state.
  ///
  TaskClassMirror IsolateInfo[NR_ISOLATES];
   
  /// virtualFields - List of all the virtual fields defined in this class.
  /// This does not contain non-redefined super fields.
  JavaField* virtualFields;
  
  /// nbVirtualFields - The number of virtual fields.
  ///
  uint16 nbVirtualFields;

  /// staticFields - List of all the static fields defined in this class.
  ///
  JavaField* staticFields;

  /// nbStaticFields - The number of static fields.
  ///
  uint16 nbStaticFields;
  
  /// virtualMethods - List of all the virtual methods defined by this class.
  /// This does not contain non-redefined super methods.
  JavaMethod* virtualMethods;

  /// nbVirtualMethods - The number of virtual methods.
  ///
  uint32 nbVirtualMethods;
  
  /// staticMethods - List of all the static methods defined by this class.
  ///
  JavaMethod* staticMethods;

  /// nbStaticMethods - The number of static methods.
  ///
  uint16 nbStaticMethods;
  
  /// ownerClass - Who is initializing this class.
  ///
  vmkit::Thread* ownerClass;
  
  /// bytes - The .class file of this class.
  ///
  ClassBytes* bytes;

  /// ctpInfo - The constant pool info of this class.
  ///
  JavaConstantPool* ctpInfo;

  /// attributes - JVM attributes of this class.
  ///
  JavaAttribute* attributes;

  /// nbAttributes - The number of attributes.
  ///
  uint16 nbAttributes;
  
  /// innerClasses - The inner classes of this class.
  ///
  Class** innerClasses;
  
  /// nbInnerClasses - The number of inner classes.
  ///
  uint16 nbInnerClasses;

  /// outerClass - The outer class, if this class is an inner class.
  ///
  Class* outerClass;
  
  /// innerAccess - The access of this class, if this class is an inner class.
  ///
  uint16 innerAccess;

  /// innerOuterResolved - Is the inner/outer resolution done?
  ///
  bool innerOuterResolved;
  
  /// isAnonymous - Is the class an anonymous class?
  ///
  bool isAnonymous;

  /// virtualTableSize - The size of the virtual table of this class.
  ///
  uint32 virtualTableSize;
  
  /// staticSize - The size of the static instance of this class.
  ///
  uint32 staticSize;

  uint16_t minJDKVersionMajor, minJDKVersionMinor, minJDKVersionBuild;

  /// getVirtualSize - Get the virtual size of instances of this class.
  ///
  uint32 getVirtualSize() const { return virtualSize; }
  
  /// getVirtualVT - Get the virtual VT of instances of this class.
  ///
  JavaVirtualTable* getVirtualVT() const { return virtualVT; }

  /// getOwnerClass - Get the thread that is currently initializing the class.
  ///
  vmkit::Thread* getOwnerClass() const {
    return ownerClass;
  }

  /// setOwnerClass - Set the thread that is currently initializing the class.
  ///
  void setOwnerClass(vmkit::Thread* th) {
    ownerClass = th;
  }
 
  /// getOuterClass - Get the class that contains the definition of this class.
  ///
  Class* getOuterClass() const {
    return outerClass;
  }

  /// getInnterClasses - Get the classes that this class defines.
  ///
  Class** getInnerClasses() const {
    return innerClasses;
  }

  /// lookupMethodDontThrow - Lookup a method in the method map of this class.
  /// Do not throw if the method is not found.
  ///
  JavaMethod* lookupMethodDontThrow(const UTF8* name, const UTF8* type,
                                    bool isStatic, bool recurse, Class** cl);
  
  /// lookupInterfaceMethodDontThrow - Lookup a method in the interfaces of
  /// this class.
  /// Do not throw if the method is not found.
  ///
  JavaMethod* lookupInterfaceMethodDontThrow(const UTF8* name,
                                             const UTF8* type);
  
  /// lookupSpecialMethodDontThrow - Lookup a method following the
  /// invokespecial specification.
  /// Do not throw if the method is not found.
  ///
  JavaMethod* lookupSpecialMethodDontThrow(const UTF8* name,
                                           const UTF8* type,
                                           Class* current);
  
  /// lookupMethod - Lookup a method and throw an exception if not found.
  ///
  JavaMethod* lookupMethod(const UTF8* name, const UTF8* type, bool isStatic,
                           bool recurse, Class** cl);
  
  /// lookupInterfaceMethodDontThrow - Lookup a method in the interfaces of
  /// this class.
  /// Throws a MethodNotFoundError if the method can not ne found.
  ///
  JavaMethod* lookupInterfaceMethod(const UTF8* name, const UTF8* type);

  /// lookupFieldDontThrow - Lookup a field in the field map of this class. Do
  /// not throw if the field is not found.
  ///
  JavaField* lookupFieldDontThrow(const UTF8* name, const UTF8* type,
                                  bool isStatic, bool recurse,
                                  Class** definingClass);
  
  /// lookupField - Lookup a field and throw an exception if not found.
  ///
  JavaField* lookupField(const UTF8* name, const UTF8* type, bool isStatic,
                         bool recurse, Class** definingClass);
   
  /// Assessor methods.
  JavaField* getStaticFields() const    { return staticFields; }
  JavaField* getVirtualFields() const   { return virtualFields; }
  JavaMethod* getStaticMethods() const  { return staticMethods; }
  JavaMethod* getVirtualMethods() const { return virtualMethods; }

  
  /// setInnerAccess - Set the access flags of this inner class.
  ///
  void setInnerAccess(uint16 access) {
    innerAccess = access;
  }
   
  /// getStaticSize - Get the size of the static instance.
  ///
  uint32 getStaticSize() const {
    return staticSize;
  }
  
  /// doNew - Allocates a Java object whose class is this class.
  ///
  JavaObject* doNew(Jnjvm* vm);
  
  /// tracer - Tracer function of instances of Class.
  ///
  void tracer(word_t closure);
  
  ~Class();
  Class();
  
  /// lookupAttribute - Look up a JVM attribute of this class.
  ///
  JavaAttribute* lookupAttribute(const UTF8* key);
  
  /// allocateStaticInstance - Allocate the static instance of this class.
  ///
  void* allocateStaticInstance(Jnjvm* vm);
  
  /// Class - Create a class in the given virtual machine and with the given
  /// name.
  Class(JnjvmClassLoader* loader, const UTF8* name, ClassBytes* bytes);
  
  /// readParents - Reads the parents, i.e. super and interfaces, of the class.
  ///
  void readParents(Reader& reader);

  /// loadExceptions - Loads and resolves the exception classes used in catch 
  /// clauses of methods defined in this class.
  ///
  void loadExceptions();

  /// readAttributes - Reads the attributes of the class.
  ///
  JavaAttribute* readAttributes(Reader& reader, uint16& size);

  /// readFields - Reads the fields of the class.
  ///
  void readFields(Reader& reader);

  /// readMethods - Reads the methods of the class.
  ///
  void readMethods(Reader& reader);
  
  /// readClass - Reads the class.
  ///
  void readClass();
 
  /// getConstantPool - Get the constant pool of the class.
  ///
  JavaConstantPool* getConstantPool() const {
    return ctpInfo;
  }

  /// getBytes - Get the bytes of the class file.
  ///
  ClassBytes* getBytes() const {
    return bytes;
  }
  
  /// resolveInnerOuterClasses - Resolve the inner/outer information.
  ///
  void resolveInnerOuterClasses();

  /// resolveClass - If the class has not been resolved yet, resolve it.
  ///
  void resolveClass();
  void resolveParents();

  /// initialiseClass - If the class has not been initialized yet,
  /// initialize it.
  ///
  void initialiseClass(Jnjvm* vm);
  
  /// acquire - Acquire this class lock.
  ///
  void acquire();
  
  /// release - Release this class lock.
  ///
  void release();

  /// waitClass - Wait for the class to be loaded/initialized/resolved.
  ///
  void waitClass();
  
  /// broadcastClass - Unblock threads that were waiting on the class being
  /// loaded/initialized/resolved.
  ///
  void broadcastClass();
  
  /// getCurrentTaskClassMirror - Get the class task mirror of the executing
  /// isolate.
  ///
  TaskClassMirror& getCurrentTaskClassMirror() {
    return IsolateInfo[0];
  }
  
  /// isReadyForCompilation - Can this class be inlined when JITing?
  ///
  bool isReadyForCompilation() {
    return isReady();
  }
  
  /// setResolved - Set the status of the class as resolved.
  ///
  void setResolved() {
    getCurrentTaskClassMirror().status = resolved;
  }
  
  /// setErroneous - Set the class as erroneous.
  ///
  void setErroneous() {
    getCurrentTaskClassMirror().status = erroneous;
  }
  
  /// setIsResolving - The class file is being resolved.
  ///
  void setIsResolving() {
    getCurrentTaskClassMirror().status = resolving;
  }
  
  /// getStaticInstance - Get the memory that holds static variables.
  ///
  void* getStaticInstance() {
    return getCurrentTaskClassMirror().staticInstance;
  }
  
  /// setStaticInstance - Set the memory that holds static variables.
  ///
  void setStaticInstance(void* val) {
    assert(getCurrentTaskClassMirror().staticInstance == NULL);
    getCurrentTaskClassMirror().staticInstance = val;
  }
  
  /// getInitializationState - Get the state of the class.
  ///
  uint8 getInitializationState() {
    return getCurrentTaskClassMirror().status;
  }

  /// setInitializationState - Set the state of the class.
  ///
  void setInitializationState(uint8 st) {
    TaskClassMirror& TCM = getCurrentTaskClassMirror();
    TCM.status = st;
    if (st == ready) TCM.initialized = true;
  }
  
  /// isReady - Has this class been initialized?
  ///
  bool isReady() {
    return getCurrentTaskClassMirror().status == ready;
  }
  
  /// isInitializing - Is the class currently being initialized?
  ///
  bool isInitializing() {
    return getCurrentTaskClassMirror().status >= inClinit;
  }
  
  /// isResolved - Has this class been resolved?
  ///
  bool isResolved() {
    uint8 stat = getCurrentTaskClassMirror().status;
    return (stat >= resolved && stat != erroneous);
  }
  
  /// isErroneous - Is the class in an erroneous state.
  ///
  bool isErroneous() {
    return getCurrentTaskClassMirror().status == erroneous;
  }

  /// isResolving - Is the class currently being resolved?
  ///
  bool isResolving() {
    return getCurrentTaskClassMirror().status == resolving;
  }

  /// isNativeOverloaded - Is the method overloaded with a native function?
  ///
  bool isNativeOverloaded(JavaMethod* meth);

  /// needsInitialisationCheck - Does the method need an initialisation check?
  ///
  bool needsInitialisationCheck();

  /// fillIMT - Fill the vector with vectors of methods with the same IMT
  /// index.
  ///
  void fillIMT(std::set<JavaMethod*>* meths);

  /// makeVT - Create the virtual table of this class.
  ///
  void makeVT();

  static void getMinimalJDKVersion(uint16_t major, uint16_t minor, uint16_t& JDKMajor, uint16_t& JDKMinor, uint16_t& JDKBuild);
  bool isClassVersionSupported(uint16_t major, uint16_t minor);
};

/// ClassArray - This class represents Java array classes.
///
class ClassArray : public CommonClass {

public:
  
  /// _baseClass - The base class of the array.
  ///
  CommonClass*  _baseClass;

  /// baseClass - Get the base class of this array class.
  ///
  CommonClass* baseClass() const {
    return _baseClass;
  }

  /// doNew - Allocate a new array in the given vm.
  ///
  JavaObject* doNew(sint32 n, Jnjvm* vm);

  /// ClassArray - Construct a Java array class with the given name.
  ///
  ClassArray(JnjvmClassLoader* loader, const UTF8* name,
             UserCommonClass* baseClass);
  
  /// SuperArray - The super of class arrays. Namely java/lang/Object.
  ///
  static Class* SuperArray;

  /// InterfacesArray - The list of interfaces for array classes.
  ///
  static Class** InterfacesArray;

  /// initialiseVT - Initialise the primitive and reference array VT.
  /// super is the java/lang/Object class.
  ///
  static void initialiseVT(Class* javaLangObject);
  
};

/// JavaMethod - This class represents Java methods.
///
class JavaMethod : public vmkit::PermanentObject {
private:

  /// _signature - The signature of this method. Null if not resolved.
  ///
  Signdef* _signature;

public:
  
  enum Type {
    Static,
    Special,
    Interface,
    Virtual
  };

  /// initialise - Create a new method.
  ///
  void initialise(Class* cl, const UTF8* name, const UTF8* type, uint16 access);
   
  /// compiledPtr - Return a pointer to the compiled code of this Java method,
  /// compiling it if necessary.
  ///
  void* compiledPtr(Class* customizeFor = NULL);

  /// setNative - Set the method as native.
  ///
  void setNative();
  
  /// JavaMethod - Delete the method as well as the cache enveloppes and
  /// attributes of the method.
  ///
  ~JavaMethod();

  /// access - Java access type of this method (e.g. private, public...).
  ///
  uint16 access;

  /// attributes - List of Java attributes of this method.
  ///
  JavaAttribute* attributes;
  
  /// nbAttributes - The number of attributes.
  ///
  uint16 nbAttributes;

  /// classDef - The Java class where the method is defined.
  ///
  Class* classDef;

  /// name - The name of the method.
  ///
  const UTF8* name;

  /// type - The UTF8 signature of the method.
  ///
  const UTF8* type;

  /// isCustomizable - Can the method be customizable?
  ///
  bool isCustomizable;

  /// code - Pointer to the compiled code of this method.
  ///
  void* code;
 
  /// offset - The index of the method in the virtual table.
  ///
  uint32 offset;

  /// lookupAttribute - Look up an attribute in the method's attributes. Returns
  /// null if the attribute is not found.
  ///
  JavaAttribute* lookupAttribute(const UTF8* key);

  /// lookupLineNumber - Find the line number based on the given frame info.
  ///
  uint16 lookupLineNumber(vmkit::FrameInfo* FI);
  
  /// lookupCtpIndex - Lookup the constant pool index pointed by the opcode
  /// related to the given frame info.
  ///
  uint16 lookupCtpIndex(vmkit::FrameInfo* FI);
  
  /// getSignature - Get the signature of the method, resolving it if
  /// necessary.
  ///
  Signdef* getSignature() {
    if(!_signature)
      _signature = classDef->classLoader->constructSign(type);
    return _signature;
  }
  
  /// toString - Return an array of chars, suitable for creating a string.
  ///
  ArrayUInt16* toString() const;
  
  /// jniConsFromMeth - Construct the JNI name of this method as if
  /// there is no other function in the class with the same name.
  ///
  void jniConsFromMeth(char* buf) const {
    jniConsFromMeth(buf, classDef->name, name, type, isSynthetic(access));
  }

  /// jniConsFromMethOverloaded - Construct the JNI name of this method
  /// as if its name is overloaded in the class.
  ///
  void jniConsFromMethOverloaded(char* buf) const {
    jniConsFromMethOverloaded(buf, classDef->name, name, type,
                              isSynthetic(access));
  }
  
  /// jniConsFromMeth - Construct the non-overloaded JNI name with
  /// the given name and type.
  ///
  static void jniConsFromMeth(char* buf, const UTF8* clName, const UTF8* name,
                              const UTF8* sign, bool synthetic);

  /// jniConsFromMethOverloaded - Construct the overloaded JNI name with
  /// the given name and type.
  ///
  static void jniConsFromMethOverloaded(char* buf, const UTF8* clName,
                                        const UTF8* name, const UTF8* sign,
                                        bool synthetic);
  
  /// getParameterTypes - Get the java.lang.Class of the parameters of
  /// the method, with the given class loader.
  ///
  ArrayObject* getParameterTypes(JnjvmClassLoader* loader);

  /// getExceptionTypes - Get the java.lang.Class of the exceptions of the
  /// method, with the given class loader.
  ///
  ArrayObject* getExceptionTypes(JnjvmClassLoader* loader);

  /// getReturnType - Get the java.lang.Class of the result of the method,
  /// with the given class loader.
  ///
  JavaObject* getReturnType(JnjvmClassLoader* loader);

//===----------------------------------------------------------------------===//
//
// Upcalls from JnJVM code to Java code. 
//
//===----------------------------------------------------------------------===//
  
#define DO_TRY
#define DO_CATCH if (th->pendingException) { th->throwFromJava(); }

private:
	jvalue* marshalArguments(vmkit::ThreadAllocator& allocator, va_list ap);

	template<class TYPE, class FUNC_TYPE_VIRTUAL_BUF>
	TYPE invokeSpecialBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj,
		void* buf) __attribute__((noinline))
	{
		llvm_gcroot(obj, 0);
		verifyNull(obj);

		void* func = this->compiledPtr();
		FUNC_TYPE_VIRTUAL_BUF call =
			(FUNC_TYPE_VIRTUAL_BUF)getSignature()->getVirtualCallBuf();

		JavaThread* th = JavaThread::get();
		th->startJava();
		TYPE res;

		DO_TRY
			res = call(cl->getConstantPool(), func, obj, buf);
		DO_CATCH

		th->endJava();
		return res;
	}

	template<class TYPE, class FUNC_TYPE_VIRTUAL_BUF>
	TYPE invokeVirtualBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj,
		void* buf) __attribute__((noinline))
	{
		llvm_gcroot(obj, 0);

		UserCommonClass* theClass = JavaObject::getClass(obj);
		UserClass* objCl =
			theClass->isArray() ? theClass->super : theClass->asClass();

		JavaMethod* meth = this;
		if ((objCl != classDef) && !isFinal(access)) {
			meth = objCl->lookupMethodDontThrow(name, type, false, true, &cl);
			assert(meth && "No method found");
		}

		assert(objCl->isSubclassOf(meth->classDef) && "Wrong type");

		return meth->invokeSpecialBuf<TYPE, FUNC_TYPE_VIRTUAL_BUF>(
			vm, cl, obj, buf);
	}

	template<class TYPE, class FUNC_TYPE_STATIC_BUF>
	TYPE invokeStaticBuf(Jnjvm* vm, UserClass* cl,
		void* buf) __attribute__((noinline))
	{
		if (!cl->isReady()) {
			cl->resolveClass();
			cl->initialiseClass(vm);
		}

		void* func = this->compiledPtr();
		FUNC_TYPE_STATIC_BUF call =
			(FUNC_TYPE_STATIC_BUF)getSignature()->getStaticCallBuf();

		JavaThread* th = JavaThread::get();
		th->startJava();
		TYPE res;

		DO_TRY
			res = call(cl->getConstantPool(), func, buf);
		DO_CATCH

		th->endJava();
		return res;
	}

	template<class TYPE, class FUNC_TYPE_VIRTUAL_BUF>
	TYPE invokeVirtualAP(Jnjvm* vm, UserClass* cl, JavaObject* obj,
		va_list ap) __attribute__((noinline))
	{
		llvm_gcroot(obj, 0);
		assert(cl && "Class is NULL");
		vmkit::ThreadAllocator allocator;
		jvalue* buffer = marshalArguments(allocator, ap);
		return invokeVirtualBuf<TYPE, FUNC_TYPE_VIRTUAL_BUF>(
			vm, cl, obj, buffer);
	}

	template<class TYPE, class FUNC_TYPE_VIRTUAL_BUF>
	TYPE invokeSpecialAP(Jnjvm* vm, UserClass* cl, JavaObject* obj,
		va_list ap) __attribute__((noinline))
	{
		llvm_gcroot(obj, 0);
		assert(cl && "Class is NULL");
		vmkit::ThreadAllocator allocator;
		jvalue* buffer = marshalArguments(allocator, ap);
		return invokeSpecialBuf<TYPE, FUNC_TYPE_VIRTUAL_BUF>(
			vm, cl, obj, buffer);
	}

	template<class TYPE, class FUNC_TYPE_STATIC_BUF>
	TYPE invokeStaticAP(Jnjvm* vm, UserClass* cl,
		va_list ap) __attribute__((noinline))
	{
		assert(cl && "Class is NULL");
		vmkit::ThreadAllocator allocator;
		jvalue* buffer = marshalArguments(allocator, ap);
		return invokeStaticBuf<TYPE, FUNC_TYPE_STATIC_BUF>(
			vm, cl, buffer);
	}

#define JavaMethod_DECL_INVOKE_VA(TYPE, TYPE_NAME) \
	TYPE invoke##TYPE_NAME##Virtual(Jnjvm* vm, UserClass* cl, JavaObject* obj, ...) __attribute__ ((noinline));	\
	TYPE invoke##TYPE_NAME##Special(Jnjvm* vm, UserClass* cl, JavaObject* obj, ...) __attribute__ ((noinline));	\
	TYPE invoke##TYPE_NAME##Static(Jnjvm* vm, UserClass* cl, ...) __attribute__ ((noinline));

#define JavaMethod_DECL_INVOKE_AP(TYPE, TYPE_NAME)	\
	TYPE invoke##TYPE_NAME##VirtualAP(Jnjvm* vm, UserClass* cl, JavaObject* obj, va_list ap) __attribute__ ((noinline));	\
	TYPE invoke##TYPE_NAME##SpecialAP(Jnjvm* vm, UserClass* cl, JavaObject* obj, va_list ap) __attribute__ ((noinline));	\
	TYPE invoke##TYPE_NAME##StaticAP(Jnjvm* vm, UserClass* cl, va_list ap) __attribute__ ((noinline));

#define JavaMethod_DECL_INVOKE_BUF(TYPE, TYPE_NAME)	\
	TYPE invoke##TYPE_NAME##VirtualBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj, void* buf) __attribute__ ((noinline));	\
	TYPE invoke##TYPE_NAME##SpecialBuf(Jnjvm* vm, UserClass* cl, JavaObject* obj, void* buf) __attribute__ ((noinline));	\
 	TYPE invoke##TYPE_NAME##StaticBuf(Jnjvm* vm, UserClass* cl, void* buf) __attribute__ ((noinline));

#define JavaMethod_DECL_INVOKE(TYPE, TYPE_NAME) \
	JavaMethod_DECL_INVOKE_BUF(TYPE, TYPE_NAME)	\
	JavaMethod_DECL_INVOKE_AP(TYPE, TYPE_NAME)	\
	JavaMethod_DECL_INVOKE_VA(TYPE, TYPE_NAME)

public:
	JavaMethod_DECL_INVOKE(uint32_t, Int)
	JavaMethod_DECL_INVOKE(int64_t, Long)
	JavaMethod_DECL_INVOKE(float,  Float)
	JavaMethod_DECL_INVOKE(double, Double)
	JavaMethod_DECL_INVOKE(JavaObject*, JavaObject)

	std::string& getName(std::string& nameBuffer, bool linkageName = false) const;
	friend std::ostream& operator << (std::ostream&, const JavaMethod&);
	void dump() const __attribute__((noinline));
  
  #define JNI_NAME_PRE "Java_"
  #define JNI_NAME_PRE_LEN 5
  
};

/// JavaField - This class represents a Java field.
///
class JavaField  : public vmkit::PermanentObject {
private:
  /// _signature - The signature of the field. Null if not resolved.
  ///
  Typedef* _signature;
  
  /// InitField - Set an initial value to the field.
  ///
  void InitStaticField(uint64 val);
  void InitStaticField(JavaObject* val);
  void InitStaticField(double val);
  void InitStaticField(float val);
  void InitNullStaticField();

public:
  
  /// constructField - Create a new field.
  ///
  void initialise(Class* cl, const UTF8* name, const UTF8* type, uint16 access);

  /// ~JavaField - Destroy the field as well as its attributes.
  ///
  ~JavaField();

  /// access - The Java access type of this field (e.g. public, private).
  ///
  uint16 access;

  /// name - The name of the field.
  ///
  const UTF8* name;

  /// type - The UTF8 type name of the field.
  ///
  const UTF8* type;

  /// attributes - List of Java attributes for this field.
  ///
  JavaAttribute* attributes;
  
  /// nbAttributes - The number of attributes.
  ///
  uint16 nbAttributes;

  /// classDef - The class where the field is defined.
  ///
  Class* classDef;

  /// ptrOffset - The offset of the field when the object containing
  /// the field is casted to an array of bytes.
  ///
  uint32 ptrOffset;
  
  /// num - The index of the field in the field list.
  ///
  uint16 num;
  
  /// getSignature - Get the signature of this field, resolving it if
  /// necessary.
  ///
  Typedef* getSignature() {
    if(!_signature)
      _signature = classDef->classLoader->constructType(type);
    return _signature;
  }

  /// InitStaticField - Init the value of the field in the given object. This is
  /// used for static fields which have a default value.
  ///
  void InitStaticField(Jnjvm* vm);

  /// lookupAttribute - Look up the attribute in the field's list of attributes.
  ///
  JavaAttribute* lookupAttribute(const UTF8* key);

  JavaObject** getStaticObjectFieldPtr() {
    assert(classDef->getStaticInstance());
    return (JavaObject**)((uint64)classDef->getStaticInstance() + ptrOffset);
  }

  JavaObject** getInstanceObjectFieldPtr(JavaObject* obj) {
    llvm_gcroot(obj, 0);
    return (JavaObject**)((uint64)obj + ptrOffset);
  }

private:
  /// getStatic*Field - Get a static field.
  ///
  template <class TYPE>
  TYPE getStaticField() __attribute__ ((noinline)) {
    assert(classDef->isResolved());
    void* ptr = (void*)((uint64)classDef->getStaticInstance() + ptrOffset);
    return *static_cast<TYPE *>(ptr);
  }

  /*
    The struct FieldSetter is a workaround in C++ to enable
    template-based method specialization in a non-template
    class. See specialization below the class declaration.
  */
  template<class TYPE>
  struct FieldSetter {
	  /// setStatic*Field - Set a field of an object.
	  ///
	  static void setStaticField(const JavaField* field, TYPE val) __attribute__ ((noinline)) {
	    assert(field->classDef->isResolved());
	    void* ptr = (void*)((uint64)field->classDef->getStaticInstance() + field->ptrOffset);
	    *static_cast<TYPE *>(ptr) = val;
	  }
      
	  /// setInstance*Field - Set an instance field.
	  ///
	  static void setInstanceField(const JavaField* field, JavaObject* obj, TYPE val) __attribute__ ((noinline)) {
	    llvm_gcroot(obj, 0);
	    assert(field->classDef->isResolved());
	    void* ptr = (void*)((uint64)obj + field->ptrOffset);
	    *static_cast<TYPE *>(ptr) = val;
	  }
  };

  /// setStatic*Field - Set a field of an object.
  ///
  template <class TYPE>
  void setStaticField(TYPE val) __attribute__ ((noinline)) {
    FieldSetter<TYPE>::setStaticField(this, val);
  }

  /// getInstance*Field - Get an instance field.
  ///
  template<class TYPE>
  TYPE getInstanceField(JavaObject* obj) __attribute__ ((noinline)) {
    llvm_gcroot(obj, 0);
    assert(classDef->isResolved());
    void* ptr = (void*)((uint64)obj + ptrOffset);
    return *static_cast<TYPE *>(ptr);
  }

  /// setInstance*Field - Set an instance field.
  ///
  template<class TYPE>
  void setInstanceField(JavaObject* obj, TYPE val) __attribute__ ((noinline)) {
    llvm_gcroot(obj, 0);
    FieldSetter<TYPE>::setInstanceField(this, obj, val);
  }

#define JavaField_DECL_ASSESSORS(TYPE, TYPE_NAME)			\
	TYPE getStatic##TYPE_NAME##Field() __attribute__ ((noinline));						\
	void setStatic##TYPE_NAME##Field(TYPE val) __attribute__ ((noinline));				\
	TYPE getInstance##TYPE_NAME##Field(JavaObject* obj) __attribute__ ((noinline));	\
	void setInstance##TYPE_NAME##Field(JavaObject* obj, TYPE val) __attribute__ ((noinline));

#define JavaField_IMPL_ASSESSORS(TYPE, TYPE_NAME)																			\
	TYPE JavaField::getStatic##TYPE_NAME##Field() {									\
  		return getStaticField<TYPE>();}												\
	void JavaField::setStatic##TYPE_NAME##Field(TYPE val) {							\
		return setStaticField<TYPE>(val);}											\
	TYPE JavaField::getInstance##TYPE_NAME##Field(JavaObject* obj) {				\
		llvm_gcroot(obj, 0);														\
		return this->getInstanceField<TYPE>(obj);}									\
	void JavaField::setInstance##TYPE_NAME##Field(JavaObject* obj, TYPE val) {		\
		llvm_gcroot(obj, 0);														\
		return this->setInstanceField<TYPE>(obj, val);}

public:
  JavaField_DECL_ASSESSORS(float, Float)
  JavaField_DECL_ASSESSORS(double, Double)
  JavaField_DECL_ASSESSORS(uint8, Int8)
  JavaField_DECL_ASSESSORS(uint16, Int16)
  JavaField_DECL_ASSESSORS(uint32, Int32)
  JavaField_DECL_ASSESSORS(sint64, Long)
  JavaField_DECL_ASSESSORS(JavaObject*, Object)
  
  bool isReference() {
    uint16 val = type->elements[0];
    return (val == '[' || val == 'L');
  }
  
  bool isDouble() {
    return (type->elements[0] == 'D');
  }

  bool isLong() {
    return (type->elements[0] == 'J');
  }

  bool isInt() {
    return (type->elements[0] == 'I');
  }

  bool isFloat() {
    return (type->elements[0] == 'F');
  }

  bool isShort() {
    return (type->elements[0] == 'S');
  }

  bool isChar() {
    return (type->elements[0] == 'C');
  }

  bool isByte() {
    return (type->elements[0] == 'B');
  }

  bool isBoolean() {
    return (type->elements[0] == 'Z');
  }

};


// Specialization for TYPE=JavaObject*
template<>
struct JavaField::FieldSetter<JavaObject*> {

  static void setStaticField(const JavaField* field, JavaObject* val) __attribute__ ((noinline)) {
	llvm_gcroot(val, 0);
	if (val != NULL) assert(val->getVirtualTable());
	assert(field->classDef->isResolved());
	JavaObject** ptr = (JavaObject**)((uint64)field->classDef->getStaticInstance() + field->ptrOffset);
	vmkit::Collector::objectReferenceNonHeapWriteBarrier((gc**)ptr, (gc*)val);
  }

  static void setInstanceField(const JavaField* field, JavaObject* obj, JavaObject* val) __attribute__ ((noinline)) {
	llvm_gcroot(obj, 0);
	llvm_gcroot(val, 0);
	if (val != NULL) assert(val->getVirtualTable());
	assert(field->classDef->isResolved());
	JavaObject** ptr = (JavaObject**)((uint64)obj + field->ptrOffset);
	vmkit::Collector::objectReferenceWriteBarrier((gc*)obj, (gc**)ptr, (gc*)val);
  }
};

template <>
void JavaField::setStaticField(JavaObject* val) __attribute__ ((noinline));

template<>
void JavaField::setInstanceField(JavaObject* obj, JavaObject* val) __attribute__ ((noinline));


} // end namespace j3

#endif
