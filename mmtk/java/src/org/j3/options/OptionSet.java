//===------------------------- OptionSet.java -----------------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

package org.j3.options;


import org.mmtk.utility.Constants;
import org.vmmagic.pragma.Uninterruptible;
import org.vmmagic.unboxed.Extent;
import org.vmmagic.unboxed.Word;
import org.vmutil.options.AddressOption;
import org.vmutil.options.BooleanOption;
import org.vmutil.options.EnumOption;
import org.vmutil.options.FloatOption;
import org.vmutil.options.IntOption;
import org.vmutil.options.MicrosecondsOption;
import org.vmutil.options.Option;
import org.vmutil.options.PagesOption;
import org.vmutil.options.StringOption;

import org.jikesrvm.SizeConstants;
import org.j3.runtime.VM;

/**
 * Class to handle command-line arguments and options for GC.
 */
public final class OptionSet extends org.vmutil.options.OptionSet {

  private String prefix;

  public static final OptionSet gc = new OptionSet("-X:gc");

  private OptionSet(String prefix) {
    this.prefix = prefix;
  }

  /**
   * Format and log an option value.
   *
   * @param o The option.
   * @param forXml Is this part of xml output?
   */
  protected void logValue(Option o, boolean forXml) {
    switch (o.getType()) {
    case Option.BOOLEAN_OPTION:
      VM.sysWrite(((BooleanOption) o).getValue() ? "true" : "false");
      break;
    case Option.INT_OPTION:
      VM.sysWrite(((IntOption) o).getValue());
      break;
    case Option.ADDRESS_OPTION:
      VM.sysWrite(((AddressOption) o).getValue());
      break;
    case Option.FLOAT_OPTION:
      VM.sysWrite(((FloatOption) o).getValue());
      break;
    case Option.MICROSECONDS_OPTION:
      VM.sysWrite(((MicrosecondsOption) o).getMicroseconds());
      VM.sysWrite(" usec");
      break;
    case Option.PAGES_OPTION:
      VM.sysWrite(((PagesOption) o).getBytes());
      VM.sysWrite(" bytes");
      break;
    case Option.STRING_OPTION:
      VM.sysWrite(((StringOption) o).getValue());
      break;
    case Option.ENUM_OPTION:
      VM.sysWrite(((EnumOption) o).getValueString());
      break;
    }
  }

  /**
   * Log a string.
   */
  protected void logString(String s) {
    VM.sysWrite(s);
  }

  /**
   * Print a new line.
   */
  protected void logNewLine() {
    VM.sysWriteln();
  }

  /**
   * Determine the VM specific key for a given option name. Option names are
   * space delimited with capitalised words (e.g. "GC Verbosity Level").
   *
   * @param name The option name.
   * @return The VM specific key.
   */
  protected String computeKey(String name) {
    int space = name.indexOf(' ');
    if (space < 0) return name.toLowerCase();

    String word = name.substring(0, space);
    String key = word.toLowerCase();

    do {
      int old = space+1;
      space = name.indexOf(' ', old);
      if (space < 0) {
        key += name.substring(old);
        return key;
      }
      key += name.substring(old, space);
    } while (true);
  }

  /**
   * A non-fatal error occurred during the setting of an option. This method
   * calls into the VM and shall not cause the system to stop.
   *
   * @param o The responsible option.
   * @param message The message associated with the warning.
   */
  protected void warn(Option o, String message) {
    VM.sysWriteln("WARNING: Option '" + o.getKey() + "' : " + message);
  }

  /**
   * A fatal error occurred during the setting of an option. This method
   * calls into the VM and is required to cause the system to stop.
   *
   * @param o The responsible option.
   * @param message The error message associated with the failure.
   */
  protected void fail(Option o, String message) {
    VM.sysFail("ERROR: Option '" + o.getKey() + "' : " + message);
  }

  /**
   * Convert bytes into pages, rounding up if necessary.
   *
   * @param bytes The number of bytes.
   * @return The corresponding number of pages.
   */
  @Uninterruptible
  protected int bytesToPages(Extent bytes) {
    return bytes.plus(Constants.BYTES_IN_PAGE-1).toWord().rshl(Constants.LOG_BYTES_IN_PAGE).toInt();
  }

  /**
   * Convert from pages into bytes.
   * @param pages the number of pages.
   * @return The corresponding number of bytes.
   */
  @Uninterruptible
  protected Extent pagesToBytes(int pages) {
    return Word.fromIntZeroExtend(pages).lsh(Constants.LOG_BYTES_IN_PAGE).toExtent();
  }
}
