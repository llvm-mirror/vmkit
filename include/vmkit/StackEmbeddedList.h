
#if JAVA_INTERFACE_CALL_STACK

#ifndef STACK_EMBEDDED_LIST_NODE_H_
#define STACK_EMBEDDED_LIST_NODE_H_

#include "vmkit/System.h"

namespace vmkit {

class Thread;

}

namespace j3 {

class JavaMethod;

enum StackEmbeddedListID {
	StackEmbeddedListIntendedCaller = 0,

	StackEmbeddedListNodeCountPerThread
};

struct StackEmbeddedListNode
{
	StackEmbeddedListNode* callerNode;
	word_t data[1];

	void initialize() {
		callerNode = NULL;
		memset(data, 0, sizeof(*data) * StackEmbeddedListNodeCountPerThread);
	}

	void dump() const __attribute__((noinline));
};

}

extern "C" void pushIntendedInvokedMethodNode(
	vmkit::Thread* thread, j3::StackEmbeddedListNode* node,
	j3::JavaMethod* intendedMethod);

extern "C" void popIntendedInvokedMethodNode(
	vmkit::Thread* thread, j3::StackEmbeddedListNode* node);

#endif

#endif
