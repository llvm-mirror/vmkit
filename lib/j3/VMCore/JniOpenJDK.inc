// OpenJDK-specific JNI implementation

jobject NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity) {
  JavaObject* res = 0;
  llvm_gcroot(res, 0);

  BEGIN_JNI_EXCEPTION

  JavaThread* th = JavaThread::get();
  Jnjvm* myvm = th->getJVM();
  UserClass* BB = myvm->upcalls->newDirectByteBuffer;

  res = BB->doNew(myvm);

  myvm->upcalls->InitDirectByteBuffer->invokeIntSpecial(myvm, BB, res,
    (uint64_t)(uintptr_t)address, (int)capacity);

  jobject ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);
  END_JNI_EXCEPTION

  RETURN_FROM_JNI(0);
}

void *GetDirectBufferAddress(JNIEnv *env, jobject _buf) {
  JavaObject* buf = 0;
  llvm_gcroot(buf, 0);

  BEGIN_JNI_EXCEPTION

  // Local object references.
  buf = *(JavaObject**)_buf;
  jlong address;

  Jnjvm* vm = myVM(env);
  address = vm->upcalls->bufferAddress->getInstanceLongField(buf);
  RETURN_FROM_JNI((void*)address);

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}
