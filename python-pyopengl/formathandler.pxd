"""Cython import description for formathandler types"""

cdef class FormatHandler:
	cdef public int ERROR_ON_COPY
	cdef object c_from_param( self, object instance, object typeCode)
	cdef object c_dataPointer( self, object instance )
	cdef c_zeros( self, object dims, object typeCode )
	cdef c_arraySize( self, object instance, object typeCode )
	cdef c_arrayByteCount( self, object instance )
	cdef c_arrayToGLType( self, object value )
	cdef c_asArray( self, object instance, object typeCode)
	cdef c_unitSize( self, object instance, typeCode )
	cdef c_dimensions( self, object instance)
