// GNUClasspath-specific JNI implementation

jobject NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity) {
  JavaObject* res = 0;
  JavaObject* p = 0;
  jobject ret = 0;
  llvm_gcroot(res, 0);
  llvm_gcroot(p, 0);
  llvm_gcroot(ret, 0);

  BEGIN_JNI_EXCEPTION

  JavaThread* th = JavaThread::get();
  Jnjvm* myvm = th->getJVM();
  UserClass* BB = myvm->upcalls->newDirectByteBuffer;

  res = BB->doNew(myvm);

#if ARCH_32
  UserClass* PP = myvm->upcalls->newPointer32;
  p = PP->doNew(myvm);
  myvm->upcalls->dataPointer32->setInstanceInt32Field(p, (uint32)address);
#elif ARCH_64
  UserClass* PP = myvm->upcalls->newPointer64;
  p = PP->doNew(myvm);
  myvm->upcalls->dataPointer64->setInstanceLongField(p, (jlong)address);
#endif

  myvm->upcalls->InitDirectByteBuffer->invokeIntSpecial(myvm, BB, res, 0, &p,
                                                        (uint32)capacity,
                                                        (uint32)capacity, 0);

  ret = (jobject)th->pushJNIRef(res);
  RETURN_FROM_JNI(ret);
  END_JNI_EXCEPTION

  RETURN_FROM_JNI(0);
}


void *GetDirectBufferAddress(JNIEnv *env, jobject _buf) {
  JavaObject* buf = 0;
  JavaObject* address = 0;
  llvm_gcroot(buf, 0);
  llvm_gcroot(address, 0);

  BEGIN_JNI_EXCEPTION

  // Local object references.
  buf = *(JavaObject**)_buf;

  Jnjvm* vm = myVM(env);
  address = vm->upcalls->bufferAddress->getInstanceObjectField(buf);
  if (address != 0) {
#if (__WORDSIZE == 32)
    int res = vm->upcalls->dataPointer32->getInstanceInt32Field(address);
#else
    jlong res = vm->upcalls->dataPointer64->getInstanceLongField(address);
#endif
    RETURN_FROM_JNI((void*)res);
  } else {
    RETURN_FROM_JNI(0);
  }

  END_JNI_EXCEPTION
  RETURN_FROM_JNI(0);
}
