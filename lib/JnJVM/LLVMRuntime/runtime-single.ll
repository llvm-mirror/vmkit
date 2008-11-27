;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific types ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; The type of internal classes. 
;;; Field 1 - The VT of a class C++ object.
;;; Field 2 - The size of instances of this class.
;;; Field 3 - The VT of instances of this class.
;;; Field 4 - The list of super classes of this class.
;;; Field 5 - The depth of the class in its super hierarchy.
;;; Field 6 - The class state (resolved, initialized, ...)

%JavaClass = type { %VT, i32, %VT, %JavaClass**, i32, i32, i32, i8, i8, 
                    %Class**, i16, %UTF8*, %Class*, i8*, %JavaField*, i16, 
                    %JavaField*, i16, %JavaMethod*, i16, %JavaMethod*, i16, 
                    i8*, i8*, %JavaObject* }


%Class = type { %JavaClass, %ArrayUInt8*, i8*, %Attribut*, i16, %Class**, i16,
                %Class*, i16, i8, i32, i32, void (i8*)*, i8*, i8* }


%Attribut = type { %UTF8*, i32, i32 }

%UTF8 = type { %ArrayUInt16 }


%JavaField = type { i8*, i16, %UTF8*, %UTF8*, %Attribut*, i16, %Class*, i64,
                    i16, i8* }

%JavaMethod = type { i8*, i16, %Attribut*, i16, %Enveloppe*, i16, %Class*,
                     %UTF8*, %UTF8*, i8, i8*, i32, i8* }

;;; The CacheNode type. The second field is the last called method.
%CacheNode = type { i8*, %JavaClass*, %CacheNode*, %Enveloppe*}
