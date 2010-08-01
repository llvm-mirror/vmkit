/*
 *  This file is part of the Jikes RVM project (http://jikesrvm.org).
 *
 *  This file is licensed to You under the Eclipse Public License (EPL);
 *  You may not use this file except in compliance with the License. You
 *  may obtain a copy of the License at
 *
 *      http://www.opensource.org/licenses/eclipse-1.0.php
 *
 *  See the COPYRIGHT.txt file distributed with this work for information
 *  regarding copyright ownership.
 */
package org.j3.config;

import org.mmtk.utility.Log;

import org.vmmagic.pragma.*;

public class Selected {
  public static final String name = "org.mmtk.plan.copyms.CopyMS";
  @Uninterruptible
  public static final class Plan extends org.mmtk.plan.copyms.CopyMS
  {
    private static final Plan plan = new Plan();

    @Inline
    public static Plan get() { return plan; }
  }

  @Uninterruptible
  public static final class Constraints extends org.mmtk.plan.copyms.CopyMSConstraints
  {
    private static final Constraints constraints = new Constraints();

    @Inline
    public static Constraints get() { return constraints; }
  }

  @Uninterruptible
  public static class Collector extends org.mmtk.plan.copyms.CopyMSCollector
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
  public static class Mutator extends org.mmtk.plan.copyms.CopyMSMutator
  {
    // Unused mutator used by the AOT compiler to know what instances
    // will be alive during MMTk execution. This allows to inline
    // virtual calls of singleton objects.
    private static final Mutator unusedMutator = new Mutator();
    
    public Mutator() {}

    @Inline
    public static native Mutator get();
  }
}
