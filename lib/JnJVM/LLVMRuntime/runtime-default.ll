;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Type definitions ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A virtual table is an array of function pointers.
%VT = type [0 x i32 (...)*]

;;; The root of all Java Objects: a VT, a class and a lock.
%JavaObject = type { %VT*, %JavaCommonClass*, i8* }

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
%CacheNode = type { i8*, %JavaCommonClass*, %CacheNode*, %Enveloppe* }

;;; The Enveloppe type, which contains the first cache and all the info
;;; to lookup in the constant pool.
%Enveloppe = type { %CacheNode*, %UTF8*, %UTF8*, i8, %JavaClass*, %CacheNode }

;;; The task class mirror.
;;; Field 1: The class state
;;; Field 2: The static instance
%TaskClassMirror = type { i32, i8* }

;;; Field 0: the VT of threads
;;; Field 1: next
;;; Field 2: prev
;;; Field 3: IsolateID
;;; Field 4: MyVM
;;; Field 5: baseSP
;;; Field 6: internalThreadID
;;; field 7: routine
;;; field 8: jnienv
;;; field 9: Java pendingException
;;; field 10: CXX pendingException
%JavaThread = type { %VT*, %JavaThread*, %JavaThread*, i8*, i8*, i8*, i8*, i8*,
                     i8*, %JavaObject*, i8* }


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

;;; getVTFromClass - Get the VT of a class from its runtime representation.
declare %VT* @getVTFromClass(%JavaClass*) readnone 

;;; getObjectSizeFromClass - Get the size of a class from its runtime
;;; representation.
declare i32 @getObjectSizeFromClass(%JavaClass*) readnone 

;;; getDisplay - Get the display array of this class.
declare %JavaCommonClass** @getDisplay(%JavaCommonClass*) readnone 

;;; getClassInDisplay - Get the super class at the given offset.
declare %JavaCommonClass* @getClassInDisplay(%JavaCommonClass**, i32) readnone 

;;; getDepth - Get the depth of the class.
declare i32 @getDepth(%JavaCommonClass*) readnone 

;;; getStaticInstance - Get the static instance of this class.
declare i8* @getStaticInstance(%JavaClass*) readnone 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Generic Runtime methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; virtualLookup - Used for interface calls.
declare i8* @jnjvmVirtualLookup(%CacheNode*, %JavaObject*)

;;; multiCallNew - Allocate multi-dimensional arrays. This will go to allocation
;;; specific methods.
declare %JavaObject* @multiCallNew(%JavaCommonClass*, i32, ...)

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

;;; vtableLookup - Look up the offset in a virtual table of a specific
;;; function. This function takes a class and an index to lookup in the
;;; constant pool and returns and stores it in the constant pool cache.
declare i8* @vtableLookup(%JavaClass*, i32, ...)

;;; newLookup - Look up a specific class. The function takes a class and an
;;; index to lookup in the constant pool and returns and stores it in the
;;; constant pool cache.
declare i8* @classLookup(%JavaClass*, i32, ...)

;;; virtualFieldLookup - Look up a specific virtual field.
declare i8* @virtualFieldLookup(%JavaClass*, i32, ...)

;;; staticFieldLookup - Look up a specific static field.
declare i8* @staticFieldLookup(%JavaClass*, i32, ...)

;;; JavaObjectAquire - This function is called when starting a synchronized
;;; block or method.
declare void @JavaObjectAquire(%JavaObject*)

;;; JavaObjectRelease - This function is called when leaving a synchronized
;;; block or method.
declare void @JavaObjectRelease(%JavaObject*)

;;; overflowThinLock - Change a thin lock to a fat lock when the thin lock
;;; overflows
declare void @overflowThinLock(%JavaObject*)

;;; isAssignableFrom - Returns if the objet's class implements the given class.
declare i1 @instanceOf(%JavaObject*, %JavaCommonClass*) readnone 

;;; isAssignableFrom - Returns if the class implements the given class.
declare i1 @isAssignableFrom(%JavaCommonClass*, %JavaCommonClass*) readnone 

;;; implements - Returns if the class implements the given interface.
declare i1 @implements(%JavaCommonClass*, %JavaCommonClass*) readnone 

;;; instantiationOfArray - Returns if the class implements the given array.
declare i1 @instantiationOfArray(%JavaCommonClass*, %JavaCommonClass*) readnone

;;; getClassDelegatee - Returns the java/lang/Class representation of the
;;; class. This method is lowered to the GEP to the class delegatee in
;;; the common class.
declare %JavaObject* @getClassDelegatee(%JavaCommonClass*) readnone 

;;; jnjvmRuntimeDelegatee - Returns the java/lang/Class representation of the
;;; class. This method is called if the class delegatee has not been created
;;; yet.
declare %JavaObject* @jnjvmRuntimeDelegatee(%JavaCommonClass*) readnone 

;;; getArrayClass - Get the array user class of the user class.
declare %JavaCommonClass* @getArrayClass(%JavaCommonClass*, 
                                         %JavaCommonClass**) readnone

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Exception methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare %JavaObject* @jnjvmNullPointerException()
declare %JavaObject* @jnjvmClassCastException(%JavaObject*, %JavaCommonClass*)
declare %JavaObject* @indexOutOfBoundsException(%JavaObject*, i32)
declare %JavaObject* @negativeArraySizeException(i32)
declare %JavaObject* @outOfMemoryError(i32)
declare void @JavaThreadThrowException(%JavaObject*)

declare void @jniProceedPendingException()
declare i8*  @getSJLJBuffer()

declare %JavaObject* @gcmalloc(i32, %VT*)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Debugging methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @printExecution(i32, i32, %JavaMethod*)
declare void @printMethodStart(%JavaMethod*)
declare void @printMethodEnd(%JavaMethod*)
