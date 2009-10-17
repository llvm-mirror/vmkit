#ifndef _N3_META_TYPE_H_
#define _N3_META_TYPE_H_

#define do_nothing(obj, val)

#define _APP(A, B) A(B)

#define ON_PRIMITIVES(_, _F)																						\
	_APP(_, _F(Boolean, bool,      IntVal.getBoolValue(), do_nothing,  writeBool, "Array<", " ", ">")) \
	_APP(_, _F(UInt8,   uint8,     IntVal.getZExtValue(), do_nothing,  writeS4,   "Array<", " ", ">")) \
	_APP(_, _F(SInt8,   sint8,     IntVal.getSExtValue(), do_nothing,  writeS4,   "Array<", " ", ">")) \
	_APP(_, _F(Char,    uint16,    IntVal.getZExtValue(), do_nothing,  writeChar, "",       "",  "")) \
	_APP(_, _F(UInt16,  uint16,    IntVal.getZExtValue(), do_nothing,  writeS4,   "Array<", " ", ">")) \
	_APP(_, _F(SInt16,  sint16,    IntVal.getSExtValue(), do_nothing,  writeS4,   "Array<", " ", ">")) \
	_APP(_, _F(UInt32,  uint32,    IntVal.getZExtValue(), do_nothing,  writeS4,   "Array<", " ", ">")) \
	_APP(_, _F(SInt32,  sint32,    IntVal.getSExtValue(), do_nothing,  writeS4,   "Array<", " ", ">")) \
	_APP(_, _F(UInt64,  uint64,    IntVal.getZExtValue(), do_nothing,  writeS8,   "Array<", " ", ">")) \
	_APP(_, _F(SInt64,  sint64,    IntVal.getSExtValue(), do_nothing,  writeS8,   "Array<", " ", ">")) \
	_APP(_, _F(UIntPtr, uint*,     PointerVal,            do_nothing,  writePtr,  "Array<", " ", ">")) \
	_APP(_, _F(IntPtr,  int*,      PointerVal,            do_nothing,  writePtr,  "Array<", " ", ">")) \
	_APP(_, _F(Float,   float,     FloatVal,              do_nothing,  writeFP,   "Array<", " ", ">")) \
	_APP(_, _F(Double,  double,    DoubleVal,             do_nothing,  writeFP,   "Array<", " ", ">"))

#define ON_VOID(_, _F)																									\
	_APP(_, _F(Void,    void,      abort(),               do_nothing,  abort(),   "",       "",  ""))

#define ON_OBJECT(_, _F)																								\
	_APP(_, _F(Object, VMObject*,  PointerVal,            llvm_gcroot, writeObj,  "Array<", " ", ">"))

#define ON_STRING(_, _F)																								\
	_APP(_, _F(String, uint16*,    PointerVal,            llvm_gcroot, writeObj,  "",       "",  ""))

#define ON_TYPES(_, _F)												\
  ON_PRIMITIVES(_, _F)												\
	ON_OBJECT(_, _F)

#define _F_NT(  name, type, gv_extractor, do_root, writer, pre, sep, post)    name, type
#define _F_NTR( name, type, gv_extractor, do_root, writer, pre, sep, post)    name, type, do_root
#define _F_NTE( name, type, gv_extractor, do_root, writer, pre, sep, post)    name, type, gv_extractor
#define _F_NTRW(name, type, gv_extractor, do_root, writer, pre, sep, post)    name, type, do_root, writer, pre, sep, post
#define _F_ALL( name, type, gv_extractor, do_root, writer, pre, sep, post)    name, type, gv_extractor, writer, do_root, pre, sep, post


#endif
