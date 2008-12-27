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

define i8 @llvm_atomic_cmp_swap_i8(i8* %ptr, i8 %cmp, i8 %swap) 
nounwind {
  %A = call i8 @llvm.atomic.cmp.swap.i8.p0i8( i8* %ptr, i8 %cmp, i8 %swap)
  ret i8 %A
}

define i16 @llvm_atomic_cmp_swap_i16(i16* %ptr, i16 %cmp, i16 %swap)
nounwind {
  %A = call i16 @llvm.atomic.cmp.swap.i16.p0i16( i16* %ptr, i16 %cmp, i16 %swap)
  ret i16 %A
}

define i32 @llvm_atomic_cmp_swap_i32(i32* %ptr, i32 %cmp, i32 %swap)
nounwind {
  %A = call i32 @llvm.atomic.cmp.swap.i32.p0i32(i32* %ptr, i32 %cmp, i32 %swap)
  ret i32 %A
}

define i64 @llvm_atomic_cmp_swap_i64(i64* %ptr, i64 %cmp, i64 %swap)
nounwind {
  %A = call i64 @llvm.atomic.cmp.swap.i64.p0i64( i64* %ptr, i64 %cmp, i64 %swap)
  ret i64 %A
}
