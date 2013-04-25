;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;; Isolate specific types ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%JavaCommonClass = type { [256 x %JavaObject*], i32,
                          %JavaClass**, i16, %UTF8*, %JavaClass*, i8*, %VT* }

%ClassBytes = type { i32, i8* }

%JavaClass = type { %JavaCommonClass, i32, i32, [256 x %TaskClassMirror],
                    %JavaField*, i16, %JavaField*, i16, %JavaMethod*, i16,
                    %JavaMethod*, i16, i8*, %ClassBytes*, %JavaConstantPool*, %Attribute*,
                    i16, %JavaClass**, i16, %JavaClass*, i16, i8, i8, i32, i32 }

declare i32 @j3SetIsolate(i32, i32*)
