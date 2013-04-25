//===--- FinalizerThread.h - Implementation of finalizable references--===//
//
//                            The VMKit project
//
// This file is distributed under the University of Pierre et Marie Curie
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef VMKIT_FINALIZERTHREAD_H_
#define VMKIT_FINALIZERTHREAD_H_

namespace vmkit {

	template <class T_THREAD> class FinalizerThread : public T_THREAD {
		public:
		/// FinalizationQueueLock - A lock to protect access to the queue.
		///
		vmkit::SpinLock FinalizationQueueLock;

		/// finalizationQueue - A list of allocated objets that contain a finalize
		/// method.
		///
		gc** FinalizationQueue;

		/// CurrentIndex - Current index in the queue of finalizable objects.
		///
		uint32 CurrentIndex;

		/// QueueLength - Current length of the queue of finalizable objects.
		///
		uint32 QueueLength;

		/// growFinalizationQueue - Grow the queue of finalizable objects.
		///
		void growFinalizationQueue() {
			if (CurrentIndex >= QueueLength) {
				uint32 newLength = QueueLength * GROW_FACTOR;
				gc** newQueue = new gc*[newLength];
				if (!newQueue) {
					fprintf(stderr, "I don't know how to handle finalizer overflows yet!\n");
					abort();
				}
				for (uint32 i = 0; i < QueueLength; ++i) newQueue[i] = FinalizationQueue[i];
				delete[] FinalizationQueue;
				FinalizationQueue = newQueue;
				QueueLength = newLength;
			}
		}


		/// ToBeFinalized - List of objects that are scheduled to be finalized.
		///
		gc** ToBeFinalized;

		/// ToBeFinalizedLength - Current length of the queue of objects scheduled
		/// for finalization.
		///
		uint32 ToBeFinalizedLength;

		/// CurrentFinalizedIndex - The current index in the ToBeFinalized queue
		/// that will be sceduled for finalization.
		///
		uint32 CurrentFinalizedIndex;

		/// growToBeFinalizedQueue - Grow the queue of the to-be finalized objects.
		///
		void growToBeFinalizedQueue() {
			if (CurrentFinalizedIndex >= ToBeFinalizedLength) {
				uint32 newLength = ToBeFinalizedLength * GROW_FACTOR;
				gc** newQueue = new gc*[newLength];
				if (!newQueue) {
					fprintf(stderr, "I don't know how to handle finalizer overflows yet!\n");
					abort();
				}
				for (uint32 i = 0; i < ToBeFinalizedLength; ++i) newQueue[i] = ToBeFinalized[i];
				delete[] ToBeFinalized;
				ToBeFinalized = newQueue;
				ToBeFinalizedLength = newLength;
			}
		}


		/// finalizationCond - Condition variable to wake up finalization threads.
		///
		vmkit::Cond FinalizationCond;

		/// finalizationLock - Lock for the condition variable.
		///
		vmkit::LockNormal FinalizationLock;

		static void finalizerStart(FinalizerThread* th) {
			gc* res = NULL;
			llvm_gcroot(res, 0);

			while (true) {
				th->FinalizationLock.lock();
				while (th->CurrentFinalizedIndex == 0) {
					th->FinalizationCond.wait(&th->FinalizationLock);
				}
				th->FinalizationLock.unlock();

				while (true) {
					th->FinalizationQueueLock.acquire();
					if (th->CurrentFinalizedIndex != 0) {
						res = th->ToBeFinalized[th->CurrentFinalizedIndex - 1];
						--th->CurrentFinalizedIndex;
					}
					th->FinalizationQueueLock.release();
					if (!res) break;

					th->MyVM->finalizeObject(res);

					res = NULL;
				}
			}
		}

		/// addFinalizationCandidate - Add an object to the queue of objects with
		/// a finalization method.
		///
		void addFinalizationCandidate(gc* obj) {
			llvm_gcroot(obj, 0);
			FinalizationQueueLock.acquire();

			if (CurrentIndex >= QueueLength) {
				growFinalizationQueue();
			}

			FinalizationQueue[CurrentIndex++] = obj;
			FinalizationQueueLock.release();
		}


		/// scanFinalizationQueue - Scan objets with a finalized method and schedule
		/// them for finalization if they are not live.
		///
		void scanFinalizationQueue(word_t closure) {
			uint32 NewIndex = 0;
			for (uint32 i = 0; i < CurrentIndex; ++i) {
				gc* obj = FinalizationQueue[i];

				if (!vmkit::Collector::isLive(obj, closure)) {
					obj = vmkit::Collector::retainForFinalize(FinalizationQueue[i], closure);

					if (CurrentFinalizedIndex >= ToBeFinalizedLength)
						growToBeFinalizedQueue();

					/* Add to object table */
					ToBeFinalized[CurrentFinalizedIndex++] = obj;
				} else {
					FinalizationQueue[NewIndex++] =
							vmkit::Collector::getForwardedFinalizable(obj, closure);
				}
			}
			CurrentIndex = NewIndex;
		}


		FinalizerThread(vmkit::VirtualMachine* vm) : T_THREAD(vm) {
			FinalizationQueue = new gc*[INITIAL_QUEUE_SIZE];
			QueueLength = INITIAL_QUEUE_SIZE;
			CurrentIndex = 0;

			ToBeFinalized = new gc*[INITIAL_QUEUE_SIZE];
			ToBeFinalizedLength = INITIAL_QUEUE_SIZE;
			CurrentFinalizedIndex = 0;
		}

		~FinalizerThread() {
			delete[] FinalizationQueue;
			delete[] ToBeFinalized;
		}
	};
}

#endif /* VMKIT_FINALIZERTHREAD_H_ */
