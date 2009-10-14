//===-------------- Mono.cpp - The Mono interface -------------------------===//
//
//                              N3
//
// This file is licensed under the University of Illinois Open Source License
// and the Mono License. See LICENSE.TXT for details.
// The file also has the following additional copyright:
//
// (C) 2008 Ximian, Inc.  http://www.ximian.com
//
//===----------------------------------------------------------------------===//

#include "mvm/JIT.h"

#include "Assembly.h"
#include "MonoString.h"
#include "MSCorlib.h"
#include "N3.h"
#include "Reader.h"
#include "VMArray.h"
#include "VMClass.h"
#include "VMObject.h"
#include "VMThread.h"

#include <glib.h>

#include "number-formatter.h"

#include "MonoPath.inc"

using namespace n3;

extern "C" int System_Environment_get_Platform (void) {
#if defined (PLATFORM_WIN32)
	/* Win32NT */
	return 2;
#else
	/* Unix */
	return 128;
#endif
}

static const char *encodings [] = {
	(char *) 1,
		"ascii", "us_ascii", "us", "ansi_x3.4_1968",
		"ansi_x3.4_1986", "cp367", "csascii", "ibm367",
		"iso_ir_6", "iso646_us", "iso_646.irv:1991",
	(char *) 2,
		"utf_7", "csunicode11utf7", "unicode_1_1_utf_7",
		"unicode_2_0_utf_7", "x_unicode_1_1_utf_7",
		"x_unicode_2_0_utf_7",
	(char *) 3,
		"utf_8", "unicode_1_1_utf_8", "unicode_2_0_utf_8",
		"x_unicode_1_1_utf_8", "x_unicode_2_0_utf_8",
	(char *) 4,
		"utf_16", "UTF_16LE", "ucs_2", "unicode",
		"iso_10646_ucs2",
	(char *) 5,
		"unicodefffe", "utf_16be",
	(char *) 6,
		"iso_8859_1",
	(char *) 0
};

/*
 * Returns the internal codepage, if the value of "int_code_page" is
 * 1 at entry, and we can not compute a suitable code page number,
 * returns the code page as a string
 */
extern "C" MonoString*
System_Text_Encoding_InternalCodePage (gint32 *int_code_page) 
{
	const char *cset;
	const char *p;
	char *c;
	char *codepage = NULL;
	int code;
	int want_name = *int_code_page;
	int i;
	
	*int_code_page = -1;

	g_get_charset (&cset);
	c = codepage = strdup (cset);
	for (c = codepage; *c; c++){
		if (isascii (*c) && isalpha (*c))
			*c = tolower (*c);
		if (*c == '-')
			*c = '_';
	}
	/* g_print ("charset: %s\n", cset); */
	
	/* handle some common aliases */
	p = encodings [0];
	code = 0;
	for (i = 0; p != 0; ){
		if ((gssize) p < 7){
			code = (gssize) p;
			p = encodings [++i];
			continue;
		}
		if (strcmp (p, codepage) == 0){
			*int_code_page = code;
			break;
		}
		p = encodings [++i];
	}
	
	if (strstr (codepage, "utf_8") != NULL)
		*int_code_page |= 0x10000000;
	free (codepage);
	
	if (want_name && *int_code_page == -1) {
		N3 *vm = (N3*)VMThread::get()->getVM();
		return (MonoString*)(vm->arrayToString(vm->asciizToArray(cset)));
	} else
		return NULL;
}

extern "C" void System_Threading_Monitor_Monitor_exit(VMObject* obj) {
  // TODO: There's a bug in the bootstrap, see why
  if (LockObj::owner(obj->lockObj)) VMObject::unlock(obj);
}

extern "C" bool System_Threading_Monitor_Monitor_try_enter(VMObject* obj, int ms) {
	VMObject::aquire(obj);
  return true;
}


extern "C" void* System_IO_MonoIO_get_ConsoleError() {
  return (void*)stderr;
}

extern "C" void* System_IO_MonoIO_get_ConsoleOutput() {
  return (void*)stdout;
}

extern "C" void* System_IO_MonoIO_get_ConsoleInput() {
  return (void*)stdin;
}

enum MonoFileType {
    Unknown=0x0000,
    Disk=0x0001,
    Char=0x0002,
    Pipe=0x0003,
    Remote=0x8000
};  

extern "C" MonoFileType System_IO_MonoIO_GetFileType(void* handle, int* error) {
  if (handle == (void*)stdin || handle == (void*)stdout || handle == (void*)stderr)
    return Char;
  else
    return Unknown;
}

extern "C" MonoString *
System_Environment_get_NewLine (void)
{
	N3 *vm = (N3*)VMThread::get()->getVM();
#if defined (PLATFORM_WIN32)
	return (MonoString*)(vm->arrayToString(vm->asciizToArray("\r\n")));
#else
	return (MonoString*)(vm->arrayToString(vm->asciizToArray("\n")));
#endif
}

extern "C" void
System_String_InternalCopyTo(MonoString* str, sint32 sindex, VMArray* dest, sint32 destIndex, sint32 count) {
  const ArrayChar* contents = str->value;
  memcpy(&dest->elements[destIndex], &contents->elements[sindex], count * sizeof(uint16));
}

extern "C" uint16 System_String_get_Chars(MonoString* str, sint32 offset) {
  return str->value->elements[offset];
}

static sint32 byteLength(VMArray* array) {
  VMClassArray* cl = (VMClassArray*)array->classOf;
  VMCommonClass* base = cl->baseClass;
  uint32 size = base->naturalType->getPrimitiveSizeInBits() / 8;  
  return array->size * size;
}

extern "C" bool System_Buffer_BlockCopyInternal (VMArray* src, int src_offset, VMArray* dest, int dest_offset, int count) {
  uint8 *src_buf, *dest_buf;

	/* watch out for integer overflow */
	if ((src_offset > byteLength(src) - count) || (dest_offset > byteLength(dest) - count))
		return false;

	src_buf = (uint8 *)src->elements + src_offset;
	dest_buf = (uint8 *)dest->elements + dest_offset;

	if (src != dest)
		memcpy (dest_buf, src_buf, count);
	else
		memmove (dest_buf, src_buf, count); /* Source and dest are the same array */

	return true;

}

extern "C" sint32 System_Buffer_ByteLengthInternal(VMArray* array) {
  return byteLength(array);
}

extern "C" sint32 
System_IO_MonoIO_Write (void* handle, ArrayUInt8 *src,
				  sint32 src_offset, sint32 count,
				  sint32 *error)
{
  char* buffer = (char*)alloca( 1024);//(count + 8) * sizeof(uint16));
	uint32 n = 0;

	*error = 0;
	
	if (src_offset + count > src->size)
		return 0;
   
  memcpy(buffer, (char*)&(src->elements[src_offset]), count);
  buffer[count] = 0;
	n = fprintf((FILE*)handle, buffer);

	return (sint32)n;
}

/* These parameters are "readonly" in corlib/System/NumberFormatter.cs */
extern "C" void
System_NumberFormatter_GetFormatterTables (guint64 const **mantissas,
					    gint32 const **exponents,
					    gunichar2 const **digitLowerTable,
					    gunichar2 const **digitUpperTable,
					    gint64 const **tenPowersList,
					    gint32 const **decHexDigits)
{
	*mantissas = Formatter_MantissaBitsTable;
	*exponents = Formatter_TensExponentTable;
	*digitLowerTable = Formatter_DigitLowerTable;
	*digitUpperTable = Formatter_DigitUpperTable;
	*tenPowersList = Formatter_TenPowersList;
	*decHexDigits = Formatter_DecHexDigits;
}

extern "C" VMObject* System_Threading_Thread_CurrentThread_internal() {
	declare_gcroot(VMObject*, appThread) = VMThread::get()->ooo_appThread;
  return appThread;
}

extern "C" VMObject*
System_Threading_Thread_GetCachedCurrentCulture (VMObject *obj)
{
	return 0;
}

extern "C" VMObject*
System_Threading_Thread_GetSerializedCurrentCulture (VMObject *obj)
{
	return 0;
}

extern "C" VMObject* System_Object_MemberwiseClone(VMObject* obj) {
  uint64 size = obj->objectSize();
  VMObject* res = ((VMClass*)obj->classOf)->doNew();
  memcpy(res, obj, size);
  res->lockObj = 0;
  return res;
}

extern "C" bool
System_Globalization_CultureInfo_construct_internal_locale_from_current_locale (VMObject *ci)
{
	return false;
}

extern "C" void
System_Threading_Thread_SetCachedCurrentCulture (VMObject* thread, VMObject *culture)
{
}

extern "C" void
System_String__ctor(MonoString* str, ArrayChar* array, sint32 startIndex, sint32 count) {
  N3* vm = VMThread::get()->getVM();
  const ArrayChar* value = vm->bufToArray(&(array->elements[startIndex]), count);
  str->length = count;
  str->startChar = array->elements[startIndex];
  str->value = value;
}

extern "C" MonoString * 
System_String_InternalJoin (MonoString *separator, VMArray * value, sint32 sindex, sint32 count)
{
	MonoString *current;
	sint32 length;
	sint32 pos;
	sint32 insertlen;
	sint32 destpos;
	sint32 srclen;
  const uint16 *insert;
	const uint16 *src;
	uint16 *dest;

	insert = separator->value->elements;
	insertlen = separator->length;

	length = 0;
	for (pos = sindex; pos != sindex + count; pos++) {
		current = (MonoString*)value->elements[pos];
		if (current != NULL)
			length += current->length;

		if (pos < sindex + count - 1)
			length += insertlen;
	}

	dest = (uint16*)alloca(length * sizeof(uint16));
	destpos = 0;

	for (pos = sindex; pos != sindex + count; pos++) {
		current = (MonoString*)value->elements[pos];
		if (current != NULL) {
			src = current->value->elements;
			srclen = current->length;

			memcpy (dest + destpos, src, srclen * sizeof(uint16));
			destpos += srclen;
		}

		if (pos < sindex + count - 1) {
			memcpy(dest + destpos, insert, insertlen * sizeof(uint16));
			destpos += insertlen;
		}
	}
  
  N3* vm = (N3*)VMThread::get()->getVM();
  const ArrayChar* array = vm->bufToArray(dest, length);
	return (MonoString*)vm->arrayToString(array);
}

extern "C" MonoString *
System_String_InternalAllocateStr (sint32 length)
{
  MonoString* str = (MonoString*)(MSCorlib::pString->doNew());
  str->length = length;
  return str;
}

