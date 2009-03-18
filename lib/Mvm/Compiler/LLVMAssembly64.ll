declare i64 @llvm.atomic.cmp.swap.i64.p0i64(i64*, i64, i64) nounwind

define i64 @llvm_atomic_cmp_swap_i64(i64* %ptr, i64 %cmp, i64 %swap)
nounwind {
  %A = call i64 @llvm.atomic.cmp.swap.i64.p0i64( i64* %ptr, i64 %cmp, i64 %swap)
  ret i64 %A
}
