//===-------------------------- Bindings.java -----------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

package org.j3.bindings;

import org.j3.config.Selected;
import org.mmtk.plan.MutatorContext;
import org.mmtk.plan.Plan;
import org.mmtk.plan.TraceLocal;
import org.mmtk.plan.TransitiveClosure;
import org.mmtk.utility.heap.HeapGrowthManager;
import org.vmmagic.pragma.Inline;
import org.vmmagic.unboxed.Address;
import org.vmmagic.unboxed.Extent;
import org.vmmagic.unboxed.ObjectReference;

public final class Bindings {
  @Inline
  private static Address gcmalloc(int size, ObjectReference virtualTable) {
    Selected.Mutator mutator = Selected.Mutator.get();
    int allocator = mutator.checkAllocator(size, 0, 0);
    Address res = mutator.alloc(size, 0, 0, allocator, 0);
    res.store(virtualTable);
    mutator.postAlloc(res.toObjectReference(), virtualTable, size, allocator);
    return res;
  }

  @Inline
  private static boolean isLive(TraceLocal closure, ObjectReference obj) {
    return closure.isLive(obj);
  }

  @Inline
  private static void reportDelayedRootEdge(TraceLocal closure, Address obj) {
    closure.reportDelayedRootEdge(obj);
  }

  @Inline
  private static void processRootEdge(TraceLocal closure, Address obj, boolean traced) {
    closure.processRootEdge(obj, traced);
  }

  @Inline
  private static ObjectReference retainForFinalize(TraceLocal closure, ObjectReference obj) {
    return closure.retainForFinalize(obj);
  }

  @Inline
  private static ObjectReference retainReferent(TraceLocal closure, ObjectReference obj) {
    return closure.retainReferent(obj);
  }

  @Inline
  private static ObjectReference getForwardedReference(TraceLocal closure, ObjectReference obj) {
    return closure.getForwardedReference(obj);
  }

  @Inline
  private static ObjectReference getForwardedReferent(TraceLocal closure, ObjectReference obj) {
    return closure.getForwardedReferent(obj);
  }

  @Inline
  private static ObjectReference getForwardedFinalizable(TraceLocal closure, ObjectReference obj) {
    return closure.getForwardedFinalizable(obj);
  }

  @Inline
  private static void processEdge(TransitiveClosure closure, ObjectReference source, Address slot) {
    closure.processEdge(source, slot);
  }

  @Inline
  private static MutatorContext allocateMutator(int id) {
    Selected.Mutator mutator = new Selected.Mutator();
    mutator.initMutator(id);
    return mutator;
  }

  @Inline
  private static void freeMutator(MutatorContext mutator) {
    mutator.deinitMutator();
  }

  @Inline
  private static void boot(Extent minSize, Extent maxSize) {
    HeapGrowthManager.boot(minSize, maxSize);
    Plan plan = Selected.Plan.get();
    plan.boot();
    plan.postBoot();
    plan.fullyBooted();
  }
}
