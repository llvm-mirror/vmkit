;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Common types ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A virtual table is an array of function pointers.
%VT = type [0 x i32 (...)*]

%CircularBase = type { %VT*, %CircularBase*, %CircularBase* }

;;; Field 0:  the parent (circular base)
;;; Field 1:  bool   doYield
;;; field 2:  void*  lastExceptionBuffer/internalPendingException
;;; field 3:  void*  vmData
;;; field 4:  gc*    pendingException
;;; field 5:  VMkit* vmkit
;;; Field 6:  void*  MyVM
;;; Field 7:  void*  baseSP
;;; Field 8:  bool   inRV
;;; Field 9:  bool   joinedRV
;;; Field 10: void*  lastSP
;;; Field 11: void*  internalThreadID
;;; field 12: void*  routine
;;; field 13: void*  lastKnownFrame
;;; field 14: void*  allVMDatas
%Thread       = type { %CircularBase, i1, i8*, i8*, i8*, i8*, i8*, i8*, i1, i1, i8*, i8*, i8*, i8*, i8* }

;;; field 0: VT
;;; field 1: mvm::MutatorThread*  mut
;;; field 2: mvm::VirtualMachine* vm
%VMThreadData = type { %VT*, %MutatorThread*, i8* }

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Printing functions ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @printFloat(float)
declare void @printDouble(double)
declare void @printLong(i64)
declare void @printInt(i32)
declare void @printObject(i8*)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Exceptions ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @_Unwind_Resume_or_Rethrow(i8*)
declare i8*  @llvm.eh.exception() nounwind
declare i32  @llvm.eh.selector.i32(i8*, i8*, ...) nounwind
declare i64  @llvm.eh.selector.i64(i8*, i8*, ...) nounwind
declare void @__gxx_personality_v0()
declare i8*  @__cxa_begin_catch(i8*)
declare void @__cxa_end_catch()
declare i32  @setjmp(i8*)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Math ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare double @llvm.sqrt.f64(double) nounwind
declare double @llvm.sin.f64(double) nounwind
declare double @llvm.cos.f64(double) nounwind
declare double @tan(double)
declare double @asin(double)
declare double @acos(double)
declare double @atan(double)
declare double @exp(double)
declare double @log(double)
declare double @ceil(double)
declare double @floor(double)
declare double @cbrt(double)
declare double @cosh(double)
declare double @expm1(double)
declare double @log10(double)
declare double @log1p(double)
declare double @sinh(double)
declare double @tanh(double)
declare double @fabs(double)
declare double @rint(double)
declare double @hypot(double, double)
declare double @pow(double, double)
declare double @atan2(double, double)
declare float  @fabsf(float)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Memory ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @llvm.memcpy.i32(i8 *, i8 *, i32, i32) nounwind
declare void @llvm.memset.i32(i8 *, i8, i32, i32) nounwind
declare i8*  @llvm.frameaddress(i32) nounwind readnone

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Atomic ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare i8  @llvm.atomic.cmp.swap.i8.p0i8(i8*, i8, i8) nounwind
declare i16 @llvm.atomic.cmp.swap.i16.p0i16(i16*, i16, i16) nounwind
declare i32 @llvm.atomic.cmp.swap.i32.p0i32(i32*, i32, i32) nounwind
declare i64 @llvm.atomic.cmp.swap.i64.p0i64(i64*, i64, i64) nounwind

declare void @unconditionalSafePoint() nounwind
declare void @conditionalSafePoint() nounwind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; GC ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

declare void @llvm.gcroot(i8**, i8*)
declare i8* @gcmalloc(i32, i8*)
declare i8* @gcmallocUnresolved(i32, i8*)
declare void @addFinalizationCandidate(i8*)
