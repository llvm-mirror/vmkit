;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Type definitions ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A virtual table is an array of function pointers.
%VT = type [0 x i32 (...)*]

;;; The root of all Java Objects: a VT and a lock.
%JavaObject = type { %VT*, i8* }

;;; Types for Java arrays. A size of 0 means an undefined size.
%JavaArray = type { %JavaObject, i8* }
%ArrayDouble = type { %JavaObject, i8*, [0 x double] }
%ArrayFloat = type { %JavaObject, i8*, [0 x float] }
%ArrayLong = type { %JavaObject, i8*, [0 x i64] }
%ArrayObject = type { %JavaObject, i8*, [0 x %JavaObject*] }
%ArraySInt16 = type { %JavaObject, i8*, [0 x i16] }
%ArraySInt32 = type { %JavaObject, i8*, [0 x i32] }
%ArraySInt8 = type { %JavaObject, i8*, [0 x i8] }
%ArrayUInt16 = type { %JavaObject, i8*, [0 x i16] }
%ArrayUInt32 = type { %JavaObject, i8*, [0 x i32] }
%ArrayUInt8 = type { %JavaObject, i8*, [0 x i8] }

;;; The CacheNode type. The first field is the last called method.
%CacheNode = type { i8*, %VT*, %CacheNode*, %Enveloppe* }

;;; The Enveloppe type, which contains the first cache and all the info
;;; to lookup in the constant pool.
%Enveloppe = type { %CacheNode*, %UTF8*, %UTF8*, i32, %JavaClass*, %CacheNode }

;;; The task class mirror.
;;; Field 1: The class state
;;; Field 2: The initialization state
;;; Field 3: The static instance
%TaskClassMirror = type { i8, i1, i8* }

%JavaThread = type { %MutatorThread, i8*, %JavaObject*, i8* }


%Attribut = type { %UTF8*, i32, i32 }

%UTF8 = type { %JavaObject, i8*, [0 x i16] }


%JavaField = type { i8*, i16, %UTF8*, %UTF8*, %Attribut*, i16, %JavaClass*, i32,
                    i16, i8* }

%JavaMethod = type { i8*, i16, %Attribut*, i16, %Enveloppe*, i16, %JavaClass*,
                     %UTF8*, %UTF8*, i8, i8*, i32, i8* }

%JavaClassPrimitive = type { %JavaCommonClass, i32 }
%JavaClassArray = type { %JavaCommonClass, %JavaCommonClass* }

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Constant calls for Jnjvm runtime internal objects field accesses ;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; jnjvmRuntimeInitialiseClass - Initialises the class.
declare %JavaClass* @jnjvmRuntimeInitialiseClass(%JavaClass*)

;;; arrayLength - Get the length of an array.
declare i32 @arrayLength(%JavaObject*) readnone 

;;; getVT - Get the VT of the object.
declare %VT* @getVT(%JavaObject*) readnone 

;;; getClass - Get the class of an object.
declare %JavaCommonClass* @getClass(%JavaObject*) readnone 

;;; getLock - Get the lock of an object.
declare i8* @getLock(%JavaObject*)

;;; getVTFromCommonClass - Get the VT of a class from its runtime
;;; representation.
declare %VT* @getVTFromCommonClass(%JavaCommonClass*) readnone 

;;; getVTFromClass - Get the VT of a class from its runtime representation.
declare %VT* @getVTFromClass(%JavaClass*) readnone 

;;; getVTFromClassArray - Get the VT of an array class from its runtime
;;; representation.
declare %VT* @getVTFromClassArray(%JavaClassArray*) readnone 

;;; getObjectSizeFromClass - Get the size of a class from its runtime
;;; representation.
declare i32 @getObjectSizeFromClass(%JavaClass*) readnone 

;;; getBaseClassVTFromVT - Get the VT of the base class of an array, or the
;;; VT of the array class of a regular class.
declare %VT* @getBaseClassVTFromVT(%VT*) readnone

;;; getDisplay - Get the display array of this VT.
declare %VT** @getDisplay(%VT*) readnone 

;;; getVTInDisplay - Get the super class at the given offset.
declare %VT* @getVTInDisplay(%VT**, i32) readnone 

;;; getDepth - Get the depth of the VT.
declare i32 @getDepth(%VT*) readnone 

;;; getStaticInstance - Get the static instance of this class.
declare i8* @getStaticInstance(%JavaClass*) readnone 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Generic Runtime methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; jnjvmInterfaceLookup - Used for interface calls.
declare i8* @jnjvmInterfaceLookup(%CacheNode*, %JavaObject*)

;;; jnjvmMultiCallNew - Allocate multi-dimensional arrays. This will go to
;;; allocation specific methods.
declare %JavaObject* @jnjvmMultiCallNew(%JavaCommonClass*, i32, ...)

;;; initialisationCheck - Checks if the class has been initialized and 
;;; initializes if not. This is used for initialization barriers in an isolate
;;; environment, and in some specific scenario in a single environment.
declare %JavaClass* @initialisationCheck(%JavaClass*) readnone 

;;; forceInitialisationCheck - Force to check initialization. The difference
;;; between this function and the initialisationCheck function is that the
;;; latter is readnone and can thus be removed. This function is removed
;;; by Jnjvm after the GVN pass, therefore it does not have an actual
;;; implementation.
declare void @forceInitialisationCheck(%JavaClass*)

;;; forceLoadedCheck - Force to check if the class was loaded. Since we do
;;; not want to run Java code in a callback, we have to make sure the class
;;; of the method that we want to compile is loaded. This is used for
;;; the invokespecial bytecode.
declare void @forceLoadedCheck(%JavaCommonClass*)

;;; getConstantPoolAt - Get the value in the constant pool of this class.
;;; This function is removed by Jnjvm after the GVn pass, therefore it does
;;; not have an actual implementation.
declare i8* @getConstantPoolAt(i8* (%JavaClass*, i32, ...)*, i8**,
                               %JavaClass*, i32, ...) readnone

;;; jnjvmVirtualTableLookup - Look up the offset in a virtual table of a
;;; specific function. This function takes a class and an index to lookup in the
;;; constant pool and returns and stores it in the constant pool cache.
declare i8* @jnjvmVirtualTableLookup(%JavaClass*, i32, ...)

;;; jnjvmClassLookup - Look up a specific class. The function takes a class and
;;; an index to lookup in the constant pool and returns and stores it in the
;;; constant pool cache.
declare i8* @jnjvmClassLookup(%JavaClass*, i32, ...)

;;; jnjvmVirtualFieldLookup - Look up a specific virtual field.
declare i8* @jnjvmVirtualFieldLookup(%JavaClass*, i32, ...)

;;; jnjvmStaticFieldLookup - Look up a specific static field.
declare i8* @jnjvmStaticFieldLookup(%JavaClass*, i32, ...)

;;; jnjvmStringLookup - Find the isolate-specific string at the given offset in
;;; the constant pool.
declare i8* @jnjvmStringLookup(%JavaClass*, i32, ...) readnone

;;; jnjvmJavaObjectAquire - This function is called when starting a synchronized
;;; block or method.
declare void @jnjvmJavaObjectAquire(%JavaObject*)

;;; jnjvmJavaObjectRelease - This function is called when leaving a synchronized
;;; block or method.
declare void @jnjvmJavaObjectRelease(%JavaObject*)

;;; jnjvmOverflowThinLock - Change a thin lock to a fat lock when the thin lock
;;; overflows
declare void @jnjvmOverflowThinLock(%JavaObject*)

;;; isAssignableFrom - Returns if a type is a subtype of another type.
declare i1 @isAssignableFrom(%VT*, %VT*) readnone

;;; isSecondaryClass - Returns if a type is a secondary super type of
;;; another type.
declare i1 @isSecondaryClass(%VT*, %VT*) readnone

;;; getClassDelegatee - Returns the java/lang/Class representation of the
;;; class. This method is lowered to the GEP to the class delegatee in
;;; the common class.
declare %JavaObject* @getClassDelegatee(%JavaCommonClass*) readnone 

;;; jnjvmRuntimeDelegatee - Returns the java/lang/Class representation of the
;;; class. This method is called if the class delegatee has not been created
;;; yet.
declare %JavaObject* @jnjvmRuntimeDelegatee(%JavaCommonClass*) readnone 

;;; jnjvmGetArrayClass - Get the array user class of the user class.
declare %JavaClassArray* @jnjvmGetArrayClass(%JavaCommonClass*, 
                                             %JavaClassArray**) readnone

declare i8 @getFinalInt8Field(i8*) readnone
declare i16 @getFinalInt16Field(i16*) readnone
declare i32 @getFinalInt32Field(i32*) readnone
declare i64 @getFinalLongField(i64*) readnone
declare double @getFinalDoubleField(double*) readnone
declare float @getFinalFloatField(float*) readnone
declare %JavaObject* @getFinalObjectField(%JavaObject**) readnone

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Exception methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare %JavaObject* @jnjvmNullPointerException()
declare %JavaObject* @jnjvmClassCastException(%JavaObject*, %JavaCommonClass*)
declare %JavaObject* @jnjvmIndexOutOfBoundsException(%JavaObject*, i32)
declare %JavaObject* @jnjvmNegativeArraySizeException(i32)
declare %JavaObject* @jnjvmOutOfMemoryError(i32)
declare %JavaObject* @jnjvmStackOverflowError()
declare %JavaObject* @jnjvmArrayStoreException(%VT*)
declare %JavaObject* @jnjvmArithmeticException()
declare void @jnjvmThrowException(%JavaObject*)
declare void @jnjvmThrowExceptionFromJIT()

declare void @jnjvmEndJNI(i32**, i8**)
declare void @jnjvmStartJNI(i32*, i32**, i8*, i8**, i8*)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Debugging methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @jnjvmPrintExecution(i32, i32, %JavaMethod*)
declare void @jnjvmPrintMethodStart(%JavaMethod*)
declare void @jnjvmPrintMethodEnd(%JavaMethod*)
