Instruction* JavaJIT::invoke(Value *F, std::vector<llvm::Value*>& args,
                       const char* Name,
                       BasicBlock *InsertAtEnd) {
  
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    return llvm::InvokeInst::Create(F, ifNormal, currentExceptionBlock,
                                    args.begin(), 
                                    args.end(), Name, InsertAtEnd);
  } else {
    return llvm::CallInst::Create(F, args.begin(), args.end(), Name, InsertAtEnd);
  }
}

Instruction* JavaJIT::invoke(Value *F, Value* arg1, const char* Name,
                       BasicBlock *InsertAtEnd) {

  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    Value* arg[1] = { arg1 };
    return InvokeInst::Create(F, ifNormal, currentExceptionBlock,
                              arg, arg + 1, Name, InsertAtEnd);
  } else {
    return CallInst::Create(F, arg1, Name, InsertAtEnd);
  }
}

Instruction* JavaJIT::invoke(Value *F, Value* arg1, Value* arg2,
                       const char* Name, BasicBlock *InsertAtEnd) {

  Value* args[2] = { arg1, arg2 };
  
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    return InvokeInst::Create(F, ifNormal, currentExceptionBlock,
                              args, args + 2, Name, InsertAtEnd);
  } else {
    return CallInst::Create(F, args, args + 2, Name, InsertAtEnd);
  }
}

Instruction* JavaJIT::invoke(Value *F, const char* Name,
                       BasicBlock *InsertAtEnd) {
  // means: is there a handler for me?
  if (currentExceptionBlock != endExceptionBlock) {
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    currentBlock = ifNormal;
    Value* args[1];
    return llvm::InvokeInst::Create(F, ifNormal, currentExceptionBlock,
                                    args, args, Name,
                                    InsertAtEnd);
  } else {
    return llvm::CallInst::Create(F, Name, InsertAtEnd);
  }
}

void JavaJIT::throwException(llvm::Function* F, Value* arg1) {
  if (currentExceptionBlock != endExceptionBlock) {
    Value* exArgs[1] = { arg1 };
    InvokeInst::Create(F, unifiedUnreachable,
                       currentExceptionBlock, exArgs, exArgs + 1,
                       "", currentBlock);
  } else {
    CallInst::Create(F, arg1, "", currentBlock);
    new UnreachableInst(currentBlock);
  }
}

void JavaJIT::throwException(Value* obj) {
  Function* F = module->ThrowExceptionFunction;
  if (currentExceptionBlock != endExceptionBlock) {
    Value* exArgs[1] = { obj };
    InvokeInst::Create(F, unifiedUnreachable,
                       currentExceptionBlock, exArgs, exArgs + 1,
                       "", currentBlock);
  } else {
    CallInst::Create(F, obj, "", currentBlock);
    new UnreachableInst(currentBlock);
  }
}

void JavaJIT::throwException(llvm::Function* F, Value** args,
                             uint32 nbArgs) {
  if (currentExceptionBlock != endExceptionBlock) {
    InvokeInst::Create(F, unifiedUnreachable,
                       currentExceptionBlock, args, args + nbArgs,
                       "", currentBlock);
  } else {
    CallInst::Create(F, args, args + nbArgs, "", currentBlock);
    new UnreachableInst(currentBlock);
  }
}

/// Handler - This class represents an exception handler. It is only needed
/// when parsing the .class file in the JIT, therefore it is only defined
/// here. The readExceptionTable function is the only function that makes
/// use of this class.
struct Handler {
  
  /// startpc - The bytecode number that begins the try clause.
  uint32 startpc;

  /// endpc - The bytecode number that ends the try clause.
  uint32 endpc;

  /// handlerpc - The bytecode number where the handler code starts.
  uint32 handlerpc;

  /// catche - Index in the constant pool of the exception class.
  uint16 catche;

  /// catchClass - The class of the exception: it must always be loaded before
  /// reading the exception table so that we do not throw an exception
  /// when compiling.
  UserClass* catchClass;

  /// catcher - The basic block that catches the exception. The catcher deals
  /// with LLVM codegen and declares the llvm.select method. This block is the
  /// destination of invoke instructions that are in the try clause.
  llvm::BasicBlock* catcher;

  /// tester - The basic block that tests if the exception is handled by this
  /// handler. If the handler is not the first of a list of handlers with the
  /// same range, than this block is the catcher block. Otherwise, it is the
  /// destination of the catcher block and of the handlers that do not handler
  /// the exception.
  llvm::BasicBlock* tester;

  /// javaHandler - The Java code that handles the exception. At this point, we
  /// know we have caught and are handling the exception. The Java exception
  /// object is the PHI node that begins this block.
  llvm::BasicBlock* javaHandler;

  /// nativeHandler - The CXX exception-related code that handles the exception.
  /// The block clears the exception from the execution environment, calls
  /// the CXX begin and end catch methods and jumps to the Java handler.
  llvm::BasicBlock* nativeHandler;

  /// exceptionPHI - The CXX exception object for the tester block. The
  /// tester has incoming blocks, either from the catcher or from other
  /// handlers that don't handle the exception. Therefore each incoming block
  /// specifies the CXX exception object that was caught.
  llvm::PHINode* exceptionPHI;
};

unsigned JavaJIT::readExceptionTable(Reader& reader, uint32 codeLen) {

  // This function uses currentBlock to simplify things. We save the current
  // value of currentBlock to restore it at the end of the function
  BasicBlock* temp = currentBlock;
  
  sint16 nbe = reader.readU2();
  sint16 sync = isSynchro(compilingMethod->access) ? 1 : 0;
  nbe += sync;
 
  // realEndExceptionBlock is the block where handlers will resume if
  // they don't treat the exception. realEndExceptionBlock does not
  // have to catch the exception.
  BasicBlock* realEndExceptionBlock = endExceptionBlock;

  // endExceptionBlockCatcher is the block where every instruction will
  // unwind.
  BasicBlock* endExceptionBlockCatcher = endExceptionBlock;

  if (sync) {
    // synchronizeExceptionBlock is the the block which will release the lock
    // on the object. trySynchronizeExceptionBlock is the block which will
    // catch the exception if one is thrown.
    BasicBlock* synchronizeExceptionBlock = 
          createBasicBlock("synchronizeExceptionBlock");
    BasicBlock* trySynchronizeExceptionBlock = 
          createBasicBlock("trySynchronizeExceptionBlock");

    // So synchronizeExceptionBlock becomes the block where every instructions
    // will unwind.
    realEndExceptionBlock = synchronizeExceptionBlock;
    endExceptionBlockCatcher = trySynchronizeExceptionBlock;
    Value* argsSync = 0;
    if (isVirtual(compilingMethod->access)) {
      argsSync = llvmFunction->arg_begin();
    } else {
      Value* cl = module->getJavaClass(compilingClass);
      argsSync = cl;
    }

    // In the synchronizeExceptionBlock: release the object and go to
    // endExceptionBlock, which will unwind the function.
    
    CallInst::Create(module->ReleaseObjectFunction, argsSync, "",
                     synchronizeExceptionBlock);

    BranchInst::Create(endExceptionBlock, synchronizeExceptionBlock);
   

    // In the trySynchronizeExceptionBlock: catch the exception and move
    // to synchronizeExceptionBlock.

    const PointerType* PointerTy_0 = module->ptrType;
    Instruction* ptr_eh_ptr = CallInst::Create(module->llvmGetException,
                                               "eh_ptr",
                                                trySynchronizeExceptionBlock);
    Constant* C = ConstantExpr::getCast(Instruction::BitCast,
                                        module->personality, PointerTy_0);
    Value* int32_eh_select_params[3] = 
      { ptr_eh_ptr, C, module->constantPtrNull };
    CallInst::Create(module->exceptionSelector, int32_eh_select_params,
                     int32_eh_select_params + 3, "eh_select", 
                     trySynchronizeExceptionBlock);

    BranchInst::Create(synchronizeExceptionBlock,
                       trySynchronizeExceptionBlock);

    // Now we can set the unwind destination of all instructions to
    // the exception catcher.
    for (uint16 i = 0; i < codeLen; ++i) {
      if (opcodeInfos[i].exceptionBlock == endExceptionBlock) {
        opcodeInfos[i].exceptionBlock = trySynchronizeExceptionBlock;
      }
    }
  }
  
  mvm::ThreadAllocator allocator;
  // Loop over all handlers in the bytecode to initialize their values.
  Handler* handlers = (Handler*)
      allocator.Allocate(sizeof(Handler) * (nbe - sync));
  for (uint16 i = 0; i < nbe - sync; ++i) {
    Handler* ex   = &handlers[i];
    ex->startpc   = reader.readU2();
    ex->endpc     = reader.readU2();
    ex->handlerpc = reader.readU2();

    ex->catche = reader.readU2();

#ifndef ISOLATE_SHARING
    if (ex->catche) {
      UserClass* cl = 
        (UserClass*)(compilingClass->ctpInfo->isClassLoaded(ex->catche));
      // When loading the class, we made sure that all exception classes
      // were loaded, so cl must have a value.
      assert(cl && "exception class has not been loaded");
      ex->catchClass = cl;
    } else {
      ex->catchClass = Classpath::newThrowable;
    }
#endif
    
    ex->catcher = createBasicBlock("testException");
    
    // Set the unwind destination of the instructions in the range of this
    // handler to the test block of the handler. If an instruction already has
    // a handler and thus is not the synchronize or regular end handler block,
    // leave it as-is.
    for (uint16 i = ex->startpc; i < ex->endpc; ++i) {
      if (opcodeInfos[i].exceptionBlock == endExceptionBlockCatcher) {
        opcodeInfos[i].exceptionBlock = ex->catcher;
      }
    }

    // If the handler pc does not already have a block, create a new one.
    if (!(opcodeInfos[ex->handlerpc].newBlock)) {
      opcodeInfos[ex->handlerpc].newBlock = createBasicBlock("javaHandler");
    }
    
    // Set the Java handler for this exception.
    ex->javaHandler = opcodeInfos[ex->handlerpc].newBlock;
    opcodeInfos[ex->handlerpc].handler = true;

    // Set the native handler of this exception, which will catch the exception
    // object.
    ex->nativeHandler = createBasicBlock("nativeHandler");
  }

  // Loop over all handlers to know which ones have the same range. Handlers
  // with a same range all verify the exception's class, but only one catches
  // the exception. This is the reason why we have a tester block
  // and a catcher block: the first one tests the exception's class, and the
  // second one catches the exception.
  bool first = true;
  for (sint16 i = 0; i < nbe - sync; ++i) {
    Handler* cur = &handlers[i];

    // If we are the first handler, we must have one block for catching
    // the exception, and one block for comparing the exception. The former
    // is catcher and the latter is tester. Handlers that live in
    // the range of this handler will jump to tester because they
    // have already catched the exception. The other instructions in the range
    // of this handler will jump to catcher because the
    // exception still has to be catched.
    if (first) {
      cur->tester = createBasicBlock("realTestException");
    } else {
      cur->tester = cur->catcher;
    }
   
    // Set the exception as a phi node. This PHI has two types of incoming
    // nodes:
    // - Handlers within the range: they have already catched the exception
    //   and verified its type. They are not the right handler for the
    //   exception, so they jump to this handler
    // - The testException block of this handler (which is unique). It has
    //   catched the exception and is now jumping to perform the test.
    cur->exceptionPHI = PHINode::Create(module->ptrType, "", cur->tester);

    // Look if the next handler has the same range or has a different range.
    // If it's in the same range, then no need to catch the exception.
    // Otherwise, it's a new range and we need to catch the exception.
    if (i + 1 != nbe - sync) {
      Handler* next = &handlers[i + 1];
      
      if (cur->startpc == next->startpc && cur->endpc == next->endpc) {
        first = false;
      } else {
        first = true;
      }
    } 
  }
  

  // Loop over all handlers to implement their catcher and tester.
  for (sint16 i = 0; i < nbe - sync; ++i) {
    Handler* cur = &handlers[i];
    BasicBlock* bbNext = 0;
    PHINode* nodeNext = 0;
    currentExceptionBlock = opcodeInfos[cur->handlerpc].exceptionBlock;

    // Look out where we go if we're not the handler for the exception.
    if (i + 1 != nbe - sync) {
      Handler* next = &handlers[i + 1];
      if (!(cur->startpc >= next->startpc && cur->endpc <= next->endpc)) {
        // If there is no handler to go to (either one that has the same range
        // or one that contains the range), then we jump to the end handler.
        bbNext = realEndExceptionBlock;
      } else {
        // If there's a handler to goto, we jump to its tester block and record
        // the exception PHI node to give our exception to the tester.
        bbNext = next->tester;
        nodeNext = next->exceptionPHI;
      }
    } else {
      // If there's no handler after us, we jump to the end handler.
      bbNext = realEndExceptionBlock;
    }

    // If the tester and the catcher is not the same, then we must implement
    // the catcher. The catcher catches the exception, jumps to the tester
    // and gives the exception as an incoming node the the exceptionPHI.
    if (cur->tester != cur->catcher) {
      const PointerType* PointerTy_0 = module->ptrType;
      Instruction* ptr_eh_ptr = 
        CallInst::Create(module->llvmGetException, "eh_ptr", cur->catcher);
      Constant* C = ConstantExpr::getCast(Instruction::BitCast,
                                          module->personality, PointerTy_0);
      Value* int32_eh_select_params[3] = 
        { ptr_eh_ptr, C, module->constantPtrNull };
      llvm::CallInst::Create(module->exceptionSelector,
                             int32_eh_select_params,
                             int32_eh_select_params + 3, "eh_select",
                             cur->catcher);
      llvm::BranchInst::Create(cur->tester, cur->catcher);
      cur->exceptionPHI->addIncoming(ptr_eh_ptr, cur->catcher);
    } 
    
    currentBlock = cur->tester;
    
    Value* clVar = 0; 
#ifdef ISOLATE_SHARING
    // We're dealing with exceptions, don't catch the exception if the class can
    // not be found.
    if (cur->catche) clVar = getResolvedClass(cur->catche, false, false, 0);
    else clVar = CallInst::Create(module->GetJnjvmExceptionClassFunction,
                                  isolateLocal, "", currentBlock);
#else
    // We know catchClass exists because we have loaded all exceptions catched
    // by the method when we loaded the class that defined this method.
    clVar = module->getNativeClass(cur->catchClass);
#endif
    if (clVar->getType() != module->JavaCommonClassType) 
      clVar = new BitCastInst(clVar, module->JavaCommonClassType, "",
                              currentBlock);

    
#ifdef SERVICE
    // Verifies that the current isolate is not stopped. If it is, we don't
    // catch the exception but resume unwinding.
    JnjvmClassLoader* loader = compilingClass->classLoader;;
    if (loader != loader->bootstrapLoader) {
      Value* Isolate = getVMPtr(getMutatorThreadPtr());
     
      Isolate = new LoadInst(Isolate, "", currentBlock);
      Isolate = new BitCastInst(Isolate, module->ptrPtrType, "", currentBlock);
      Value* Status = GetElementPtrInst::Create(Isolate, module->constantOne, "",
                                                currentBlock);
      Status = new LoadInst(Status, "", currentBlock);
      Status = new PtrToIntInst(Status, Type::Int32Ty, "", currentBlock);
  
      Value* stopping = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, Status,
                                     module->constantOne, "");

      BasicBlock* raiseBlock = createBasicBlock("raiseBlock");
      BasicBlock* continueBlock = createBasicBlock("continueBlock");
      BranchInst::Create(raiseBlock, continueBlock, stopping, currentBlock);
      currentBlock = raiseBlock;
      BranchInst::Create(endExceptionBlock, currentBlock); 

      currentBlock = continueBlock;
    }
#endif
   
		Value* mutatorThreadId = getMutatorThreadPtr();
		Value* javaThreadId = getJavaThreadPtr(mutatorThreadId);
    Value* javaExceptionPtr = getGetJavaExceptionPtr(javaThreadId);
    
    // Get the Java exception.
    Value* obj = new LoadInst(javaExceptionPtr, "", currentBlock);

    Value* objCl = CallInst::Create(module->GetClassFunction, obj, "",
                                    currentBlock);

    Value* depthCl = ConstantInt::get(Type::Int32Ty, cur->catchClass->depth);
    Value* depthClObj = CallInst::Create(module->GetDepthFunction, objCl, "",
                                         currentBlock);
    
    // Compare the exception with the exception class we catch.
    Value* cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_ULE, depthCl,
                              depthClObj, "");

    BasicBlock* supDepth = createBasicBlock("superior depth");
            
    BranchInst::Create(supDepth, bbNext, cmp, currentBlock);
    
    if (nodeNext)
      nodeNext->addIncoming(cur->exceptionPHI, currentBlock);
  
    currentBlock = supDepth;
    Value* inDisplay = CallInst::Create(module->GetDisplayFunction,
                                        objCl, "", currentBlock);
            
    Value* displayArgs[2] = { inDisplay, depthCl };
    Value* clInDisplay = CallInst::Create(module->GetClassInDisplayFunction,
                                          displayArgs, displayArgs + 2, "",
                                          currentBlock);
             
    cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, clInDisplay, clVar,
                       "");
    
    // If we are catching this exception, then jump to the nativeHandler,
    // otherwise jump to our next handler.
    BranchInst::Create(cur->nativeHandler, bbNext, cmp, currentBlock); 

    // Add the incoming value to the next handler, which is the exception we
    // just catched.
    if (nodeNext)
      nodeNext->addIncoming(cur->exceptionPHI, currentBlock);
    
    currentBlock = cur->nativeHandler;
 
		mutatorThreadId = getMutatorThreadPtr();
		javaThreadId = getJavaThreadPtr(mutatorThreadId);
    javaExceptionPtr = getGetJavaExceptionPtr(javaThreadId);
    
    // Get the Java exception.
    Value* exc = new LoadInst(javaExceptionPtr, "", currentBlock);
    Value* cxxExceptionPtr = getCXXExceptionPtr(mutatorThreadId);

    // Clear exceptions.
    new StoreInst(module->constantPtrNull, cxxExceptionPtr, currentBlock);
    new StoreInst(module->JavaObjectNullConstant, javaExceptionPtr,
                  currentBlock);

    // Call the CXX begin and end catcher.
    CallInst::Create(module->exceptionBeginCatch, cur->exceptionPHI,
                           "tmp8", cur->nativeHandler);
    CallInst::Create(module->exceptionEndCatch, "", cur->nativeHandler);

    // We can now jump to the Java handler!
    BranchInst::Create(cur->javaHandler, cur->nativeHandler);

    // If the Java handler is empty, create a PHI node that will contain the
    // exception and give our own.
    if (cur->javaHandler->empty()) {
      PHINode* node = PHINode::Create(JnjvmModule::JavaObjectType, "",
                                      cur->javaHandler);
      node->addIncoming(exc, cur->nativeHandler);
      
    } else {
      // If the Java handler is not empty, then the first instruction is the
      // PHI node. Give it our own.
      Instruction* insn = cur->javaHandler->begin();
      PHINode* node = dyn_cast<PHINode>(insn);
      assert(node && "malformed exceptions");
      node->addIncoming(exc, cur->nativeHandler);
    }


#if defined(SERVICE)

    // Change the isolate we are currently running, now that we have catched
    // the exception: the exception may have been thrown by another isolate.
    Value* mutatorThreadId = 0;
    Value* OldIsolateID = 0;
    Value* IsolateIDPtr = 0;
    Value* OldIsolate = 0;
    Value* NewIsolate = 0;
    Value* IsolatePtr = 0;
    currentBlock = cur->javaHandler;
    if (loader != loader->bootstrapLoader) {
      mutatorThreadId = getGetMutatorThreadPtr();
      IsolateIDPtr = getIsolateIDPtr(mutatorThreadId);
      const Type* realType = PointerType::getUnqual(intrinsics->pointerSizeType);
      IsolateIDPtr = new BitCastInst(IsolateIDPtr, realType, "",
                                     currentBlock);
      OldIsolateID = new LoadInst(IsolateIDPtr, "", currentBlock);

      Value* MyID = ConstantInt::get(intrinsics->pointerSizeType,
                                     loader->getIsolate()->IsolateID);

      new StoreInst(MyID, IsolateIDPtr, currentBlock);
      IsolatePtr = getVMPtr(mutatorThreadPtr);
     
      OldIsolate = new LoadInst(IsolatePtr, "", currentBlock);
      NewIsolate = intrinsics->getIsolate(loader->getIsolate(), currentBlock);
      new StoreInst(NewIsolate, IsolatePtr, currentBlock);

    }
#endif
     
  }
 
  // Restore currentBlock.
  currentBlock = temp;
  return nbe;

}

void JavaJIT::finishExceptions() {
  pred_iterator PI = pred_begin(endExceptionBlock);
  pred_iterator PE = pred_end(endExceptionBlock);
  if (PI == PE) {
    endExceptionBlock->eraseFromParent();
  } else {
    Value* cxxExceptionPtr = getCXXExceptionPtr(getMutatorThreadPtr());
    Value* cxxException = new LoadInst(cxxExceptionPtr, "", currentBlock);
    llvm::CallInst::Create(module->unwindResume, cxxException, "", currentBlock);
    new UnreachableInst(currentBlock);
  }
  
  PI = pred_begin(unifiedUnreachable);
  PE = pred_end(unifiedUnreachable);
  if (PI == PE) {
    unifiedUnreachable->eraseFromParent();
  } else {
    new UnreachableInst(unifiedUnreachable);
  }

}
