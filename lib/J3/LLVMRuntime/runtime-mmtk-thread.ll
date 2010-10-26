%BumpPtrAllocator = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8* }

;;; Field 0: the thread
;;; field 1: allocator
;;; field 2: MutatorContext
;;; field 3: realRoutine
%MutatorThread = type { %Thread, %BumpPtrAllocator, i8*, i8* }
