//===------------ ClassContext.inc -- Implement GetClassContext -----------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

ArrayObject* GetClassContext(uint32 level = 0) {
  ArrayObject* result = NULL;
  JavaObject* delegatee = NULL;
  llvm_gcroot(result, 0);
  llvm_gcroot(delegatee, 0);

  JavaThread* th = JavaThread::get();
  Jnjvm* vm = th->getJVM();
  uint32 length = th->getFrameContextLength();

  vmkit::ThreadAllocator allocator;
  uintptr_t* buffer = (uintptr_t*)allocator.Allocate(length * sizeof(uintptr_t));

  uint32 finalSize = th->getJavaFrameContext((void**)buffer);

  assert(level < finalSize);
  result = (ArrayObject*)
    vm->upcalls->classArrayClass->doNew(finalSize - level, vm);

  for (uint32 i = level; i != finalSize; ++i) {
    JavaMethod* meth = ((JavaMethod**)buffer)[i];
    assert(meth && "Wrong stack trace");
    delegatee = meth->classDef->getClassDelegatee(vm);;
    ArrayObject::setElement(result, delegatee, i - level);
  }

  return result;
}
