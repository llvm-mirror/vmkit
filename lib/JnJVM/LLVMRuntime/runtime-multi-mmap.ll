;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;; Collector specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @MarkAndTrace(%JavaObject*, i8*)
declare void @JavaObjectTracer(%JavaObject*, i8*)
declare void @JavaArrayTracer(%JavaObject*, i8*)
declare void @ArrayObjectTracer(%JavaObject*, i8*)
