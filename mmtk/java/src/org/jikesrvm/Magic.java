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
package org.jikesrvm;

import org.vmmagic.Intrinsic;
import org.vmmagic.pragma.Entrypoint;
import org.vmmagic.pragma.Uninterruptible;
import org.vmmagic.unboxed.Address;
import org.vmmagic.unboxed.Offset;
import org.vmmagic.unboxed.Word;
import org.vmmagic.unboxed.WordArray;

import org.j3.runtime.VM;

/**
 * Magic methods for accessing raw machine memory, registers, and
 * operating system calls.
 *
 * <p> These are "inline assembler functions" that cannot be implemented in
 * Java code. Their names are recognized by RVM's compilers
 * and cause inline machine code to be generated instead of
 * actual method calls.
 */
@SuppressWarnings({"UnusedDeclaration"})
@Intrinsic
public final class Magic {

  /**
   * Read contents of hardware time base registers.
   * Note:     we think that 1 "tick" == 4 "machine cycles", but this seems to be
   *           undocumented and may vary across processor implementations.
   * @return number of ticks (epoch undefined)
   */
  public static long getTimeBase() {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return -1;
  }

  //---------------------------------------//
  //           Memory Access.              //
  //---------------------------------------//

  /**
   * Get unsigned byte at arbitrary (byte) offset from object. The
   * most significant 24bits of the result will be 0.
   */
  public static byte getUnsignedByteAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return -1;
  }

  /**
   * Get byte at arbitrary (byte) offset from object. The most
   * significant 24bits of the result will be the same as the most
   * significant bit in the byte.
   */
  public static byte getByteAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return -1;
  }

  /**
   * Get short at arbitrary (byte) offset from object. The most
   * significant 16bits will be the same as the most significant bit
   * in the short.
   */
  public static short getShortAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return (short) -1;
  }

  /**
   * Get char at arbitrary (byte) offset from object. The most
   * significant 16bits will be 0.
   */
  public static char getCharAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return (char) -1;
  }

  /**
   * Get int at arbitrary (byte) offset from object.
   * Use getIntAtOffset(obj, ofs) instead of getMemoryInt(objectAsAddress(obj)+ofs)
   */
  public static int getIntAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return -1;
  }

  /**
   * Get Word at arbitrary (byte) offset from object.
   * Use getWordAtOffset(obj, ofs) instead of getMemoryWord(objectAsAddress(obj)+ofs)
   */
  public static Word getWordAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return Word.max();
  }

  /**
   * Get Object at arbitrary (byte) offset from object.
   * Use getObjectAtOffset(obj, ofs) instead of
   * addressAsObject(getMemoryAddress(objectAsAddress(obj)+ofs))
   */
  public static Object getObjectAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return null;
  }

  /**
   * Get Object at arbitrary (byte) offset from object.
   * Use getObjectAtOffset(obj, ofs) instead of
   * addressAsObject(getMemoryAddress(objectAsAddress(obj)+ofs))
   */
  public static Object getObjectAtOffset(Object object, Offset offset, int locationMetadata) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return null;
  }

  /**
   * Get Word at arbitrary (byte) offset from object.
   */
  public static Word getWordAtOffset(Object object, Offset offset, int locationMetadata) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return null;
  }

  /**
   * Get long at arbitrary (byte) offset from object.
   * Use getlongAtOffset(obj, ofs) instead of two getIntAtOffset
   */
  public static long getLongAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return -1;
  }

  /**
   * Get double at arbitrary (byte) offset from object.
   * Use getDoubleAtOffset(obj, ofs) instead of two getIntAtOffset
   */
  public static double getDoubleAtOffset(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return -1;
  }

  /**
   * Set byte at arbitrary (byte) offset from object.
   */
  public static void setByteAtOffset(Object object, Offset offset, byte newvalue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  /**
   * Set char at arbitrary (byte) offset from object.
   */
  public static void setCharAtOffset(Object object, Offset offset, char newvalue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  /**
   * Set int at arbitrary (byte) offset from object.
   * Use setIntAtOffset(obj, ofs, new) instead of setMemoryWord(objectAsAddress(obj)+ofs, new)
   */
  public static void setIntAtOffset(Object object, Offset offset, int newvalue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  /**
   * Set word at arbitrary (byte) offset from object.
   * Use setWordAtOffset(obj, ofs, new) instead of setMemoryWord(objectAsAddress(obj)+ofs, new)
   */
  public static void setWordAtOffset(Object object, Offset offset, Word newvalue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  /**
   * Set word at arbitrary (byte) offset from object.
   * Use setWordAtOffset(obj, ofs, new) instead of setMemoryWord(objectAsAddress(obj)+ofs, new)
   */
  public static void setWordAtOffset(Object object, Offset offset, Word newvalue, int locationMetadata) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  /**
   * Set Object at arbitrary (byte) offset from object.
   * Use setObjectAtOffset(obj, ofs, new) instead of setMemoryWord(objectAsAddress(obj)+ofs, objectAsAddress(new))
   */
  public static void setObjectAtOffset(Object object, Offset offset, Object newvalue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  /**
   * Set Object at arbitrary (byte) offset from object.
   */
  public static void setObjectAtOffset(Object object, Offset offset, Object newvalue, int locationMetadata) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  /**
   * Set long at arbitrary (byte) offset from object.
   * Use setlongAtOffset(obj, ofs) instead of two setIntAtOffset
   */
  public static void setLongAtOffset(Object object, Offset offset, long newvalue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  /**
   * Set double at arbitrary (byte) offset from object.
   * Use setDoubleAtOffset(obj, ofs) instead of two setIntAtOffset
   */
  public static void setDoubleAtOffset(Object object, Offset offset, double newvalue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
  }

  //---------------------------------------//
  //    Atomic Memory Access Primitives.   //
  //---------------------------------------//

  /**
   * Get contents of (object + offset) and begin conditional critical section.
   */
  public static int prepareInt(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return -1;
  }

  /**
   * Get contents of (object + offset) and begin conditional critical section.
   */
  public static Object prepareObject(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return null;
  }

  /**
   * Get contents of (object + offset) and begin conditional critical section.
   */
  public static Address prepareAddress(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return Address.max();
  }

  /**
   * Get contents of (object + offset) and begin conditional critical section.
   */
  public static Word prepareWord(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return Word.max();
  }

  /**
   * Get contents of (object + offset) and begin conditional critical section.
   */
  @Uninterruptible
  public static long prepareLong(Object object, Offset offset) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return -1;
  }

  /**
   * Sets the memory at (object + offset) to newValue if its contents are oldValue.
   * Must be paired with a preceding prepare (which returned the oldValue)
   * Returns true if successful.
   * Ends conditional critical section.
   */
  public static boolean attemptInt(Object object, Offset offset, int oldValue, int newValue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return false;
  }

  /**
   * Sets the memory at (object + offset) to newValue if its contents are oldValue.
   * Must be paired with a preceding prepare (which returned the oldValue)
   * Returns true if successful.
   * Ends conditional critical section.
   */
  public static boolean attemptObject(Object object, Offset offset, Object oldValue, Object newValue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return false;
  }

  /**
   * Sets the memory at (object + offset) to newValue if its contents are oldValue.
   * Must be paired with a preceding prepare (which returned the oldValue)
   * Returns true if successful.
   * Ends conditional critical section.
   */
  public static boolean attemptAddress(Object object, Offset offset, Address oldValue, Address newValue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return false;
  }

  /**
   * Sets the memory at (object + offset) to newValue if its contents are oldValue.
   * Must be paired with a preceding prepare (which returned the oldValue)
   * Returns true if successful.
   * Ends conditional critical section.
   */
  public static boolean attemptWord(Object object, Offset offset, Word oldValue, Word newValue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return false;
  }

  /**
   * Sets the memory at (object + offset) to newValue if its contents are oldValue.
   * Must be paired with a preceding prepare (which returned the oldValue)
   * Returns true if successful.
   * Ends conditional critical section.
   */
  public static boolean attemptLong(Object object, Offset offset, long oldValue,
      long newValue) {
    if (VM.VerifyAssertions) VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    return false;
  }

  //---------------------------------------//
  //            Cache Management.          //
  //---------------------------------------//

  /**** NOTE: all per-address operations now live in vmmagic.Address *****/

  /**
   * Wait for preceeding cache flush/invalidate instructions to
   * complete on all processors.
   */
  public static void sync() {
    if (VM.runningVM && VM.VerifyAssertions) {
      VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    }
  }

  /**
   * Wait for all preceeding instructions to complete and discard any
   * prefetched instructions on this processor.
   */
  public static void isync() {
    if (VM.runningVM && VM.VerifyAssertions) {
      VM._assert(VM.NOT_REACHED);  // call site should have been hijacked by magic in compiler
    }
  }

}

