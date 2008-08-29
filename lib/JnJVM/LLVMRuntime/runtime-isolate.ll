;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%Jnjvm = type {%VT, %JavaClass*, [9 x %JavaClass*]}


;;; enveloppeLookup - Find the enveloppe for the current user class.
declare i8* @enveloppeLookup(%JavaClass*, i32, ...)

;;; stringLookup - Find the isolate-specific string at the given offset in the
;;; constant pool.
declare i8* @stringLookup(%JavaClass*, i32, ...)

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
