;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Type definitions ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A virtual table is an array of function pointers.
%VT = type i32 (...)**

%Jnjvm = type {%VT, %JavaClass*, [9 x %JavaClass*]}

;;; The task class mirror in an isolate environment.
;;; Field 1: The class state
;;; Field 2: The class delegatee (java/lang/Class)
;;; Field 3: The static instance
%TaskClassMirror = type { i32, %JavaObject*, i8* }

;;; The type of internal classes. This is not complete, but we only need
;;; the first fields for now. 
;;; Field 1 - The VT of a class C++ object.
;;; Field 2 - The size of instances of this class.
;;; Field 3 - The VT of instances of this class.
;;; Field 4 - The list of super classes of this class.
;;; Field 5 - The depth of the class in its super hierarchy.
;;; Field 6 - The class state (resolved, initialized, ...)
;;; field 7 - The task class mirror, for an isolate environment
%JavaClass = type { %VT, i32, %VT ,%JavaClass**, i32, i32,
                    [0 x %TaskClassMirror] }

;;; The root of all Java Objects: a VT, a class and a lock.
%JavaObject = type { %VT, %JavaClass*, i8* }

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

;;; The CacheNode type. The second field is the last called method. The
;;; last field is for multi vm environment.
%CacheNode = type { i8*, %JavaClass*, %CacheNode*, %Enveloppe*, i8** }

;;; The Enveloppe type, which contains the first cache and all the info
;;; to lookup in the constant pool.
%Enveloppe = type { %CacheNode*, i8**, i8*, i32 }

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Constant calls for Jnjvm runtime internal objects field accesses ;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; jnjvmRuntimeInitialiseClass - Initialises the class.
declare %JavaClass* @jnjvmRuntimeInitialiseClass(%JavaClass*) readnone 

;;; arrayLength - Get the length of an array.
declare i32 @arrayLength(%JavaObject*) readnone 

;;; getVT - Get the VT of the object.
declare %VT @getVT(%JavaObject*) readnone 

;;; getClass - Get the class of an object.
declare %JavaClass* @getClass(%JavaObject*) readnone 

;;; getLock - Get the lock of an object.
declare i8* @getLock(%JavaObject*)

;;; getVTFromClass - Get the VT of a class from its runtime representation.
declare %VT @getVTFromClass(%JavaClass*) readnone 

;;; getObjectSizeFromClass - Get the size of a class from its runtime
;;; representation.
declare i32 @getObjectSizeFromClass(%JavaClass*) readnone 

;;; getDisplay - Get the display array of this class.
declare %JavaClass** @getDisplay(%JavaClass*) readnone 

;;; getClassInDisplay - Get the super class at the given offset.
declare %JavaClass* @getClassInDisplay(%JavaClass**, i32) readnone 

;;; getDepth - Get the depth of the class.
declare i32 @getDepth(%JavaClass*) readnone 

;;; getStaticInstance - Get the static instance of this class.
declare i8* @getStaticInstance(%JavaClass*) readnone 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Generic Runtime methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; virtualLookup - Used for interface calls.
declare i8* @jnjvmVirtualLookup(%CacheNode*, %JavaObject*)

;;; multiCallNew - Allocate multi-dimensional arrays. This will go to allocation
;;; specific methods.
declare %JavaObject* @multiCallNew(%JavaClass*, i32, ...)

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
declare i1 @instanceOf(%JavaObject*, %JavaClass*) readnone 

;;; isAssignableFrom - Returns if the class implements the given class.
declare i1 @isAssignableFrom(%JavaClass*, %JavaClass*) readnone 

;;; implements - Returns if the class implements the given interface.
declare i1 @implements(%JavaClass*, %JavaClass*) readnone 

;;; instantiationOfArray - Returns if the class implements the given array.
declare i1 @instantiationOfArray(%JavaClass*, %JavaClass*) readnone 

;;; getClassDelegatee - Returns the java/lang/Class representation of the
;;; class.
declare %JavaObject* @getClassDelegatee(%JavaClass*) readnone 

;;; getArrayClass - Get the array user class of the user class.
declare %JavaClass* @getArrayClass(%JavaClass*, %JavaClass**) readnone

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Exception methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @jnjvmNullPointerException()
declare void @jnjvmClassCastException(%JavaObject*, %JavaClass*)
declare void @indexOutOfBoundsException(%JavaObject*, i32)
declare void @negativeArraySizeException(i32)
declare void @outOfMemoryError(i32)

declare void         @JavaThreadThrowException(%JavaObject*)
declare void         @JavaThreadClearException()
declare i8*          @JavaThreadGetException()
declare %JavaObject* @JavaThreadGetJavaException()
declare i1           @JavaThreadCompareException(%JavaClass*)

declare void @jniProceedPendingException()
declare i8*  @getSJLJBuffer()

declare %JavaObject* @gcmalloc(i32, %VT)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Debugging methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @printExecution(i8*, i32, i8*)
declare void @printMethodStart(i8*)
declare void @printMethodEnd(i8*)
