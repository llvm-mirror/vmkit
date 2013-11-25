#ifndef _J3_PRIMITIVE_H_
#define _J3_PRIMITIVE_H_

namespace j3 {

#define onJavaPrimitives(_)														\
	_(Boolean, bool,     Int1)													\
	_(Byte,    int8_t,   Int8)													\
	_(Short,   int16_t,  Int16)													\
	_(Char,    uint16_t, Int16)													\
	_(Integer, int32_t,  Int32)													\
	_(Long,    int64_t,  Int64)													\
	_(Float,   float,    Float)													\
	_(Double,  double,   Double)												\

#define onJavaObject(_)													\
	_(Object,  J3ObjectHandle*, Fatal)

#define onJavaVoid(_)														\
	_(Void, void,        Void)																	

#define onJavaTypes(_)													\
	onJavaPrimitives(_)														\
	onJavaVoid(_)

#define onJavaFields(_)													\
	onJavaPrimitives(_)														\
	onJavaObject(_)

}

#endif
