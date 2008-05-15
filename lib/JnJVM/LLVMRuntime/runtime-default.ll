;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Type definitions ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A virtual table is an array of function pointers.
%VT = type i32**

;;; The type of internal classes. This is not complete, but we only need
;;; the two first fields for now. 
;;; Field 1 - The VT of a class C++ object.
;;; Field 2 - The size of instances of this class.
;;; Field 3 - The VT of instances of this class.
%JavaClass = type { %VT, i32, %VT }

;;; The root of all Java Objects: a VT, a class and a lock.
%JavaObject = type { %VT, %JavaClass*, i8* }

;;; Types for Java arrays. A size of 0 means an undefined size.
%JavaArray = type { %JavaObject, i32 }
%ArrayDouble = type { %JavaObject, i32, [0 x double] }
%ArrayFloat = type { %JavaObject, i32, [0 x float] }
%ArrayLong = type { %JavaObject, i32, [0 x i64] }
%ArrayObject = type { %JavaObject, i32, [0 x %JavaObject*] }
%ArraySInt16 = type { %JavaObject, i32, [0 x i16] }
%ArraySInt32 = type { %JavaObject, i32, [0 x i32] }
%ArraySInt8 = type { %JavaObject, i32, [0 x i8] }
%ArrayUInt16 = type { %JavaObject, i32, [0 x i16] }
%ArrayUInt32 = type { %JavaObject, i32, [0 x i32] }
%ArrayUInt8 = type { %JavaObject, i32, [0 x i8] }

;;; The CacheNode type. The second field is the last called method
%CacheNode = type { %VT, i8*, %JavaClass*, %CacheNode*, %Enveloppe* }

;;; The Enveloppe type, which contains the first cache and all the info
;;; to lookup in the constant pool.
%Enveloppe = type { %VT, %CacheNode*, i8*, i8*, i32 }


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; Constant calls for Jnjvm runtime internal objects field accesses ;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; initialisationCheck - Checks if the class has been initialized and 
;;; initializes if not. This is used for initialization barriers in an isolate
;;; environment, and in some specific scenario in a single environment.
declare %JavaClass* @initialisationCheck(%JavaClass*) readnone 

;;; arrayLength - Get the length of an array.
declare i32 @arrayLength(%JavaObject*) readnone 

;;; getVT - Get the VT of the object.
declare %VT @getVT(%JavaObject*) readnone 

;;; getClass - Get the class of an object
declare %JavaClass* @getClass(%JavaObject*) readnone 

;;; getVTFromClass - Get the VT of a class from its runtime representation.
declare %VT @getVTFromClass(%JavaClass*) readnone 

;;; getObjectSizeFromClass - Get the size of a class from its runtime
;;; representation.
declare i32 @getObjectSizeFromClass(%JavaClass*) readnone 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Generic Runtime methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; virtualLookup - Used for interface calls.
declare i8* @virtualLookup(%CacheNode*, %JavaObject*)

;;; multiCallNew - Allocate multi-dimensional arrays. This will go to allocation
;;; specific methods.
declare %JavaObject* @multiCallNew(%JavaClass*, i32, ...)

;;; forceInitialisationCheck - Force to check initialization. The difference
;;; between this function and the initialisationCheck function is that the
;;; latter is readnone and can thus be removed.
declare void @forceInitialisationCheck(%JavaClass*)

;;; vtableLookup - Look up the offset in a virtual table of a specific
;;; function. This function takes a class and an index to lookup in the
;;; constant pool and returns and sets in the last argument the offset.
declare i32 @vtableLookup(%JavaObject*, %JavaClass*, i32, i32*) readnone 

;;; newLookup - Look up a specific class. The function takes a class and an
;;; index to lookup in the constant pool and returns and sets in the third
;;; argument the found class. The last argument tells if the class has to be
;;; initialized.
declare %JavaClass* @newLookup(%JavaClass*, i32, %JavaClass**, i32) readnone 

;;; fieldLookup - Look up a specific field.
declare i8* @fieldLookup(%JavaObject*, %JavaClass*, i32, i32, i8**, i32*)
  readnone 

;;; JavaObjectAquire - This function is called when starting a synchronized
;;; block or method.
declare void @JavaObjectAquire(%JavaObject*)

;;; JavaObjectRelease - This function is called when leaving a synchronized
;;; block or method.
declare void @JavaObjectRelease(%JavaObject*)

;;; JavaObjectInstanceOf - Returns if a Java object implements the given class.
declare i32 @JavaObjectInstanceOf(%JavaObject*, %JavaClass*) readnone 

;;; getClassDelegatee - Returns the java/lang/Class representation of the
;;; class.
declare %JavaObject* @getClassDelegatee(%JavaClass*) readnone 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Exception methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @nullPointerException()
declare void @classCastException(%JavaObject*, %JavaClass*)
declare void @indexOutOfBoundsException(%JavaObject*, i32)
declare void @negativeArraySizeException(i32)
declare void @outOfMemoryError(i32)

declare void @JavaThreadThrowException(%JavaObject*)
declare void @JavaThreadClearException()
declare i8* @JavaThreadGetException()
declare %JavaObject* @JavaThreadGetJavaException()
declare i1 @JavaThreadCompareException(%JavaClass*)

declare void @jniProceedPendingException()
declare i8* @getSJLJBuffer()

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Debugging methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @printExecution(i32, i32, i32)
declare void @printMethodStart(i32)
declare void @printMethodEnd(i32)
