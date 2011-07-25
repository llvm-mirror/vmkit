//===------------------- UTF8.h - Utilities for UTF8 ----------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _JNJVM_UTF8_H_
#define _JNJVM_UTF8_H_

#include "types.h"

#include "mvm/UTF8.h"
#include "mvm/PrintBuffer.h"

namespace j3 {
	using mvm::UTF8;
	using mvm::UTF8Map;

/// UTF8Buffer - Helper class to create char* buffers suitable for
/// printf.
///
class UTF8Buffer : public mvm::PrintBuffer {
public:
  /// UTF8Buffer - Create a buffer with the following UTF8.
  UTF8Buffer(const UTF8* val) : mvm::PrintBuffer(val) {}

  /// toCompileName - Change the utf8 following JNI conventions.
  ///
  UTF8Buffer* toCompileName(const char* suffix = "") {
		const char *buffer = cString();
    uint32 len = strlen(buffer);
    uint32 suffixLen = strlen(suffix);
    char* newBuffer = new char[(len << 1) + suffixLen + 1];
    uint32 j = 0;
    for (uint32 i = 0; i < len; ++i) {
      if (buffer[i] == '/') {
        newBuffer[j++] = '_';
      } else if (buffer[i] == '_') {
        newBuffer[j++] = '_';
        newBuffer[j++] = '1';
      } else if (buffer[i] == ';') {
        newBuffer[j++] = '_';
        newBuffer[j++] = '2';
      } else if (buffer[i] == '[') {
        newBuffer[j++] = '_';
        newBuffer[j++] = '3';
      } else if (buffer[i] == '$') {
        newBuffer[j++] = '_';
        newBuffer[j++] = '4';
      } else {
        newBuffer[j++] = buffer[i];
      }
    }
    for (uint32 i = 0; i < suffixLen; i++) {
      newBuffer[j++] = suffix[i];
    }
    newBuffer[j] = 0;
		replaceWith(newBuffer);
    return this;
  }
};

}

#endif
