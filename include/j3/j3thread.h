#ifndef _J3_THREAD_H_
#define _J3_THREAD_H_

#include "vmkit/thread.h"
#include "vmkit/allocator.h"
#include "j3/j3object.h"
#include "j3/j3jni.h"

namespace vmkit {
	class Safepoint;
}

namespace j3 {
	class J3;

	class J3Thread : public vmkit::Thread {
		friend class J3Monitor;
		friend class J3CodeGen;

		static const uint32_t gepInterfaceMethodIndex = 1;
		uint32_t              _interfaceMethodIndex;

		vmkit::BumpAllocator*      _allocator;
		JNIEnv                     _jniEnv;
		J3LocalReferences          _localReferences;
		J3ObjectHandle*            _pendingException;
		J3ObjectHandle*            _javaThread;
		J3Thread*                  _nextLocked;

		virtual void run();
		static void doRun();

	public:
		J3Thread(J3* vm);
		~J3Thread();

		void               assocJavaThread(J3ObjectHandle* javaThread);
		J3ObjectHandle*    javaThread() { return _javaThread; }
		static J3Thread*   nativeThread(J3ObjectHandle* handle);

		J3Method*          getJavaCaller(uint32_t level=0);

		uint32_t           interfaceMethodIndex() { return _interfaceMethodIndex; }

		void               ensureCapacity(uint32_t capacity);
		J3ObjectHandle*    pendingException();
		bool               hasPendingException() { return _pendingException; }
		void               setPendingException(J3ObjectHandle* handle) { _pendingException = handle; }

		J3ObjectHandle*    push(J3ObjectHandle* handle);
		J3ObjectHandle*    push(J3Object* obj);
		J3ObjectHandle*    tell();
		void               restore(J3ObjectHandle* ptr);

		J3* vm() { return (J3*)Thread::vm(); }

		JNIEnv* jniEnv() { return &_jniEnv; }

		static J3Thread* get();

		static void start(J3ObjectHandle* handle);
	};

	class J3ThreadBootstrap : public J3Thread {
	public:
		J3ThreadBootstrap(J3* j3);

		void run();
	};
}

#endif
