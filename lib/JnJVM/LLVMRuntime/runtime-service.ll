;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Service specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; jnjvmServiceCallStart - Mark the switching between services.
declare void @jnjvmServiceCallStart(i8*, i8*)

;;; jnjvmServiceCallStop - Mark the switching between services.
declare void @jnjvmServiceCallStop(i8*, i8*)
