;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Common types ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; A virtual table is an array of function pointers.
%VT = type [0 x i32 (...)*]

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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;Helper functions for gcc < 4.2 ;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i8 @runtime.llvm.atomic.cmp.swap.i8(i8* %ptr, i8 %cmp, i8 %swap) 
nounwind {
  %A = call i8 @llvm.atomic.cmp.swap.i8.p0i8( i8* %ptr, i8 %cmp, i8 %swap)
  ret i8 %A
}

define i16 @runtime.llvm.atomic.cmp.swap.i16(i16* %ptr, i16 %cmp, i16 %swap)
nounwind {
  %A = call i16 @llvm.atomic.cmp.swap.i16.p0i16( i16* %ptr, i16 %cmp, i16 %swap)
  ret i16 %A
}

define i32 @runtime.llvm.atomic.cmp.swap.i32(i32* %ptr, i32 %cmp, i32 %swap)
nounwind {
  %A = call i32 @llvm.atomic.cmp.swap.i32.p0i32(i32* %ptr, i32 %cmp, i32 %swap)
  ret i32 %A
}

define i64 @runtime.llvm.atomic.cmp.swap.i64(i64* %ptr, i64 %cmp, i64 %swap)
nounwind {
  %A = call i64 @llvm.atomic.cmp.swap.i64.p0i64( i64* %ptr, i64 %cmp, i64 %swap)
  ret i64 %A
}
