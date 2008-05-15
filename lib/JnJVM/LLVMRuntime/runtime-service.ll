;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;; Service specific methods ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; JavaObjectAquireInSharedDomain - This function is called when starting a
;;; synchronized block or method inside a shared method.
declare void @JavaObjectAquireInSharedDomain(%JavaObject*)

;;; JavaObjectReleaseInSharedDomain - This function is called when leaving a 
;;; synchronized block or method inside a shared method.
declare void @JavaObjectReleaseInSharedDomain(%JavaObject*)

;;; serviceCallStart - Mark the switching between services.
declare void @serviceCallStart(i8*, i8*)

;;; serviceCallStop - Mark the switching between services.
declare void @serviceCallStop(i8*, i8*)
