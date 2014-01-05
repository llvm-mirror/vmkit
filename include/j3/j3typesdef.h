#ifndef _J3_PRIMITIVE_H_
#define _J3_PRIMITIVE_H_

namespace j3 {

	/* name, ctype, llvmType, scale size */
#define onJavaPrimitives(_)																							\
	_(Boolean,   bool,     Int1,   0)																			\
	_(Byte,      int8_t,   Int8,   0)																			\
	_(Short,     int16_t,  Int16,  1)																			\
	_(Character, uint16_t, Int16,  1)																			\
	_(Integer,   int32_t,  Int32,  2)																			\
	_(Long,      int64_t,  Int64,  3)																			\
	_(Float,     float,    Float,  2)																			\
	_(Double,    double,   Double, 3)																			\

#define onJavaVoid(_)														\
	_(Void, void,        Void, 0)																	

#define onJavaObject(_)													\
	_(Object,  J3ObjectHandle*, Fatal, 3)

#define onJavaTypes(_)													\
	onJavaPrimitives(_)														\
	onJavaVoid(_)

#define onJavaFields(_)													\
	onJavaPrimitives(_)														\
	onJavaObject(_)

}

#endif
