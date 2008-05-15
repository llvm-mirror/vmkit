;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;; Collector specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @MarkAndTrace(%JavaObject*, i8*)
declare void @JavaObjectTracer(%JavaObject*, i8*)
declare %JavaObject* @gcmalloc(i32, %VT, i8*)
declare i8* @getCollector(i8*) readnone
