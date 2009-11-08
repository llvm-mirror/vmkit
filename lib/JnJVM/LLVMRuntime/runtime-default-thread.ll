;;; Field 0: the VT of threads
;;; Field 1: next
;;; Field 2: prev
;;; Field 3: IsolateID
;;; Field 4: MyVM
;;; Field 5: baseSP
;;; Field 6: doYield
;;; Field 7: inGC
;;; Field 8: stackScanned
;;; Field 9: lastSP
;;; Field 10: internalThreadID
;;; field 11: routine
;;; field 12: lastKnownFrame
%MutatorThread = type { %VT*, %JavaThread*, %JavaThread*, i8*, i8*, i8*, i1, i1,
                        i1, i8*, i8*, i8*, i8*}
