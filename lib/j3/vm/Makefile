#===- ./Makefile -------------------------------------------*- Makefile -*--===#
# 
#                     The vmkit project
#===------------------------------------------------------------------------===#

LEVEL := ../../..

MODULE=j3

ifdef rien
EXTRACT=					                           \
	'j3::J3Type::vt()' \
	'j3::J3Method::index()' \
	'j3::J3Type::initialise()' \
	'j3::J3ObjectType::javaClass()'    \
	'j3::J3Class::size()' \
	'j3::J3Class::staticInstance()' \
	'j3::J3Class::stringAt(unsigned short)'    \
	'j3::J3Object::allocate(j3::J3VirtualTable*, unsigned long)' \
	'j3::J3::jniEnv()'												 \
	'j3::J3::throwException(j3::J3Object*)'  \
	'j3::J3::classCastException()'  \
	'j3::J3::nullPointerException()'  \
	'j3::J3VirtualTable::slowIsAssignableTo(j3::J3VirtualTable*)' \
	'j3::J3VirtualTable::fastIsAssignableToPrimaryChecker(j3::J3VirtualTable*, unsigned int)' \
	'j3::J3VirtualTable::fastIsAssignableToNonPrimaryChecker(j3::J3VirtualTable*)' \
	'j3::J3Thread::push(j3::J3ObjectHandle*)' \
	'j3::J3Thread::push(j3::J3Object*)' \
  'j3::J3Thread::tell()' \
  'j3::J3Thread::restore(j3::J3ObjectHandle*)' \
  'j3::J3Thread::get()'

EXTRACT_TYPE= 				      \
	'struct.JNIEnv_'					\
	'class.j3::J3ArrayObject'
endif

include $(LEVEL)/Makefile.rules
