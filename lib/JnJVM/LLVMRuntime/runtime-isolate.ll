;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;; getStaticInstance - Get the static instance of a class.
declare %JavaObject* @getStaticInstance(%JavaClass*, i8*) readnone 

;;; runtimeUTF8ToStr - Convert the UTF8 in a string.
declare %JavaObject* @runtimeUTF8ToStr(%ArraySInt16*)
