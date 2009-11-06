//===------------------------- Selected.java ------------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

package org.j3.config;

import org.mmtk.utility.Log;

import org.vmmagic.pragma.*;

public class Selected {
  public static final String name = "org.mmtk.plan.marksweep.MS";
  @Uninterruptible
  public static final class Plan extends org.mmtk.plan.marksweep.MS
  {
    private static final Plan plan = new Plan();

    @Inline
    public static Plan get() { return plan; }
  }

  @Uninterruptible
  public static final class Constraints extends org.mmtk.plan.marksweep.MSConstraints
  {
    private static final Constraints constraints = new Constraints();

    @Inline
    public static Constraints get() { return constraints; }
  }

  @Uninterruptible
  public static class Collector extends org.mmtk.plan.marksweep.MSCollector
  {
    private static final Collector bootstrapCollector = new Collector();
    
    public static void staticCollect() {
      bootstrapCollector.collect();
    }

    public Collector() {}
    @Inline
    public static Collector get() {
      return bootstrapCollector;
    }
  }

  @Uninterruptible
  public static class Mutator extends org.mmtk.plan.marksweep.MSMutator
  {
    public Mutator() {}

    @Inline
    public static native Mutator get();
  }
}
