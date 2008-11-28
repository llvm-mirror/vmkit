;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific types ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%Jnjvm = type {%VT, %JavaClass*, [9 x %JavaClass*]}

%JavaCommonClass = type { %JavaCommonClass**, i32, [32 x %JavaObject*],
                          i16, %JavaClass**, i16, %UTF8*, %JavaClass*, i8* }


%JavaClass = type { %JavaCommonClass, i32, %VT, [32 x %TaskClassMirror], i8*, 
                    %JavaField*, i16, %JavaField*, i16, %JavaMethod*, i16, 
                    %JavaMethod*, i16, i8*, %ArrayUInt8*, i8*, %Attribut*, 
                    i16, %JavaClass**, i16, %JavaClass*, i16, i8, i32, i32, i8*,
                    void (i8*)* }

;;; The CacheNode type. The second field is the last called method. The
;;; last field is for multi vm environment.
%CacheNode = type { i8*, %JavaCommonClass*, %CacheNode*, %Enveloppe*, i8** }

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
