Instruction* JavaJIT::invoke(Value *F, std::vector<llvm::Value*>& args,
                       const char* Name,
                       BasicBlock *InsertAtEnd) {
  
  Instruction* res = CallInst::Create(F, args.begin(), args.end(), Name,
                                      InsertAtEnd);
  DebugLoc DL = CreateLocation();
  res->setDebugLoc(DL);
  
  if (TheCompiler->hasExceptionsEnabled()) {
    Value* threadId = getCurrentThread(intrinsics->JavaThreadType);
    Value* geps[2] = { intrinsics->constantZero,
                       intrinsics->OffsetJavaExceptionInJavaThreadConstant };

    Value* javaExceptionPtr = GetElementPtrInst::Create(threadId, geps,
                                                        geps + 2, "",
                                                        currentBlock);
    
    // Get the Java exception.
    Value* obj = 0;
    
    BasicBlock* ifNormal = createBasicBlock("no exception block");
  
    Value* test = 0;
    Constant* zero = intrinsics->JavaObjectNullConstant;

    // If F is a runtime intrinsic that does not access memory, use a hack
    // that will prevent LLVM from moving the exception check: runtime
    // intrinsics return the exception if an exception was raised.
    if (F == intrinsics->InitialisationCheckFunction || 
        F == intrinsics->GetConstantPoolAtFunction ||
        F == intrinsics->GetArrayClassFunction ||
        F == intrinsics->GetClassDelegateeFunction) {
      // Make the load volatile to force the instruction after the call.
      // Otherwise, LLVM will merge the load with a previous load because
      // the function is readnone.
      obj = new LoadInst(javaExceptionPtr, "", TheCompiler->useCooperativeGC(), currentBlock);
      test = new BitCastInst(res, intrinsics->JavaObjectType, "", currentBlock);
      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, test, obj, "");
      Value* T = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, obj, zero, "");
      test = BinaryOperator::CreateAnd(test, T, "", currentBlock);
    } else {
      obj = new LoadInst(javaExceptionPtr, "", currentBlock);
      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, obj, zero, "");
    }

    BranchInst::Create(currentExceptionBlock, ifNormal, test, currentBlock);

    
    if (!currentExceptionBlock->empty()) {
      Instruction* insn = currentExceptionBlock->begin();
      PHINode* node = dyn_cast<PHINode>(insn);
      if (node) node->addIncoming(obj, currentBlock);
    }
  
    currentBlock = ifNormal; 
  }

  return res;
}

Instruction* JavaJIT::invoke(Value *F, Value* arg1, const char* Name,
                       BasicBlock *InsertAtEnd) {

  Instruction* res = CallInst::Create(F, arg1, Name, InsertAtEnd);
  DebugLoc DL = CreateLocation();
  res->setDebugLoc(DL);
  
  if (TheCompiler->hasExceptionsEnabled()) {
    Value* threadId = getCurrentThread(intrinsics->JavaThreadType);
    Value* geps[2] = { intrinsics->constantZero,
                       intrinsics->OffsetJavaExceptionInJavaThreadConstant };

    Value* javaExceptionPtr = GetElementPtrInst::Create(threadId, geps,
                                                        geps + 2, "",
                                                        currentBlock);
    
    // Get the Java exception.
    Value* obj = 0;
    
    BasicBlock* ifNormal = createBasicBlock("no exception block");
  
    Value* test = 0;
    Constant* zero = intrinsics->JavaObjectNullConstant;
    if (F == intrinsics->InitialisationCheckFunction || 
        F == intrinsics->GetConstantPoolAtFunction ||
        F == intrinsics->GetArrayClassFunction ||
        F == intrinsics->GetClassDelegateeFunction) {
      obj = new LoadInst(javaExceptionPtr, "", TheCompiler->useCooperativeGC(), currentBlock);
      test = new BitCastInst(res, intrinsics->JavaObjectType, "", currentBlock);
      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, test, obj, "");
      Value* T = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, obj, zero, "");
      test = BinaryOperator::CreateAnd(test, T, "", currentBlock);
    } else {
      obj = new LoadInst(javaExceptionPtr, "", currentBlock);
      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, obj, zero, "");
    }

    BranchInst::Create(currentExceptionBlock, ifNormal, test, currentBlock);
 
    if (!currentExceptionBlock->empty()) {
      Instruction* insn = currentExceptionBlock->begin();
      PHINode* node = dyn_cast<PHINode>(insn);
      if (node) node->addIncoming(obj, currentBlock);
    }
  
    currentBlock = ifNormal;
  }

  return res;
}

Instruction* JavaJIT::invoke(Value *F, Value* arg1, Value* arg2,
                       const char* Name, BasicBlock *InsertAtEnd) {

  Value* args[2] = { arg1, arg2 };
  
  Instruction* res = CallInst::Create(F, args, args + 2, Name, InsertAtEnd);
  DebugLoc DL = CreateLocation();
  res->setDebugLoc(DL);
  
  if (TheCompiler->hasExceptionsEnabled()) {
    Value* threadId = getCurrentThread(intrinsics->JavaThreadType);
    Value* geps[2] = { intrinsics->constantZero,
                       intrinsics->OffsetJavaExceptionInJavaThreadConstant };

    Value* javaExceptionPtr = GetElementPtrInst::Create(threadId, geps,
                                                        geps + 2, "",
                                                        currentBlock);
    
    // Get the Java exception.
    Value* obj = 0;
    
    BasicBlock* ifNormal = createBasicBlock("no exception block");
    
    Value* test = 0;
    Constant* zero = intrinsics->JavaObjectNullConstant;
    if (F == intrinsics->InitialisationCheckFunction || 
        F == intrinsics->GetConstantPoolAtFunction ||
        F == intrinsics->GetArrayClassFunction ||
        F == intrinsics->GetClassDelegateeFunction) {
      obj = new LoadInst(javaExceptionPtr, "", TheCompiler->useCooperativeGC(), currentBlock);
      test = new BitCastInst(res, intrinsics->JavaObjectType, "", currentBlock);
      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, test, obj, "");
      Value* T = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, obj, zero, "");
      test = BinaryOperator::CreateAnd(test, T, "", currentBlock);
    } else {
      obj = new LoadInst(javaExceptionPtr, "", currentBlock);
      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, obj, zero, "");
    }
  
    BranchInst::Create(currentExceptionBlock, ifNormal, test, currentBlock);
  
    if (!currentExceptionBlock->empty()) {
      Instruction* insn = currentExceptionBlock->begin();
      PHINode* node = dyn_cast<PHINode>(insn);
      if (node) node->addIncoming(obj, currentBlock);
    }

    currentBlock = ifNormal;
  }

  return res;
}

Instruction* JavaJIT::invoke(Value *F, const char* Name,
                       BasicBlock *InsertAtEnd) {
  Instruction* res = llvm::CallInst::Create(F, Name, InsertAtEnd);
  DebugLoc DL = CreateLocation();
  res->setDebugLoc(DL);
  
  if (TheCompiler->hasExceptionsEnabled()) {
    Value* threadId = getCurrentThread(intrinsics->JavaThreadType);
    Value* geps[2] = { intrinsics->constantZero,
                       intrinsics->OffsetJavaExceptionInJavaThreadConstant };

    Value* javaExceptionPtr = GetElementPtrInst::Create(threadId, geps,
                                                        geps + 2, "",
                                                        currentBlock);
    
    // Get the Java exception.
    Value* obj = 0;
    
    BasicBlock* ifNormal = createBasicBlock("no exception block");
  
    Value* test = 0;
    Constant* zero = intrinsics->JavaObjectNullConstant;
    if (F == intrinsics->InitialisationCheckFunction || 
        F == intrinsics->GetConstantPoolAtFunction ||
        F == intrinsics->GetArrayClassFunction ||
        F == intrinsics->GetClassDelegateeFunction) {
      obj = new LoadInst(javaExceptionPtr, "", TheCompiler->useCooperativeGC(), currentBlock);
      test = new BitCastInst(res, intrinsics->JavaObjectType, "", currentBlock);
      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, test, obj, "");
      Value* T = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, obj, zero, "");
      test = BinaryOperator::CreateAnd(test, T, "", currentBlock);
    } else {
      obj = new LoadInst(javaExceptionPtr, "", currentBlock);
      test = new ICmpInst(*currentBlock, ICmpInst::ICMP_NE, obj, zero, "");
    }

    BranchInst::Create(currentExceptionBlock, ifNormal, test, currentBlock);
  
    if (!currentExceptionBlock->empty()) {
      Instruction* insn = currentExceptionBlock->begin();
      PHINode* node = dyn_cast<PHINode>(insn);
      if (node) node->addIncoming(obj, currentBlock);
    }

    currentBlock = ifNormal;
  }

  return res;
}

void JavaJIT::throwException(llvm::Function* F, Value* arg1) {
  Instruction* obj = CallInst::Create(F, arg1, "", currentBlock);
  DebugLoc DL = CreateLocation();
  obj->setDebugLoc(DL);

  if (currentExceptionBlock != endExceptionBlock) {
    Instruction* insn = currentExceptionBlock->begin();
    PHINode* node = dyn_cast<PHINode>(insn);
    if (node) node->addIncoming(obj, currentBlock);
    BranchInst::Create(currentExceptionBlock, currentBlock);
  } else {
    if (endNode) {
      endNode->addIncoming(Constant::getNullValue(endNode->getType()),
                           currentBlock);
    }
    BranchInst::Create(endBlock, currentBlock);
  }
}

void JavaJIT::throwException(Value* obj) {
  JITVerifyNull(obj);
  Value* threadId = getCurrentThread(intrinsics->JavaThreadType);
  Value* geps[2] = { intrinsics->constantZero,
                     intrinsics->OffsetJavaExceptionInJavaThreadConstant };

  Value* javaExceptionPtr = GetElementPtrInst::Create(threadId, geps,
                                                      geps + 2, "",
                                                      currentBlock);
  new StoreInst(obj, javaExceptionPtr, currentBlock);
  if (currentExceptionBlock != endExceptionBlock) {
    Instruction* insn = currentExceptionBlock->begin();
    PHINode* node = dyn_cast<PHINode>(insn);
    if (node) node->addIncoming(obj, currentBlock);
    BranchInst::Create(currentExceptionBlock, currentBlock);
  } else {
    if (endNode) {
      endNode->addIncoming(Constant::getNullValue(endNode->getType()),
                           currentBlock);
    }
    BranchInst::Create(endBlock, currentBlock);
  }
}

void JavaJIT::throwException(llvm::Function* F, Value** args,
                             uint32 nbArgs) {
  Instruction* obj = CallInst::Create(F, args, args + nbArgs, "", currentBlock);
  DebugLoc DL = CreateLocation();
  obj->setDebugLoc(DL);

  if (currentExceptionBlock != endExceptionBlock) {
    Instruction* insn = currentExceptionBlock->begin();
    PHINode* node = dyn_cast<PHINode>(insn);
    if (node) node->addIncoming(obj, currentBlock);
    BranchInst::Create(currentExceptionBlock, currentBlock);
  } else {
    if (endNode) {
      endNode->addIncoming(Constant::getNullValue(endNode->getType()),
                           currentBlock);
    }
    BranchInst::Create(endBlock, currentBlock);
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

};

unsigned JavaJIT::readExceptionTable(Reader& reader, uint32 codeLen) {

  // This function uses currentBlock to simplify things. We save the current
  // value of currentBlock to restore it at the end of the function
  BasicBlock* temp = currentBlock;
  
  sint16 nbe = reader.readU2();
  sint16 sync = isSynchro(compilingMethod->access) ? 1 : 0;
  nbe += sync;
 
  mvm::ThreadAllocator allocator;
  // Loop over all handlers in the bytecode to initialize their values.
  Handler* handlers =
      (Handler*)allocator.Allocate(sizeof(Handler) * (nbe - sync));
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
    
    ex->tester = createBasicBlock("testException");
    
    // PHI Node for the exception object
    PHINode::Create(intrinsics->JavaObjectType, "", ex->tester);
    
    // Set the unwind destination of the instructions in the range of this
    // handler to the test block of the handler. If an instruction already has
    // a handler and thus is not the synchronize or regular end handler block,
    // leave it as-is.
    for (uint16 i = ex->startpc; i < ex->endpc; ++i) {
      if (opcodeInfos[i].exceptionBlock == endExceptionBlock) {
        opcodeInfos[i].exceptionBlock = ex->tester;
      }
    }

    // If the handler pc does not already have a block, create a new one.
    if (!(opcodeInfos[ex->handlerpc].newBlock)) {
      opcodeInfos[ex->handlerpc].newBlock = createBasicBlock("javaHandler");
    }
    
    // Set the Java handler for this exception.
    ex->javaHandler = opcodeInfos[ex->handlerpc].newBlock;
    opcodeInfos[ex->handlerpc].handler = true;
    
    if (ex->javaHandler->empty()) {
      PHINode::Create(intrinsics->JavaObjectType, "", ex->javaHandler);
    }

  }

  // Loop over all handlers to implement their tester.
  for (sint16 i = 0; i < nbe - sync; ++i) {
    Handler* cur = &handlers[i];
    BasicBlock* bbNext = 0;
    PHINode* javaNode = 0;
    currentExceptionBlock = opcodeInfos[cur->handlerpc].exceptionBlock;

    // Look out where we go if we're not the handler for the exception.
    if (i + 1 != nbe - sync) {
      Handler* next = &handlers[i + 1];
      if (!(cur->startpc >= next->startpc && cur->endpc <= next->endpc)) {
        // If there is no handler to go to (either one that has the same range
        // or one that contains the range), then we jump to the end handler.
        bbNext = endExceptionBlock;
      } else {
        // If there's a handler to goto, we jump to its tester block and record
        // the exception PHI node to give our exception to the tester.
        bbNext = next->tester;
        javaNode = dyn_cast<PHINode>(bbNext->begin());
        assert(javaNode);
      }
    } else {
      // If there's no handler after us, we jump to the end handler.
      bbNext = endExceptionBlock;
    }

    currentBlock = cur->tester;
    
    assert(cur->catchClass && 
           "Class not loaded when reading the exception table");

    Value* VTVar = TheCompiler->getVirtualTable(cur->catchClass->virtualVT);

    
#ifdef SERVICE
    // Verifies that the current isolate is not stopped. If it is, we don't
    // catch the exception but resume unwinding.
    JnjvmClassLoader* loader = compilingClass->classLoader;;
    if (loader != loader->bootstrapLoader) {
      Value* threadId = getCurrentThread(intrinsics->MutatorThreadType);
      Value* Isolate = GetElementPtrInst::Create(threadId,
                                                 intrinsics->constantFour, "",
                                                 currentBlock);
     
      Isolate = new LoadInst(Isolate, "", currentBlock);
      Isolate = new BitCastInst(Isolate, intrinsics->ptrPtrType, "", currentBlock);
      Value* Status = GetElementPtrInst::Create(Isolate, intrinsics->constantOne, "",
                                                currentBlock);
      Status = new LoadInst(Status, "", currentBlock);
      Status = new PtrToIntInst(Status, Type::Int32Ty, "", currentBlock);
  
      Value* stopping = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, Status,
                                     intrinsics->constantOne, "");

      BasicBlock* raiseBlock = createBasicBlock("raiseBlock");
      BasicBlock* continueBlock = createBasicBlock("continueBlock");
      BranchInst::Create(raiseBlock, continueBlock, stopping, currentBlock);
      currentBlock = raiseBlock;
      BranchInst::Create(endExceptionBlock, currentBlock); 

      currentBlock = continueBlock;
    }
#endif
    
    // Get the Java exception.
    Value* obj = currentBlock->begin();
    
    Value* objVT = CallInst::Create(intrinsics->GetVTFunction, obj, "",
                                    currentBlock);

    uint32 depth = cur->catchClass->virtualVT->depth;
    Value* depthCl = ConstantInt::get(Type::getInt32Ty(*llvmContext), depth);
    Value* cmp = 0;

    if (depth >= JavaVirtualTable::getDisplayLength()) {
      Value* classArgs[2] = { objVT, VTVar };
          
      cmp = CallInst::Create(intrinsics->IsSecondaryClassFunction,
                             classArgs, classArgs + 2, "",
                             currentBlock);

    } else {
     
      Value* inDisplay = CallInst::Create(intrinsics->GetDisplayFunction,
                                          objVT, "", currentBlock);
            
      Value* displayArgs[2] = { inDisplay, depthCl };
      Value* VTInDisplay = CallInst::Create(intrinsics->GetVTInDisplayFunction,
                                            displayArgs, displayArgs + 2, "",
                                            currentBlock);
             
      cmp = new ICmpInst(*currentBlock, ICmpInst::ICMP_EQ, VTInDisplay, VTVar,
                         "");
    }
   
    // Add the Java exception in the phi node of the handler.
    Instruction* insn = cur->javaHandler->begin();
    PHINode* node = dyn_cast<PHINode>(insn);
    assert(node && "malformed exceptions");
    node->addIncoming(obj, currentBlock);
   
    // Add the Java exception in the phi node of the next block.
    if (javaNode)
      javaNode->addIncoming(obj, currentBlock);
 
    // If we are catching this exception, then jump to the Java Handler,
    // otherwise jump to our next handler.
    BranchInst::Create(cur->javaHandler, bbNext, cmp, currentBlock);

    currentBlock = cur->javaHandler;

    // First thing in the handler: clear the exception.
    Value* geps[2] = { intrinsics->constantZero,
                       intrinsics->OffsetJavaExceptionInJavaThreadConstant };
    Value* threadId = getCurrentThread(intrinsics->JavaThreadType);
    Value* javaExceptionPtr = GetElementPtrInst::Create(threadId, geps,
                                                        geps + 2, "",
                                                        currentBlock);
    
    // Clear exceptions.
    new StoreInst(intrinsics->JavaObjectNullConstant, javaExceptionPtr,
                  currentBlock);

#if defined(SERVICE)
  
    // Change the isolate we are currently running, now that we have catched
    // the exception: the exception may have been thrown by another isolate.
    Value* threadId = 0;
    Value* OldIsolateID = 0;
    Value* IsolateIDPtr = 0;
    Value* OldIsolate = 0;
    Value* NewIsolate = 0;
    Value* IsolatePtr = 0;
    currentBlock = cur->javaHandler;
    if (loader != loader->bootstrapLoader) {
      threadId = getCurrentThread(intrinsics->MutatorThreadType);
     
      IsolateIDPtr = GetElementPtrInst::Create(threadId, intrinsics->constantThree,
                                               "", cur->javaHandler);
      const Type* realType = PointerType::getUnqual(intrinsics->pointerSizeType);
      IsolateIDPtr = new BitCastInst(IsolateIDPtr, realType, "",
                                     cur->javaHandler);
      OldIsolateID = new LoadInst(IsolateIDPtr, "", cur->javaHandler);

      Value* MyID = ConstantInt::get(intrinsics->pointerSizeType,
                                     loader->getIsolate()->IsolateID);

      new StoreInst(MyID, IsolateIDPtr, cur->javaHandler);
      IsolatePtr = GetElementPtrInst::Create(threadId, intrinsics->constantFour, "",
                                             cur->javaHandler);
     
      OldIsolate = new LoadInst(IsolatePtr, "", cur->javaHandler);
      NewIsolate = intrinsics->getIsolate(loader->getIsolate(), currentBlock);
      new StoreInst(NewIsolate, IsolatePtr, cur->javaHandler);

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
    if (endNode) {
      endNode->addIncoming(Constant::getNullValue(endNode->getType()),
                           endExceptionBlock);
    }
    BranchInst::Create(endBlock, endExceptionBlock);
  }
 

  PI = pred_begin(unifiedUnreachable);
  PE = pred_end(unifiedUnreachable);
  if (PI == PE) {
    unifiedUnreachable->eraseFromParent();
  } else {
    new UnreachableInst(*llvmContext, unifiedUnreachable);
  }
  
  for (Function::iterator BI = llvmFunction->begin(), BE = llvmFunction->end();
       BI != BE; BI++) {
    PI = pred_begin(BI);
    PE = pred_end(BI);
    if (PI == PE) {
      Instruction* insn = BI->begin();
      PHINode* node = dyn_cast<PHINode>(insn);
      if (node) {
        node->replaceAllUsesWith(Constant::getNullValue(node->getType()));
        node->eraseFromParent();
      }
    }
  }

}
