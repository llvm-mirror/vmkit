// DefineClass.inc

UserClass* defineClass(JnjvmClassLoader * JCL, const UTF8* className,
  const char* buf, size_t bufLen) {
  JavaObject* excp = 0;
  llvm_gcroot(excp, 0);

  Jnjvm* vm = JavaThread::get()->getJVM();
  ClassBytes * classBytes = new (JCL->allocator, bufLen) ClassBytes(bufLen);
  memcpy(classBytes->elements, buf, bufLen);
  Reader reader(classBytes);

  // Before creating a class, do a check on the bytes.
  uint32 magic = reader.readU4();
  if (magic != Jnjvm::Magic) {
    JavaThread::get()->getJVM()->classFormatError("bad magic number");
    UNREACHABLE();
  }

  // Also check that we don't already have a class for this name
  if (JCL->lookupClass(className)) {
    excp = vm->CreateLinkageError("duplicate class definition");
    JavaThread::get()->throwException(excp);
    UNREACHABLE();
  }

  // Construct the class
  UserClass* cl = JCL->constructClass(className, classBytes);
  cl->resolveClass();
  return cl;
}
