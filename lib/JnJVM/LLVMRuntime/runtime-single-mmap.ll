;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;; Collector specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @MarkAndTrace(%JavaObject*)
declare void @JavaObjectTracer(%JavaObject*)
declare void @JavaArrayTracer(%JavaObject*)
declare void @ArrayObjectTracer(%JavaObject*)
declare void @RegularObjectTracer(%JavaObject*)
declare void @EmptyTracer(%JavaObject*)
