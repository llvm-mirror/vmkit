//===---ReferenceQueue.h - Implementation of soft/weak/phantom references--===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_REFERENCE_QUEUE_H
#define J3_REFERENCE_QUEUE_H

#include "vmkit/ReferenceThread.h"
#include "vmkit/FinalizerThread.h"
#include "JavaThread.h"

namespace j3 {

class Jnjvm;

class JavaFinalizerThread : public vmkit::FinalizerThread<JavaThread>{
	public:
		JavaFinalizerThread(Jnjvm* vm) : FinalizerThread<JavaThread>(vm) {}
};

class JavaReferenceThread : public vmkit::ReferenceThread<JavaThread> {
public:
	JavaReferenceThread(Jnjvm* vm) : ReferenceThread<JavaThread>(vm) {}
};

} // namespace j3

#endif  //J3_REFERENCE_QUEUE_H
