;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific types ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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

;;; The CacheNode type. The second field is the last called method. The
;;; last field is for multi vm environment.
%CacheNode = type { i8*, %JavaClass*, %CacheNode*, %Enveloppe*, i8** }

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; enveloppeLookup - Find the enveloppe for the current user class.
declare i8* @enveloppeLookup(%JavaClass*, i32, ...)

;;; stringLookup - Find the isolate-specific string at the given offset in the
;;; constant pool.
declare i8* @stringLookup(%JavaClass*, i32, ...)

;;; staticCtpLookup - Find the user constant pool at the given offset in the
;;; constant pool.
declare i8* @staticCtpLookup(%JavaClass*, i32, ...)

;;; specialCtpLookup - Find the user constant pool at the given offset in the
;;; constant pool.
declare i8** @specialCtpLookup(i8**, i32, i8**)

;;; getCtpCacheNode - Get the constant pool cache of a cache node. This is a
;;; constant call because the cache node never changes.
declare i8** @getCtpCacheNode(%CacheNode*) readnone

;;; getCtpCacheNode - Get the constant pool cache of a class. This is a
;;; constant call because the constant pool never changes.
declare i8** @getCtpClass(%JavaClass*) readnone

;;; getJnjvmExceptionClass - Get the exception user class for the given
;;; isolate.
declare %JavaClass* @getJnjvmExceptionClass(%Jnjvm*) readnone

;;; getJnjvmArrayClass - Get the array user class of the index, for the given
;;; isolate.
declare %JavaClass* @getJnjvmArrayClass(%Jnjvm*, i32) readnone
