;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific types ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%Jnjvm = type { %VT*, %JavaClass*, [9 x %JavaClass*] }

%JavaCommonClass = type { [32 x %JavaObject*],
                          i16, %JavaClass**, i16, %UTF8*, %JavaClass*, i8*,
                          %VT* }


%JavaClass = type { %JavaCommonClass, i32, i32, [32 x %TaskClassMirror], i8*, 
                    %JavaField*, i16, %JavaField*, i16, %JavaMethod*, i16, 
                    %JavaMethod*, i16, i8*, %ArrayUInt8*, i8*, %Attribut*, 
                    i16, %JavaClass**, i16, %JavaClass*, i16, i8, i8, i32, i32 }

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; j3EnveloppeLookup - Find the enveloppe for the current user class.
declare i8* @j3EnveloppeLookup(%JavaClass*, i32, ...) readnone

;;; j3StaticCtpLookup - Find the user constant pool at the given offset in
;;; the constant pool.
declare i8* @j3StaticCtpLookup(%JavaClass*, i32, ...) readnone

;;; j3SpecialCtpLookup - Find the user constant pool at the given offset in
;;; the constant pool.
declare i8** @j3SpecialCtpLookup(i8**, i32, i8**) readnone

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
