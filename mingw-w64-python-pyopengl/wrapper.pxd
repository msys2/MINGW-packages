"""Importable Cython declarations for wrapper module"""
cdef class cArgConverter:
	cdef object c_call( self, tuple pyArgs, int index, object baseOperation )
cdef class pyArgConverter:
	cdef object c_call( self, object incoming, object function, tuple arguments )
cdef class cArgumentConverter:
	cdef object c_call( self, object incoming )
cdef class returnConverter:
	cdef object c_call( self, object result, object baseOperation, tuple pyArgs, tuple cArgs )
