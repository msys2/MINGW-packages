// Copyright 2024 - 2026 The excelize Authors. All rights reserved. Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.
//
// Package excelize-py is a Python port of Go Excelize library, providing a set
// of functions that allow you to write and read from XLAM / XLSM / XLSX / XLTM
// / XLTX files. Supports reading and writing spreadsheet documents generated
// by Microsoft Excel™ 2007 and later. Supports complex components by high
// compatibility, and provided streaming API for generating or reading data from
// a worksheet with huge amounts of data. This library needs Python version 3.9
// or later.

package main

/*
#include "types_c.h"
*/
import "C"

import (
	"bytes"
	"errors"
	"reflect"
	"sync"
	"time"
	"unicode"
	"unsafe"

	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"

	_ "golang.org/x/image/tiff"

	"github.com/xuri/excelize/v2"
)

type Cells struct {
	Cell []string
}
type StringMatrixErrorResult struct {
	Row []Cells
}

const (
	Nil     C.int = 0
	Int     C.int = 1
	Int32   C.int = 2
	String  C.int = 3
	Float   C.int = 4
	Boolean C.int = 5
	Time    C.int = 6
)

var (
	files, rowsIterator, sw = sync.Map{}, sync.Map{}, sync.Map{}
	emptyString             string
	errFilePtr              = "can not find file pointer"
	errRowsIteratorPtr      = "can not find rows iterator pointer"
	errStreamWriterPtr      = "can not find stream writer pointer"
	errArgType              = errors.New("invalid argument data type")

	// goBaseTypes defines Go's basic data types.
	goBaseTypes = map[reflect.Kind]bool{
		reflect.Bool:    true,
		reflect.Int:     true,
		reflect.Int8:    true,
		reflect.Int16:   true,
		reflect.Int32:   true,
		reflect.Int64:   true,
		reflect.Uint:    true,
		reflect.Uint8:   true,
		reflect.Uint16:  true,
		reflect.Uint32:  true,
		reflect.Uint64:  true,
		reflect.Uintptr: true,
		reflect.Float32: true,
		reflect.Float64: true,
		reflect.Map:     true,
		reflect.String:  true,
	}
	// cToBaseGoTypeFuncs defined functions mapping for G to Go basic data types
	// convention.
	cToBaseGoTypeFuncs = map[reflect.Kind]func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error){
		reflect.Bool: func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(cVal.Bool()), nil
		},
		reflect.Uint: func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(uint(cVal.Interface().(C.uint))), nil
		},
		reflect.Uint8: func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(uint8(cVal.Interface().(C.uchar))), nil
		},
		reflect.Uint64: func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(uint64(cVal.Interface().(C.uint))), nil
		},
		reflect.Int: func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(int(cVal.Int())), nil
		},
		reflect.Int64: func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(cVal.Int()), nil
		},
		reflect.Float64: func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(float64(cVal.Float())), nil
		},
		reflect.String: func(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			if cVal.Elem().CanAddr() {
				return reflect.ValueOf(C.GoString(cVal.Interface().(*C.char))), nil
			}
			return reflect.ValueOf(""), nil
		},
	}
	// goBaseValueToCFuncs defined functions mapping for Go basic data types
	// value to C convention.
	goBaseValueToCFuncs = map[reflect.Kind]func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error){
		reflect.Bool: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C._Bool(goVal.Bool())), nil
		},
		reflect.Uint: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.uint(uint32(goVal.Uint()))), nil
		},
		reflect.Uint8: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.uint(uint32(goVal.Uint()))), nil
		},
		reflect.Uint32: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.uint(uint32(goVal.Uint()))), nil
		},
		reflect.Uint64: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.ulong(goVal.Uint())), nil
		},
		reflect.Int: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.int(int32(goVal.Int()))), nil
		},
		reflect.Int32: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.long(int32(goVal.Int()))), nil
		},
		reflect.Int64: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.longlong(int64(goVal.Int()))), nil
		},
		reflect.Float64: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.double(goVal.Float())), nil
		},
		reflect.String: func(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
			return reflect.ValueOf(C.CString(goVal.String())), nil
		},
	}
)

// cToGoBaseType convert JavaScript value to Go basic data type variable.
func cToGoBaseType(cVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
	fn, ok := cToBaseGoTypeFuncs[kind]
	if !ok {
		return reflect.ValueOf(nil), errArgType
	}
	return fn(cVal, kind)
}

// cToGoArray convert C language array to Go array base on the given Go	structure
// types.
func cToGoArray(cArray reflect.Value, cArrayLen int) reflect.Value {
	switch cArray.Elem().Type().String() {
	case "*main._Ctype_char":
		return reflect.ValueOf(append([]*C.char{}, unsafe.Slice(cArray.Interface().(**C.char), cArrayLen)...))
	case "*main._Ctype_struct_Options": // []*excelize.Options
		val := cArray.Interface().(**C.struct_Options)
		arr := unsafe.Slice(val, cArrayLen)
		return reflect.ValueOf(arr)
	case "main._Ctype_struct_Border":
		val := cArray.Interface().(*C.struct_Border)
		arr := unsafe.Slice(val, cArrayLen)
		return reflect.ValueOf(arr)
	case "main._Ctype_struct_ChartSeries":
		val := cArray.Interface().(*C.struct_ChartSeries)
		arr := unsafe.Slice(val, cArrayLen)
		return reflect.ValueOf(arr)
	case "main._Ctype_struct_ConditionalFormatOptions":
		val := cArray.Interface().(*C.struct_ConditionalFormatOptions)
		arr := unsafe.Slice(val, cArrayLen)
		return reflect.ValueOf(arr)
	case "main._Ctype_struct_PivotTableField":
		val := cArray.Interface().(*C.struct_PivotTableField)
		arr := unsafe.Slice(val, cArrayLen)
		return reflect.ValueOf(arr)
	case "main._Ctype_struct_RichTextRun":
		val := cArray.Interface().(*C.struct_RichTextRun)
		arr := unsafe.Slice(val, cArrayLen)
		return reflect.ValueOf(arr)
	case "main._Ctype_struct_Selection":
		val := cArray.Interface().(*C.struct_Selection)
		arr := unsafe.Slice(val, cArrayLen)
		return reflect.ValueOf(arr)
	}
	return cArray
}

// cValueToGo convert C language object to Go variable base on the given Go
// structure types, this function extract each fields of the structure from
// object recursively.
func cValueToGo(cVal reflect.Value, goType reflect.Type) (reflect.Value, error) {
	result := reflect.New(goType)
	s := result.Elem()
	for resultFieldIdx := 0; resultFieldIdx < s.NumField(); resultFieldIdx++ {
		field := goType.Field(resultFieldIdx)
		if unicode.IsLower(rune(field.Name[0])) {
			continue
		}
		if goBaseTypes[field.Type.Kind()] {
			cBaseVal := cVal.FieldByName(field.Name)
			goBaseVal, err := cToGoBaseType(cBaseVal, field.Type.Kind())
			if err != nil {
				return result, err
			}
			s.Field(resultFieldIdx).Set(goBaseVal.Convert(s.Field(resultFieldIdx).Type()))
			continue
		}
		switch field.Type.Kind() {
		case reflect.Ptr:
			// Pointer of the Go data type, for example: *excelize.Options or *string
			ptrType := field.Type.Elem()
			if !goBaseTypes[ptrType.Kind()] {
				// Pointer of the Go struct, for example: *excelize.Options
				cObjVal := cVal.FieldByName(field.Name)
				if cObjVal.Elem().CanAddr() {
					v, err := cValueToGo(cObjVal.Elem(), ptrType)
					if err != nil {
						return result, err
					}
					s.Field(resultFieldIdx).Set(v)
				}
			}
			if goBaseTypes[ptrType.Kind()] {
				// Pointer of the Go basic data type, for example: *string
				cBaseVal := cVal.FieldByName(field.Name)
				if !cBaseVal.IsNil() {
					v, err := cToGoBaseType(cBaseVal.Elem(), ptrType.Kind())
					if err != nil {
						return result, err
					}
					x := reflect.New(ptrType)
					x.Elem().Set(v)
					s.Field(resultFieldIdx).Set(x.Elem().Addr())
				}
			}
		case reflect.Struct:
			// The Go struct, for example: excelize.Options, convert sub fields recursively
			structType := field.Type
			cObjVal := cVal.FieldByName(field.Name)
			v, err := cValueToGo(cObjVal, structType)
			if err != nil {
				return result, err
			}
			s.Field(resultFieldIdx).Set(v.Elem())
		case reflect.Slice:
			// The Go data type array, for example:
			// []*excelize.Options, []excelize.Options, []string, []*string
			ele := field.Type.Elem()
			cArray := cVal.FieldByName(field.Name)
			if cArray.IsZero() {
				continue
			}
			if ele.Kind() == reflect.Ptr {
				// Pointer array of the Go data type, for example: []*excelize.Options or []*string
				subEle := ele.Elem()
				cArrayLen := int(cVal.FieldByName(field.Name + "Len").Int())
				cArray = cToGoArray(cArray, cArrayLen)
				for i := 0; i < cArray.Len(); i++ {
					if goBaseTypes[subEle.Kind()] {
						// Pointer array of the Go basic data type, for example: []*string
						v, err := cToGoBaseType(cArray.Index(i), subEle.Kind())
						if err != nil {
							return result, err
						}
						x := reflect.New(subEle)
						x.Elem().Set(v)
						s.Field(resultFieldIdx).Set(reflect.Append(s.Field(resultFieldIdx), x.Elem().Addr()))
					} else {
						// Pointer array of the Go struct, for example: []*excelize.Options
						v, err := cValueToGo(cArray.Index(i).Elem(), subEle)
						if err != nil {
							return result, err
						}
						x := reflect.New(subEle)
						x.Elem().Set(v.Elem())
						s.Field(resultFieldIdx).Set(reflect.Append(s.Field(resultFieldIdx), x.Elem().Addr()))
					}
				}
			} else {
				// The Go data type array, for example: []excelize.Options or []string
				subEle := ele
				cArrayLen := int(cVal.FieldByName(field.Name + "Len").Int())
				if subEle.Kind() == reflect.Uint8 { // []byte
					buf := C.GoBytes(unsafe.Pointer(cArray.Interface().(*C.uchar)), C.int(cArrayLen))
					s.Field(resultFieldIdx).Set(reflect.ValueOf(buf))
					continue
				}
				cArray = cToGoArray(cArray, cArrayLen)
				for i := 0; i < cArray.Len(); i++ {
					if goBaseTypes[subEle.Kind()] {
						// The Go basic data type array, for example: []string
						v, err := cToGoBaseType(cArray.Index(i), subEle.Kind())
						if err != nil {
							return result, err
						}

						s.Field(resultFieldIdx).Set(reflect.Append(s.Field(resultFieldIdx), v))
					} else {
						// The Go struct array, for example: []excelize.Options
						v, err := cValueToGo(cArray.Index(i), subEle)
						if err != nil {
							return result, err
						}
						s.Field(resultFieldIdx).Set(reflect.Append(s.Field(resultFieldIdx), v.Elem()))
					}
				}
			}
		}
	}
	return result, nil
}

// goBaseTypeToC convert Go basic data type value to C variable.
func goBaseTypeToC(goVal reflect.Value, kind reflect.Kind) (reflect.Value, error) {
	fn, ok := goBaseValueToCFuncs[kind]
	if !ok {
		return reflect.ValueOf(nil), errors.New("invalid argument data type" + kind.String())
	}
	return fn(goVal, kind)
}

// goValueToC convert Go variable to C object base on the given Go structure
// types, this function extract each fields of the structure from structure
// variable recursively.
func goValueToC(goVal, cVal reflect.Value) (reflect.Value, error) {
	result := cVal
	c := result.Elem()
	for i := 0; i < goVal.Type().NumField(); i++ {
		cField, _ := c.Type().FieldByName(goVal.Type().Field(i).Name)
		field := goVal.Type().Field(i)
		if unicode.IsLower(rune(field.Name[0])) {
			continue
		}
		if goBaseTypes[field.Type.Kind()] {
			goBaseVal := goVal.FieldByName(field.Name)
			cBaseVal, err := goBaseTypeToC(goBaseVal, goBaseVal.Type().Kind())
			if err != nil {
				return result, err
			}
			c.FieldByName(field.Name).Set(cBaseVal.Convert(cField.Type))
			continue
		}
		switch goVal.Type().Field(i).Type.Kind() {
		case reflect.Ptr:
			// Pointer of the Go data type, for example: *excelize.Options or *string
			ptrType := field.Type.Elem()
			if !goBaseTypes[ptrType.Kind()] {
				// Pointer of the Go struct, for example: *excelize.Options
				goStructVal := goVal.Field(i)
				if !goStructVal.IsNil() {
					cPtr := C.malloc(C.size_t(cField.Type.Elem().Size()))
					cStructPtr := reflect.NewAt(cField.Type.Elem(), cPtr)
					v, err := goValueToC(goStructVal.Elem(), cStructPtr)
					if err != nil {
						return result, err
					}
					c.FieldByName(field.Name).Set(v)
				}
			}
			if goBaseTypes[ptrType.Kind()] {
				// Pointer of the Go basic data type, for example: *string
				goBaseVal := goVal.Field(i)
				if !goBaseVal.IsNil() {
					v, err := goBaseTypeToC(goBaseVal.Elem(), ptrType.Kind())
					if err != nil {
						return result, err
					}
					cValPtr := C.malloc(C.size_t(unsafe.Sizeof(cField.Type.Elem().Size())))
					ptrVal := reflect.NewAt(v.Type(), cValPtr).Elem()
					ptrVal.Set(v)
					c.FieldByName(field.Name).Set(ptrVal.Addr())
				} else {
					c.FieldByName(field.Name).Set(reflect.Zero(cField.Type))
				}
			}
		case reflect.Struct:
			// The Go struct, for example: excelize.Options, convert sub fields recursively
			goStructVal := goVal.Field(i)
			v, err := goValueToC(goStructVal, reflect.New(cField.Type))
			if err != nil {
				return result, err
			}
			c.FieldByName(field.Name).Set(v.Elem())
		case reflect.Slice:
			// The Go data type array, for example:
			// []*excelize.Options, []excelize.Options, []string, []*string
			goSlice := goVal.Field(i)
			ele := goSlice.Type().Elem()
			l, err := goBaseTypeToC(reflect.ValueOf(goSlice.Len()), reflect.Int)
			if err != nil {
				return result, err
			}
			c.FieldByName(field.Name + "Len").Set(l)
			if ele.Kind() == reflect.Uint8 { // []byte
				byteSlice := goSlice.Bytes()
				cArray := C.malloc(C.size_t(len(byteSlice)))
				cSlice := unsafe.Slice((*byte)(cArray), len(byteSlice))
				copy(cSlice, byteSlice)
				c.FieldByName(field.Name).Set(reflect.ValueOf((*C.uchar)(cArray)))
				continue
			}
			cArray := C.malloc(C.size_t(goSlice.Len()) * C.size_t(cField.Type.Elem().Size()))
			for j := 0; j < goSlice.Len(); j++ {
				if goBaseTypes[ele.Kind()] {
					// The Go basic data type array, for example: []string
					cBaseVal, err := goBaseTypeToC(goSlice.Index(j), goSlice.Index(j).Type().Kind())
					if err != nil {
						return result, err
					}
					elePtr := unsafe.Pointer(uintptr(cArray) + uintptr(j)*cBaseVal.Type().Size())
					ele := reflect.NewAt(cBaseVal.Type(), elePtr).Elem()
					ele.Set(cBaseVal)
				} else {
					// The Go struct array, for example: []excelize.Options
					cPtr := C.malloc(C.size_t(cField.Type.Elem().Size()))
					cStructPtr := reflect.NewAt(cField.Type.Elem(), cPtr)
					v, err := goValueToC(goSlice.Index(j), cStructPtr)
					if err != nil {
						return result, err
					}
					elePtr := unsafe.Pointer(uintptr(cArray) + uintptr(j)*cField.Type.Elem().Size())
					ele := reflect.NewAt(cField.Type.Elem(), elePtr).Elem()
					ele.Set(reflect.NewAt(cField.Type.Elem(), unsafe.Pointer(v.Pointer())).Elem())
				}
			}
			c.FieldByName(field.Name).Set(reflect.NewAt(cField.Type.Elem(), cArray))
		}
	}
	return result, nil
}

// cInterfaceToGo convert C interface to Go interface data type value.
func cInterfaceToGo(val C.struct_Interface) interface{} {
	switch val.Type {
	case Int:
		return int(val.Integer)
	case Int32:
		return int32(val.Integer32)
	case String:
		return C.GoString(val.String)
	case Float:
		return float64(val.Float64)
	case Boolean:
		return bool(val.Boolean)
	case Time:
		return time.Unix(int64(val.Integer), 0)
	default:
		return nil
	}
}

// goInterfaceToC convert Go interface to C interface data type value.
func goInterfaceToC(val interface{}) C.struct_Interface {
	switch v := val.(type) {
	case int:
		return C.struct_Interface{Type: Int, Integer: C.int(v)}
	case int32:
		return C.struct_Interface{Type: Int32, Integer32: C.int(v)}
	case string:
		return C.struct_Interface{Type: String, String: C.CString(v)}
	case float64:
		return C.struct_Interface{Type: Float, Float64: C.double(v)}
	case bool:
		return C.struct_Interface{Type: Boolean, Boolean: C._Bool(v)}
	case time.Time:
		return C.struct_Interface{Type: Time, Integer: C.int(v.Unix())}
	default:
		return C.struct_Interface{Type: Nil}
	}
}

// AddChart provides the method to add chart in a sheet by given chart format
// set (such as offset, scale, aspect ratio setting and print settings) and
// properties set.
//
//export AddChart
func AddChart(idx int, sheet, cell *C.char, chart *C.struct_Chart, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	charts := make([]*excelize.Chart, length)
	for i, c := range unsafe.Slice(chart, length) {
		goVal, err := cValueToGo(reflect.ValueOf(c), reflect.TypeOf(excelize.Chart{}))
		if err != nil {
			return C.CString(err.Error())
		}
		c := goVal.Elem().Interface().(excelize.Chart)
		charts[i] = &c
	}
	if len(charts) > 1 {
		if err := f.(*excelize.File).AddChart(C.GoString(sheet), C.GoString(cell), charts[0], charts[1:]...); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).AddChart(C.GoString(sheet), C.GoString(cell), charts[0]); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddChartSheet provides the method to create a chartsheet by given chart
// format set (such as offset, scale, aspect ratio setting and print settings)
// and properties set. In Excel a chartsheet is a worksheet that only contains
// a chart.
//
//export AddChartSheet
func AddChartSheet(idx int, sheet *C.char, chart *C.struct_Chart, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	charts := make([]*excelize.Chart, length)
	for i, c := range unsafe.Slice(chart, length) {
		goVal, err := cValueToGo(reflect.ValueOf(c), reflect.TypeOf(excelize.Chart{}))
		if err != nil {
			return C.CString(err.Error())
		}
		c := goVal.Elem().Interface().(excelize.Chart)
		charts[i] = &c
	}
	if len(charts) > 1 {
		if err := f.(*excelize.File).AddChartSheet(C.GoString(sheet), charts[0], charts[1:]...); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).AddChartSheet(C.GoString(sheet), charts[0]); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddComment provides the method to add comments in a sheet by giving the
// worksheet name, cell reference, and format set (such as author and text).
// Note that the maximum author name length is 255 and the max text length is
// 32512.
//
//export AddComment
func AddComment(idx int, sheet *C.char, opts *C.struct_Comment) *C.char {
	var comment excelize.Comment
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Comment{}))
	if err != nil {
		return C.CString(err.Error())
	}
	comment = goVal.Elem().Interface().(excelize.Comment)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).AddComment(C.GoString(sheet), comment); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddFormControl provides the method to add form control object in a worksheet
// by given worksheet name and form control options. Supported form control
// type: button, check box, group box, label, option button, scroll bar and
// spinner. If set macro for the form control, the workbook extension should be
// XLSM or XLTM. Scroll value must be between 0 and 30000.
//
//export AddFormControl
func AddFormControl(idx int, sheet *C.char, opts *C.struct_FormControl) *C.char {
	var options excelize.FormControl
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.FormControl{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.FormControl)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).AddFormControl(C.GoString(sheet), options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddHeaderFooterImage provides a mechanism to set the graphics that can be
// referenced in the header and footer definitions via &G, supported image
// types: EMF, EMZ, GIF, ICO, JPEG, JPG, PNG, SVG, TIF, TIFF, WMF, and WMZ.
//
// The extension should be provided with a "." in front, e.g. ".png".
// The width and height should have units in them, e.g. "100pt".
//
//export AddHeaderFooterImage
func AddHeaderFooterImage(idx int, sheet *C.char, opts *C.struct_HeaderFooterImageOptions) *C.char {
	var options excelize.HeaderFooterImageOptions
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.HeaderFooterImageOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.HeaderFooterImageOptions)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).AddHeaderFooterImage(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddIgnoredErrors provides the method to ignored error for a range of cells.
//
//export AddIgnoredErrors
func AddIgnoredErrors(idx int, sheet, rangeRef *C.char, ignoredErrorsType C.int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).AddIgnoredErrors(C.GoString(sheet), C.GoString(rangeRef), excelize.IgnoredErrorsType(ignoredErrorsType)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddPicture add picture in a sheet by given picture format set (such as
// offset, scale, aspect ratio setting and print settings) and file path,
// supported image types: BMP, EMF, EMZ, GIF, ICO, JPEG, JPG, PNG, SVG, TIF,
// TIFF, WMF and WMZ.
//
//export AddPicture
func AddPicture(idx int, sheet, cell, name *C.char, opts *C.struct_GraphicOptions) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.GraphicOptions{}))
		if err != nil {
			return C.CString(err.Error())
		}
		options := goVal.Elem().Interface().(excelize.GraphicOptions)
		if err := f.(*excelize.File).AddPicture(C.GoString(sheet), C.GoString(cell), C.GoString(name), &options); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).AddPicture(C.GoString(sheet), C.GoString(cell), C.GoString(name), nil); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddPictureFromBytes provides the method to add picture in a sheet by given
// picture format set (such as offset, scale, aspect ratio setting and print
// settings), file base name, extension name and file bytes, supported image
// types: EMF, EMZ, GIF, JPEG, JPG, PNG, SVG, TIF, TIFF, WMF, and WMZ. Note that
// this function only supports adding pictures placed over the cells currently,
// and doesn't support adding pictures placed in cells or creating the Kingsoft
// WPS Office embedded image cells
//
//export AddPictureFromBytes
func AddPictureFromBytes(idx int, sheet, cell *C.char, pic *C.struct_Picture) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	goVal, err := cValueToGo(reflect.ValueOf(*pic), reflect.TypeOf(excelize.Picture{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options := goVal.Elem().Interface().(excelize.Picture)
	if err := f.(*excelize.File).AddPictureFromBytes(C.GoString(sheet), C.GoString(cell), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddPivotTable provides the method to add pivot table by given pivot table
// options. Note that the same fields can not in Columns, Rows and Filter
// fields at the same time.
//
//export AddPivotTable
func AddPivotTable(idx int, opts *C.struct_PivotTableOptions) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.PivotTableOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options := goVal.Elem().Interface().(excelize.PivotTableOptions)
	if err := f.(*excelize.File).AddPivotTable(&options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddShape provides the method to add shape in a sheet by given worksheet
// name and shape format set (such as offset, scale, aspect ratio setting and
// print settings).
//
//export AddShape
func AddShape(idx int, sheet *C.char, opts *C.struct_Shape) *C.char {
	var options excelize.Shape
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Shape{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.Shape)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).AddShape(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddSlicer function inserts a slicer by giving the worksheet name and slicer
// settings.
//
//export AddSlicer
func AddSlicer(idx int, sheet *C.char, opts *C.struct_SlicerOptions) *C.char {
	var options excelize.SlicerOptions
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.SlicerOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.SlicerOptions)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).AddSlicer(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddSparkline provides a function to add sparklines to the worksheet by
// given formatting options. Sparklines are small charts that fit in a single
// cell and are used to show trends in data. Sparklines are a feature of Excel
// 2010 and later only. You can write them to workbook that can be read by Excel
// 2007, but they won't be displayed.
//
//export AddSparkline
func AddSparkline(idx int, sheet *C.char, opts *C.struct_SparklineOptions) *C.char {
	var options excelize.SparklineOptions
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.SparklineOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.SparklineOptions)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).AddSparkline(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddTable provides the method to add table in a worksheet by given worksheet
// name, range reference and format set.
//
//export AddTable
func AddTable(idx int, sheet *C.char, table *C.struct_Table) *C.char {
	var tbl excelize.Table
	goVal, err := cValueToGo(reflect.ValueOf(*table), reflect.TypeOf(excelize.Table{}))
	if err != nil {
		return C.CString(err.Error())
	}
	tbl = goVal.Elem().Interface().(excelize.Table)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).AddTable(C.GoString(sheet), &tbl); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AddVBAProject provides the method to add vbaProject.bin file which contains
// functions and/or macros. The file extension should be XLSM or XLTM.
//
//export AddVBAProject
func AddVBAProject(idx int, file *C.uchar, fileLen C.int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	buf := C.GoBytes(unsafe.Pointer(file), fileLen)
	if err := f.(*excelize.File).AddVBAProject(buf); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// AutoFilter provides the method to add auto filter in a worksheet by given
// worksheet name, range reference and settings. An auto filter in Excel is a
// way of filtering a 2D range of data based on some simple criteria.
//
//export AutoFilter
func AutoFilter(idx int, sheet, rangeRef *C.char, opts *C.struct_AutoFilterOptions, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	options := make([]excelize.AutoFilterOptions, length)
	for i, val := range unsafe.Slice(opts, length) {
		goVal, err := cValueToGo(reflect.ValueOf(val), reflect.TypeOf(excelize.AutoFilterOptions{}))
		if err != nil {
			return C.CString(err.Error())
		}
		options[i] = goVal.Elem().Interface().(excelize.AutoFilterOptions)
	}
	if err := f.(*excelize.File).AutoFilter(C.GoString(sheet), C.GoString(rangeRef), options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// CalcCellValue provides a function to get calculated cell value. This feature
// is currently in working processing. Iterative calculation, implicit
// intersection, explicit intersection, array formula, table formula and some
// other formulas are not supported currently.
//
//export CalcCellValue
func CalcCellValue(idx int, sheet, cell *C.char, opts *C.struct_Options) C.struct_StringErrorResult {
	var options excelize.Options
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringErrorResult{val: C.CString(emptyString), err: C.CString(errFilePtr)}
	}
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_StringErrorResult{val: C.CString(emptyString), err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	val, err := f.(*excelize.File).CalcCellValue(C.GoString(sheet), C.GoString(cell), options)
	if err != nil {
		return C.struct_StringErrorResult{val: C.CString(val), err: C.CString(err.Error())}
	}
	return C.struct_StringErrorResult{val: C.CString(val), err: C.CString(emptyString)}
}

// CellNameToCoordinates converts alphanumeric cell name to [X, Y] coordinates
// or returns an error.
//
//export CellNameToCoordinates
func CellNameToCoordinates(cell *C.char) C.struct_CellNameToCoordinatesResult {
	col, row, err := excelize.CellNameToCoordinates(C.GoString(cell))
	if err != nil {
		return C.struct_CellNameToCoordinatesResult{col: C.int(col), row: C.int(row), err: C.CString(err.Error())}
	}
	return C.struct_CellNameToCoordinatesResult{col: C.int(col), row: C.int(row), err: C.CString(emptyString)}
}

// ColumnNameToNumber provides a function to convert Excel sheet column name
// (case-insensitive) to int. The function returns an error if column name
// incorrect.
//
//export ColumnNameToNumber
func ColumnNameToNumber(name *C.char) C.struct_IntErrorResult {
	col, err := excelize.ColumnNameToNumber(C.GoString(name))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(col), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(col), err: C.CString(emptyString)}
}

// ColumnNumberToName provides a function to convert the integer to Excel
// sheet column title.
//
//export ColumnNumberToName
func ColumnNumberToName(num int) C.struct_StringErrorResult {
	col, err := excelize.ColumnNumberToName(num)
	if err != nil {
		return C.struct_StringErrorResult{val: C.CString(col), err: C.CString(err.Error())}
	}
	return C.struct_StringErrorResult{val: C.CString(col), err: C.CString(emptyString)}
}

// CoordinatesToCellName converts [X, Y] coordinates to alpha-numeric cell name
// or returns an error.
//
//export CoordinatesToCellName
func CoordinatesToCellName(col, row int, abs bool) C.struct_StringErrorResult {
	cell, err := excelize.CoordinatesToCellName(col, row, abs)
	if err != nil {
		return C.struct_StringErrorResult{val: C.CString(cell), err: C.CString(err.Error())}
	}
	return C.struct_StringErrorResult{val: C.CString(cell), err: C.CString(emptyString)}
}

// Close closes and cleanup the open temporary file for the spreadsheet.
//
//export Close
func Close(idx int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	defer files.Delete(idx)
	if err := f.(*excelize.File).Close(); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// CopySheet provides a function to duplicate a worksheet by gave source and
// target worksheet index. Note that currently doesn't support duplicate
// workbooks that contain tables, charts or pictures.
//
//export CopySheet
func CopySheet(idx, from, to int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).CopySheet(from, to); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DeleteChart provides a function to delete chart in spreadsheet by given
// worksheet name and cell reference.
//
//export DeleteChart
func DeleteChart(idx int, sheet, cell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DeleteChart(C.GoString(sheet), C.GoString(cell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DeleteComment provides the method to delete comment in a sheet by given
// worksheet name.
//
//export DeleteComment
func DeleteComment(idx int, sheet, cell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DeleteComment(C.GoString(sheet), C.GoString(cell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DeleteDefinedName provides a function to delete the defined names of the
// workbook or worksheet. If not specified scope, the default scope is
// workbook.
//
//export DeleteDefinedName
func DeleteDefinedName(idx int, definedName *C.struct_DefinedName) *C.char {
	var df excelize.DefinedName
	goVal, err := cValueToGo(reflect.ValueOf(*definedName), reflect.TypeOf(excelize.DefinedName{}))
	if err != nil {
		return C.CString(err.Error())
	}
	df = goVal.Elem().Interface().(excelize.DefinedName)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DeleteDefinedName(&df); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DeleteFormControl provides the method to delete form control in a worksheet
// by given worksheet name and cell reference.
//
//export DeleteFormControl
func DeleteFormControl(idx int, sheet, cell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DeleteFormControl(C.GoString(sheet), C.GoString(cell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DeletePicture provides a function to delete charts in spreadsheet by given
// worksheet name and cell reference. Note that the image file won't be
// deleted from the document currently.
//
//export DeletePicture
func DeletePicture(idx int, sheet, cell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DeletePicture(C.GoString(sheet), C.GoString(cell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DeleteSheet provides a function to delete worksheet in a workbook by given
// worksheet name. Use this method with caution, which will affect changes in
// references such as formulas, charts, and so on. If there is any referenced
// value of the deleted worksheet, it will cause a file error when you open
// it. This function will be invalid when only one worksheet is left.
//
//export DeleteSheet
func DeleteSheet(idx int, sheet *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DeleteSheet(C.GoString(sheet)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DeleteSlicer provides the method to delete a slicer by a given slicer name.
//
//export DeleteSlicer
func DeleteSlicer(idx int, name *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DeleteSlicer(C.GoString(name)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DuplicateRow inserts a copy of specified row (by its Excel row number)
// below. Use this method with caution, which will affect changes in
// references such as formulas, charts, and so on. If there is any referenced
// value of the worksheet, it will cause a file error when you open it. The
// excelize only partially updates these references currently.
//
//export DuplicateRow
func DuplicateRow(idx int, sheet *C.char, row int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DuplicateRow(C.GoString(sheet), row); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// DuplicateRowTo inserts a copy of specified row by it Excel number to
// specified row position moving down exists rows after target position. Use
// this method with caution, which will affect changes in references such as
// formulas, charts, and so on. If there is any referenced value of the
// worksheet, it will cause a file error when you open it. The excelize only
// partially updates these references currently.
//
//export DuplicateRowTo
func DuplicateRowTo(idx int, sheet *C.char, row, row2 int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).DuplicateRowTo(C.GoString(sheet), row, row2); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// GetActiveSheetIndex provides a function to get active sheet index of the
// spreadsheet. If not found the active sheet will be return integer 0.
//
//export GetActiveSheetIndex
func GetActiveSheetIndex(idx int) int {
	f, ok := files.Load(idx)
	if !ok {
		return 0
	}
	return f.(*excelize.File).GetActiveSheetIndex()
}

// GetAppProps provides a function to get document application properties.
//
//export GetAppProps
func GetAppProps(idx int) C.struct_GetAppPropsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetAppPropsResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetAppProps()
	if err != nil {
		return C.struct_GetAppPropsResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(*opts), reflect.ValueOf(&C.struct_AppProperties{}))
	if err != nil {
		return C.struct_GetAppPropsResult{err: C.CString(err.Error())}
	}
	return C.struct_GetAppPropsResult{opts: cVal.Elem().Interface().(C.struct_AppProperties), err: C.CString(emptyString)}
}

// GetCalcProps provides a function to gets calculation properties.
//
//export GetCalcProps
func GetCalcProps(idx int) C.struct_GetCalcPropsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetCalcPropsResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetCalcProps()
	if err != nil {
		return C.struct_GetCalcPropsResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(opts), reflect.ValueOf(&C.struct_CalcPropsOptions{}))
	if err != nil {
		return C.struct_GetCalcPropsResult{err: C.CString(err.Error())}
	}
	return C.struct_GetCalcPropsResult{opts: cVal.Elem().Interface().(C.struct_CalcPropsOptions), err: C.CString(emptyString)}
}

// GetCellFormula provides a function to get formula from cell by given
// worksheet name and cell reference in spreadsheet.
//
//export GetCellFormula
func GetCellFormula(idx int, sheet, cell *C.char) C.struct_StringErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringErrorResult{val: C.CString(emptyString), err: C.CString(errFilePtr)}
	}
	formula, err := f.(*excelize.File).GetCellFormula(C.GoString(sheet), C.GoString(cell))
	if err != nil {
		return C.struct_StringErrorResult{val: C.CString(formula), err: C.CString(err.Error())}
	}
	return C.struct_StringErrorResult{val: C.CString(formula), err: C.CString(emptyString)}
}

// GetCellHyperLink gets a cell hyperlink based on the given worksheet name and
// cell reference. If the cell has a hyperlink, it will return 'true' and
// the link address, otherwise it will return 'false' and an empty link
// address.
//
//export GetCellHyperLink
func GetCellHyperLink(idx int, sheet, cell *C.char) C.struct_GetCellHyperLinkResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetCellHyperLinkResult{link: false, target: C.CString(emptyString), err: C.CString(errFilePtr)}
	}
	link, target, err := f.(*excelize.File).GetCellHyperLink(C.GoString(sheet), C.GoString(cell))
	if err != nil {
		return C.struct_GetCellHyperLinkResult{link: C._Bool(link), target: C.CString(target), err: C.CString(err.Error())}
	}
	return C.struct_GetCellHyperLinkResult{link: C._Bool(link), target: C.CString(target), err: C.CString(emptyString)}
}

// GetCellRichText provides a function to get rich text of cell by given
// worksheet and cell reference.
//
//export GetCellRichText
func GetCellRichText(idx int, sheet, cell *C.char) C.struct_GetCellRichTextResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetCellRichTextResult{Err: C.CString(errFilePtr)}
	}
	runs, err := f.(*excelize.File).GetCellRichText(C.GoString(sheet), C.GoString(cell))
	if err != nil {
		return C.struct_GetCellRichTextResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(runs)) * C.size_t(unsafe.Sizeof(C.struct_RichTextRun{})))
	for i, r := range runs {
		cVal, err := goValueToC(reflect.ValueOf(r), reflect.ValueOf(&C.struct_RichTextRun{}))
		if err != nil {
			return C.struct_GetCellRichTextResult{Err: C.CString(err.Error())}
		}
		*(*C.struct_RichTextRun)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_RichTextRun{}))) = cVal.Elem().Interface().(C.struct_RichTextRun)
	}
	return C.struct_GetCellRichTextResult{RunsLen: C.int(len(runs)), Runs: (*C.struct_RichTextRun)(cArray), Err: C.CString(emptyString)}
}

// GetCellStyle provides a function to get cell style index by given worksheet
// name and cell reference. This function is concurrency safe.
//
//export GetCellStyle
func GetCellStyle(idx int, sheet, cell *C.char) C.struct_IntErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(errFilePtr)}
	}
	idx, err := f.(*excelize.File).GetCellStyle(C.GoString(sheet), C.GoString(cell))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(idx), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(idx), err: C.CString(emptyString)}
}

// GetCellValue provides a function to get formatted value from cell by given
// worksheet name and cell reference in spreadsheet. The return value is
// converted to the `string` data type. If the cell format can be applied to
// the value of a cell, the applied value will be returned, otherwise the
// original value will be returned. All cells' values will be the same in a
// merged range.
//
//export GetCellValue
func GetCellValue(idx int, sheet, cell *C.char, opts *C.struct_Options) C.struct_StringErrorResult {
	var options excelize.Options
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringErrorResult{val: C.CString(emptyString), err: C.CString(errFilePtr)}
	}
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_StringErrorResult{val: C.CString(emptyString), err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	val, err := f.(*excelize.File).GetCellValue(C.GoString(sheet), C.GoString(cell), options)
	if err != nil {
		return C.struct_StringErrorResult{val: C.CString(val), err: C.CString(err.Error())}
	}
	return C.struct_StringErrorResult{val: C.CString(val), err: C.CString(emptyString)}
}

// GetColOutlineLevel provides a function to get outline level of a single
// column by given worksheet name and column name.
//
//export GetColOutlineLevel
func GetColOutlineLevel(idx int, sheet, col *C.char) C.struct_IntErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(errFilePtr)}
	}
	val, err := f.(*excelize.File).GetColOutlineLevel(C.GoString(sheet), C.GoString(col))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(int32(val)), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(int32(val)), err: C.CString(emptyString)}
}

// GetColStyle provides a function to get column style ID by given worksheet
// name and column name. This function is concurrency safe.
//
//export GetColStyle
func GetColStyle(idx int, sheet, col *C.char) C.struct_IntErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(errFilePtr)}
	}
	val, err := f.(*excelize.File).GetColStyle(C.GoString(sheet), C.GoString(col))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(val), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(val), err: C.CString(emptyString)}
}

// GetColVisible provides a function to get visible of a single column by given
// worksheet name and column name. This function is concurrency safe.
//
//export GetColVisible
func GetColVisible(idx int, sheet, col *C.char) C.struct_BoolErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_BoolErrorResult{val: C._Bool(false), err: C.CString(errFilePtr)}
	}
	val, err := f.(*excelize.File).GetColVisible(C.GoString(sheet), C.GoString(col))
	if err != nil {
		return C.struct_BoolErrorResult{val: C._Bool(val), err: C.CString(err.Error())}
	}
	return C.struct_BoolErrorResult{val: C._Bool(val), err: C.CString(emptyString)}
}

// GetColWidth provides a function to get column width by given worksheet name
// and column name. This function is concurrency safe.
//
//export GetColWidth
func GetColWidth(idx int, sheet, col *C.char) C.struct_Float64ErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_Float64ErrorResult{val: C.double(0), err: C.CString(errFilePtr)}
	}
	val, err := f.(*excelize.File).GetColWidth(C.GoString(sheet), C.GoString(col))
	if err != nil {
		return C.struct_Float64ErrorResult{val: C.double(val), err: C.CString(err.Error())}
	}
	return C.struct_Float64ErrorResult{val: C.double(val), err: C.CString(emptyString)}
}

// GetCols gets the value of all cells by columns on the worksheet based on the
// given worksheet name, returned as a two-dimensional array, where the value
// of the cell is converted to the `string` type. If the cell format can be
// applied to the value of the cell, the applied value will be used, otherwise
// the original value will be used.
//
//export GetCols
func GetCols(idx int, sheet *C.char, opts *C.struct_Options) C.struct_StringMatrixErrorResult {
	var (
		options excelize.Options
		result  StringMatrixErrorResult
	)
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringMatrixErrorResult{err: C.CString(errFilePtr)}
	}
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_StringMatrixErrorResult{err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	cols, err := f.(*excelize.File).GetCols(C.GoString(sheet), options)
	if err != nil {
		return C.struct_StringMatrixErrorResult{err: C.CString(err.Error())}
	}
	for _, col := range cols {
		var c Cells
		c.Cell = append(c.Cell, col...)
		result.Row = append(result.Row, c)
	}
	cVal, err := goValueToC(reflect.ValueOf(result), reflect.ValueOf(&C.struct_StringMatrixErrorResult{}))
	if err != nil {
		return C.struct_StringMatrixErrorResult{err: C.CString(err.Error())}
	}
	ret := cVal.Elem().Interface().(C.struct_StringMatrixErrorResult)
	ret.err = C.CString(emptyString)
	return ret
}

// GetComments retrieves all comments in a worksheet by given worksheet name.
//
//export GetComments
func GetComments(idx int, sheet *C.char) C.struct_GetCommentsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetCommentsResult{Err: C.CString(errFilePtr)}
	}
	comments, err := f.(*excelize.File).GetComments(C.GoString(sheet))
	if err != nil {
		return C.struct_GetCommentsResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(comments)) * C.size_t(unsafe.Sizeof(C.struct_Comment{})))
	for i, r := range comments {
		cVal, err := goValueToC(reflect.ValueOf(r), reflect.ValueOf(&C.struct_Comment{}))
		if err != nil {
			return C.struct_GetCommentsResult{Err: C.CString(err.Error())}
		}
		*(*C.struct_Comment)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_Comment{}))) = cVal.Elem().Interface().(C.struct_Comment)
	}
	return C.struct_GetCommentsResult{CommentsLen: C.int(len(comments)), Comments: (*C.struct_Comment)(cArray), Err: C.CString(emptyString)}
}

// GetCustomProps provides a function to get custom file properties.
//
//export GetCustomProps
func GetCustomProps(idx int) C.struct_GetCustomPropsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetCustomPropsResult{Err: C.CString(errFilePtr)}
	}
	props, err := f.(*excelize.File).GetCustomProps()
	if err != nil {
		return C.struct_GetCustomPropsResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(props)) * C.size_t(unsafe.Sizeof(C.struct_CustomProperty{})))
	for i, prop := range props {
		cVal := C.struct_CustomProperty{Name: C.CString(prop.Name), Value: goInterfaceToC(prop.Value)}
		*(*C.struct_CustomProperty)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_CustomProperty{}))) = cVal
	}
	return C.struct_GetCustomPropsResult{CustomPropsLen: C.int(len(props)), CustomProps: (*C.struct_CustomProperty)(cArray), Err: C.CString(emptyString)}
}

// GetDefaultFont provides the default font name currently set in the
// workbook. The spreadsheet generated by excelize default font is Calibri.
//
//export GetDefaultFont
func GetDefaultFont(idx int) C.struct_StringErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringErrorResult{val: C.CString(emptyString), err: C.CString(errFilePtr)}
	}
	val, err := f.(*excelize.File).GetDefaultFont()
	if err != nil {
		return C.struct_StringErrorResult{val: C.CString(val), err: C.CString(err.Error())}
	}
	return C.struct_StringErrorResult{val: C.CString(val), err: C.CString(emptyString)}
}

// GetDefinedName provides a function to get the defined names of the workbook
// or worksheet.
//
//export GetDefinedName
func GetDefinedName(idx int) C.struct_GetDefinedNameResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetDefinedNameResult{Err: C.CString(errFilePtr)}
	}
	definedNames := f.(*excelize.File).GetDefinedName()
	cArray := C.malloc(C.size_t(len(definedNames)) * C.size_t(unsafe.Sizeof(C.struct_DefinedName{})))
	for i, dn := range definedNames {
		cVal, err := goValueToC(reflect.ValueOf(dn), reflect.ValueOf(&C.struct_DefinedName{}))
		if err != nil {
			return C.struct_GetDefinedNameResult{Err: C.CString(err.Error())}
		}
		*(*C.struct_DefinedName)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_DefinedName{}))) = cVal.Elem().Interface().(C.struct_DefinedName)
	}
	return C.struct_GetDefinedNameResult{DefinedNamesLen: C.int(len(definedNames)), DefinedNames: (*C.struct_DefinedName)(cArray), Err: C.CString(emptyString)}
}

// GetDocProps provides a function to get document core properties.
//
//export GetDocProps
func GetDocProps(idx int) C.struct_GetDocPropsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetDocPropsResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetDocProps()
	if err != nil {
		return C.struct_GetDocPropsResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(*opts), reflect.ValueOf(&C.struct_DocProperties{}))
	if err != nil {
		return C.struct_GetDocPropsResult{err: C.CString(err.Error())}
	}
	return C.struct_GetDocPropsResult{opts: cVal.Elem().Interface().(C.struct_DocProperties), err: C.CString(emptyString)}
}

// GetFormControls retrieves all form controls in a worksheet by a given
// worksheet name. Note that, this function does not support getting the width
// and height of the form controls currently.
//
//export GetFormControls
func GetFormControls(idx int, sheet *C.char) C.struct_GetFormControlsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetFormControlsResult{Err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetFormControls(C.GoString(sheet))
	if err != nil {
		return C.struct_GetFormControlsResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(opts)) * C.size_t(unsafe.Sizeof(C.struct_FormControl{})))
	for i, opt := range opts {
		cVal, err := goValueToC(reflect.ValueOf(opt), reflect.ValueOf(&C.struct_FormControl{}))
		if err != nil {
			return C.struct_GetFormControlsResult{Err: C.CString(err.Error())}
		}
		*(*C.struct_FormControl)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_FormControl{}))) = cVal.Elem().Interface().(C.struct_FormControl)
	}
	return C.struct_GetFormControlsResult{FormControlsLen: C.int(len(opts)), FormControls: (*C.struct_FormControl)(cArray), Err: C.CString(emptyString)}
}

// GetHyperLinkCells returns cell references which contain hyperlinks in a
// given worksheet name and link type. The optional parameter 'linkType' use for
// specific link type,the optional values are "External" for website links,
// "Location" for moving to one of cell in this workbook, "None" for no links.
// If linkType is empty, it will return all hyperlinks in the worksheet.
//
//export GetHyperLinkCells
func GetHyperLinkCells(idx int, sheet, linkType *C.char) C.struct_StringArrayErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringArrayErrorResult{Err: C.CString(errFilePtr)}
	}
	result, err := f.(*excelize.File).GetHyperLinkCells(C.GoString(sheet), C.GoString(linkType))
	if err != nil {
		return C.struct_StringArrayErrorResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(result)) * C.size_t(unsafe.Sizeof(uintptr(0))))
	for i, v := range result {
		*(*unsafe.Pointer)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(uintptr(0)))) = unsafe.Pointer(C.CString(v))
	}
	return C.struct_StringArrayErrorResult{ArrLen: C.int(len(result)), Arr: (**C.char)(cArray), Err: C.CString(emptyString)}
}

// GetMergeCells provides a function to get all merged cells from a specific
// worksheet. If the `withoutValues` parameter is set to true, it will not
// return the cell values of merged cells, only the range reference will be
// returned.
//
//export GetMergeCells
func GetMergeCells(idx int, sheet *C.char, withoutValues bool) C.struct_StringMatrixErrorResult {
	var result StringMatrixErrorResult
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringMatrixErrorResult{err: C.CString(errFilePtr)}
	}
	rows, err := f.(*excelize.File).GetMergeCells(C.GoString(sheet), withoutValues)
	if err != nil {
		return C.struct_StringMatrixErrorResult{err: C.CString(err.Error())}
	}
	for _, row := range rows {
		var r Cells
		r.Cell = append(r.Cell, row...)
		result.Row = append(result.Row, r)
	}
	cVal, err := goValueToC(reflect.ValueOf(result), reflect.ValueOf(&C.struct_StringMatrixErrorResult{}))
	if err != nil {
		return C.struct_StringMatrixErrorResult{err: C.CString(err.Error())}
	}
	ret := cVal.Elem().Interface().(C.struct_StringMatrixErrorResult)
	ret.err = C.CString(emptyString)
	return ret
}

// GetPageLayout provides a function to gets worksheet page layout.
//
//export GetPageLayout
func GetPageLayout(idx int, sheet *C.char) C.struct_GetPageLayoutResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetPageLayoutResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetPageLayout(C.GoString(sheet))
	if err != nil {
		return C.struct_GetPageLayoutResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(opts), reflect.ValueOf(&C.struct_PageLayoutOptions{}))
	if err != nil {
		return C.struct_GetPageLayoutResult{err: C.CString(err.Error())}
	}
	return C.struct_GetPageLayoutResult{opts: cVal.Elem().Interface().(C.struct_PageLayoutOptions), err: C.CString(emptyString)}
}

// GetPageMargins provides a function to get worksheet page margins.
//
//export GetPageMargins
func GetPageMargins(idx int, sheet *C.char) C.struct_GetPageMarginsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetPageMarginsResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetPageMargins(C.GoString(sheet))
	if err != nil {
		return C.struct_GetPageMarginsResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(opts), reflect.ValueOf(&C.struct_PageLayoutMarginsOptions{}))
	if err != nil {
		return C.struct_GetPageMarginsResult{err: C.CString(err.Error())}
	}
	return C.struct_GetPageMarginsResult{opts: cVal.Elem().Interface().(C.struct_PageLayoutMarginsOptions), err: C.CString(emptyString)}
}

// GetPictures provides a function to get picture meta info and raw content
// embed in spreadsheet by given worksheet and cell name. This function
// returns the image contents as []byte data types. This function is
// concurrency safe. Note that this function currently does not support
// retrieving all properties from the image's Format property, and the value of
// the ScaleX and ScaleY property is a floating-point number greater than 0 with
// a precision of two decimal places.
//
//export GetPictures
func GetPictures(idx int, sheet, cell *C.char) C.struct_GetPicturesResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetPicturesResult{Err: C.CString(errFilePtr)}
	}
	pics, err := f.(*excelize.File).GetPictures(C.GoString(sheet), C.GoString(cell))
	if err != nil {
		return C.struct_GetPicturesResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(pics)) * C.size_t(unsafe.Sizeof(C.struct_Picture{})))
	for i, pic := range pics {
		cVal, err := goValueToC(reflect.ValueOf(pic), reflect.ValueOf(&C.struct_Picture{}))
		if err != nil {
			return C.struct_GetPicturesResult{Err: C.CString(err.Error())}
		}
		*(*C.struct_Picture)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_Picture{}))) = cVal.Elem().Interface().(C.struct_Picture)
	}
	return C.struct_GetPicturesResult{PicturesLen: C.int(len(pics)), Pictures: (*C.struct_Picture)(cArray), Err: C.CString(emptyString)}
}

// GetPivotTables returns all pivot table definitions in a worksheet by given
// worksheet name. Currently only support get pivot table cache with worksheet
// source type, and doesn't support source types: external, consolidation
// and scenario.
//
//export GetPivotTables
func GetPivotTables(idx int, sheet *C.char) C.struct_GetPivotTablesResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetPivotTablesResult{Err: C.CString(errFilePtr)}
	}
	pivotTables, err := f.(*excelize.File).GetPivotTables(C.GoString(sheet))
	if err != nil {
		return C.struct_GetPivotTablesResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(pivotTables)) * C.size_t(unsafe.Sizeof(C.struct_PivotTableOptions{})))
	for i, tbl := range pivotTables {
		cVal, err := goValueToC(reflect.ValueOf(tbl), reflect.ValueOf(&C.struct_PivotTableOptions{}))
		if err != nil {
			return C.struct_GetPivotTablesResult{Err: C.CString(err.Error())}
		}
		*(*C.struct_PivotTableOptions)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_PivotTableOptions{}))) = cVal.Elem().Interface().(C.struct_PivotTableOptions)
	}
	return C.struct_GetPivotTablesResult{PivotTablesLen: C.int(len(pivotTables)), PivotTables: (*C.struct_PivotTableOptions)(cArray), Err: C.CString(emptyString)}
}

// GetRowHeight provides a function to get row height by given worksheet name
// and row number.
//
//export GetRowHeight
func GetRowHeight(idx int, sheet *C.char, row C.int) C.struct_Float64ErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_Float64ErrorResult{val: C.double(0), err: C.CString(errFilePtr)}
	}
	val, err := f.(*excelize.File).GetRowHeight(C.GoString(sheet), int(row))
	if err != nil {
		return C.struct_Float64ErrorResult{val: C.double(val), err: C.CString(err.Error())}
	}
	return C.struct_Float64ErrorResult{val: C.double(val), err: C.CString(emptyString)}
}

// GetRowOutlineLevel provides a function to get outline level of a single row
// by given worksheet name and row number.
//
//export GetRowOutlineLevel
func GetRowOutlineLevel(idx int, sheet *C.char, row C.int) C.struct_IntErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(errFilePtr)}
	}
	val, err := f.(*excelize.File).GetRowOutlineLevel(C.GoString(sheet), int(row))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(int32(val)), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(int32(val)), err: C.CString(emptyString)}
}

// GetRowVisible provides a function to get visible of a single row by given
// worksheet name and Excel row number.
//
//export GetRowVisible
func GetRowVisible(idx int, sheet *C.char, row int) C.struct_BoolErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_BoolErrorResult{val: C._Bool(false), err: C.CString(errFilePtr)}
	}
	val, err := f.(*excelize.File).GetRowVisible(C.GoString(sheet), row)
	if err != nil {
		return C.struct_BoolErrorResult{val: C._Bool(val), err: C.CString(err.Error())}
	}
	return C.struct_BoolErrorResult{val: C._Bool(val), err: C.CString(emptyString)}
}

// GetRows return all the rows in a sheet by given worksheet name, returned as
// a two-dimensional array, where the value of the cell is converted to the
// string type. If the cell format can be applied to the value of the cell,
// the applied value will be used, otherwise the original value will be used.
// GetRows fetched the rows with value or formula cells, the continually blank
// cells in the tail of each row will be skipped, so the length of each row
// may be inconsistent.
//
//export GetRows
func GetRows(idx int, sheet *C.char, opts *C.struct_Options) C.struct_StringMatrixErrorResult {
	var (
		options excelize.Options
		result  StringMatrixErrorResult
	)
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringMatrixErrorResult{err: C.CString(errFilePtr)}
	}
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_StringMatrixErrorResult{err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	rows, err := f.(*excelize.File).GetRows(C.GoString(sheet), options)
	if err != nil {
		return C.struct_StringMatrixErrorResult{err: C.CString(err.Error())}
	}
	for _, row := range rows {
		var r Cells
		r.Cell = append(r.Cell, row...)
		result.Row = append(result.Row, r)
	}
	cVal, err := goValueToC(reflect.ValueOf(result), reflect.ValueOf(&C.struct_StringMatrixErrorResult{}))
	if err != nil {
		return C.struct_StringMatrixErrorResult{err: C.CString(err.Error())}
	}
	ret := cVal.Elem().Interface().(C.struct_StringMatrixErrorResult)
	ret.err = C.CString(emptyString)
	return ret
}

// GetSheetDimension provides the method to get the used range of the worksheet.
//
//export GetSheetDimension
func GetSheetDimension(idx int, sheet *C.char) C.struct_StringErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringErrorResult{val: C.CString(emptyString), err: C.CString(errFilePtr)}
	}
	dimension, err := f.(*excelize.File).GetSheetDimension(C.GoString(sheet))
	if err != nil {
		return C.struct_StringErrorResult{val: C.CString(dimension), err: C.CString(err.Error())}
	}
	return C.struct_StringErrorResult{val: C.CString(dimension), err: C.CString(emptyString)}
}

// GetSheetIndex provides a function to get a sheet index of the workbook by
// the given sheet name. If the given sheet name is invalid or sheet doesn't
// exist, it will return an integer type value -1.
//
//export GetSheetIndex
func GetSheetIndex(idx int, sheet *C.char) C.struct_IntErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(-1), err: C.CString(errFilePtr)}
	}
	idx, err := f.(*excelize.File).GetSheetIndex(C.GoString(sheet))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(idx), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(idx), err: C.CString(emptyString)}
}

// GetSheetList provides a function to get worksheets, chart sheets, and
// dialog sheets name list of the workbook.
//
//export GetSheetList
func GetSheetList(idx int) C.struct_StringArrayErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringArrayErrorResult{Err: C.CString(errFilePtr)}
	}
	result := f.(*excelize.File).GetSheetList()
	cArray := C.malloc(C.size_t(len(result)) * C.size_t(unsafe.Sizeof(uintptr(0))))
	for i, v := range result {
		*(*unsafe.Pointer)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(uintptr(0)))) = unsafe.Pointer(C.CString(v))
	}
	return C.struct_StringArrayErrorResult{ArrLen: C.int(len(result)), Arr: (**C.char)(cArray), Err: C.CString(emptyString)}
}

// GetSheetMap provides a function to get worksheets, chart sheets, dialog
// sheets ID and name map of the workbook.
//
//export GetSheetMap
func GetSheetMap(idx int) C.struct_GetSheetMapResult {
	type IntStringResult struct {
		K int
		V string
	}
	type GetSheetMapResult struct {
		Arr []IntStringResult
		Err string
	}
	var result GetSheetMapResult
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetSheetMapResult{
			Err: C.CString(errFilePtr),
		}
	}
	for k, v := range f.(*excelize.File).GetSheetMap() {
		result.Arr = append(result.Arr, IntStringResult{K: k, V: v})
	}

	cVal, err := goValueToC(reflect.ValueOf(result), reflect.ValueOf(&C.struct_GetSheetMapResult{}))
	if err != nil {
		return C.struct_GetSheetMapResult{Err: C.CString(err.Error())}
	}
	ret := cVal.Elem().Interface().(C.struct_GetSheetMapResult)
	ret.Err = C.CString(emptyString)
	return ret
}

// GetSheetName provides a function to get the sheet name by the given worksheet index.
// If the given worksheet index is invalid, it will return an error.
//
//export GetSheetName
func GetSheetName(idx int, sheetIndex int) C.struct_StringErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringErrorResult{val: C.CString(emptyString), err: C.CString(errFilePtr)}
	}
	return C.struct_StringErrorResult{val: C.CString(f.(*excelize.File).GetSheetName(sheetIndex)), err: C.CString(emptyString)}
}

// GetSheetProps provides a function to get worksheet properties.
//
//export GetSheetProps
func GetSheetProps(idx int, sheet *C.char) C.struct_GetSheetPropsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetSheetPropsResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetSheetProps(C.GoString(sheet))
	if err != nil {
		return C.struct_GetSheetPropsResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(opts), reflect.ValueOf(&C.struct_SheetPropsOptions{}))
	if err != nil {
		return C.struct_GetSheetPropsResult{err: C.CString(err.Error())}
	}
	return C.struct_GetSheetPropsResult{opts: cVal.Elem().Interface().(C.struct_SheetPropsOptions), err: C.CString(emptyString)}
}

// GetSheetProtection provides a function to get worksheet protection settings
// by given worksheet name. Note that the password in the returned will always
// be empty.
//
//export GetSheetProtection
func GetSheetProtection(idx int, sheet *C.char) C.struct_GetSheetProtectionResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetSheetProtectionResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetSheetProtection(C.GoString(sheet))
	if err != nil {
		return C.struct_GetSheetProtectionResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(opts), reflect.ValueOf(&C.struct_SheetProtectionOptions{}))
	if err != nil {
		return C.struct_GetSheetProtectionResult{err: C.CString(err.Error())}
	}
	return C.struct_GetSheetProtectionResult{opts: cVal.Elem().Interface().(C.struct_SheetProtectionOptions), err: C.CString(emptyString)}
}

// GetSheetView gets the value of sheet view options. The viewIndex may be
// negative and if so is counted backward (-1 is the last view).
//
//export GetSheetView
func GetSheetView(idx int, sheet *C.char, viewIndex int) C.struct_GetSheetViewResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetSheetViewResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetSheetView(C.GoString(sheet), viewIndex)
	if err != nil {
		return C.struct_GetSheetViewResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(opts), reflect.ValueOf(&C.struct_ViewOptions{}))
	if err != nil {
		return C.struct_GetSheetViewResult{err: C.CString(err.Error())}
	}
	return C.struct_GetSheetViewResult{opts: cVal.Elem().Interface().(C.struct_ViewOptions), err: C.CString(emptyString)}
}

// GetSheetVisible provides a function to get worksheet visible by given
// worksheet name.
//
//export GetSheetVisible
func GetSheetVisible(idx int, sheet *C.char) C.struct_BoolErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_BoolErrorResult{val: C._Bool(false), err: C.CString(errFilePtr)}
	}
	visible, err := f.(*excelize.File).GetSheetVisible(C.GoString(sheet))
	if err != nil {
		return C.struct_BoolErrorResult{val: C._Bool(visible), err: C.CString(err.Error())}
	}
	return C.struct_BoolErrorResult{val: C._Bool(visible), err: C.CString(emptyString)}
}

// GetSlicers provides the method to get all slicers in a worksheet by a given
// worksheet name. Note that, this function does not support getting the height,
// width, and graphic options of the slicer shape currently.
//
//export GetSlicers
func GetSlicers(idx int, sheet *C.char) C.struct_GetSlicersResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetSlicersResult{Err: C.CString(errFilePtr)}
	}
	tables, err := f.(*excelize.File).GetSlicers(C.GoString(sheet))
	if err != nil {
		return C.struct_GetSlicersResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(tables)) * C.size_t(unsafe.Sizeof(C.struct_SlicerOptions{})))
	for i, t := range tables {
		cVal, err := goValueToC(reflect.ValueOf(t), reflect.ValueOf(&C.struct_SlicerOptions{}))
		if err != nil {
			return C.struct_GetSlicersResult{Err: C.CString(err.Error())}
		}
		*(*C.struct_SlicerOptions)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_SlicerOptions{}))) = cVal.Elem().Interface().(C.struct_SlicerOptions)
	}
	return C.struct_GetSlicersResult{SlicersLen: C.int(len(tables)), Slicers: (*C.struct_SlicerOptions)(cArray), Err: C.CString(emptyString)}
}

// GetStyle provides a function to get style definition by given style index.
//
//export GetStyle
func GetStyle(idx, styleID int) C.struct_GetStyleResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetStyleResult{err: C.CString(errFilePtr)}
	}
	style, err := f.(*excelize.File).GetStyle(styleID)
	if err != nil {
		return C.struct_GetStyleResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(*style), reflect.ValueOf(&C.struct_Style{}))
	if err != nil {
		return C.struct_GetStyleResult{err: C.CString(err.Error())}
	}
	return C.struct_GetStyleResult{style: cVal.Elem().Interface().(C.struct_Style), err: C.CString(emptyString)}
}

// GetTables provides the method to get all tables in a worksheet by given
// worksheet name.
//
//export GetTables
func GetTables(idx int, sheet *C.char) C.struct_GetTablesResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetTablesResult{Err: C.CString(errFilePtr)}
	}
	tables, err := f.(*excelize.File).GetTables(C.GoString(sheet))
	if err != nil {
		return C.struct_GetTablesResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(tables)) * C.size_t(unsafe.Sizeof(C.struct_Table{})))
	for i, t := range tables {
		cVal, err := goValueToC(reflect.ValueOf(t), reflect.ValueOf(&C.struct_Table{}))
		if err != nil {
			return C.struct_GetTablesResult{Err: C.CString(err.Error())}
		}
		*(*C.struct_Table)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(C.struct_Table{}))) = cVal.Elem().Interface().(C.struct_Table)
	}
	return C.struct_GetTablesResult{TablesLen: C.int(len(tables)), Tables: (*C.struct_Table)(cArray), Err: C.CString(emptyString)}
}

// GetWorkbookProps provides a function to gets workbook properties.
//
//export GetWorkbookProps
func GetWorkbookProps(idx int) C.struct_GetWorkbookPropsResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetWorkbookPropsResult{err: C.CString(errFilePtr)}
	}
	opts, err := f.(*excelize.File).GetWorkbookProps()
	if err != nil {
		return C.struct_GetWorkbookPropsResult{err: C.CString(err.Error())}
	}
	cVal, err := goValueToC(reflect.ValueOf(opts), reflect.ValueOf(&C.struct_WorkbookPropsOptions{}))
	if err != nil {
		return C.struct_GetWorkbookPropsResult{err: C.CString(err.Error())}
	}
	return C.struct_GetWorkbookPropsResult{opts: cVal.Elem().Interface().(C.struct_WorkbookPropsOptions), err: C.CString(emptyString)}
}

// GroupSheets provides a function to group worksheets by given worksheets
// name. Group worksheets must contain an active worksheet.
//
//export GroupSheets
func GroupSheets(idx int, sheets **C.char, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	array := make([]string, length)
	for i, val := range unsafe.Slice(sheets, length) {
		array[i] = C.GoString(val)
	}
	if err := f.(*excelize.File).GroupSheets(array); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// InsertCols provides a function to insert new columns before the given column
// name and number of columns.
//
//export InsertCols
func InsertCols(idx int, sheet, col *C.char, n int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).InsertCols(C.GoString(sheet), C.GoString(col), n); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// InsertPageBreak create a page break to determine where the printed page
// ends and where begins the next one by given worksheet name and cell
// reference, so the content before the page break will be printed on one page
// and after the page break on another.
//
//export InsertPageBreak
func InsertPageBreak(idx int, sheet, cell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).InsertPageBreak(C.GoString(sheet), C.GoString(cell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// InsertRows provides a function to insert new rows after the given Excel row
// number starting from 1 and number of rows.
//
//export InsertRows
func InsertRows(idx int, sheet *C.char, row, n int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).InsertRows(C.GoString(sheet), row, n); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// JoinCellName joins cell name from column name and row number.
//
//export JoinCellName
func JoinCellName(col *C.char, row int) C.struct_StringErrorResult {
	result, err := excelize.JoinCellName(C.GoString(col), row)
	if err != nil {
		return C.struct_StringErrorResult{err: C.CString(err.Error())}
	}
	return C.struct_StringErrorResult{val: C.CString(result), err: C.CString(emptyString)}
}

// MergeCell provides a function to merge cells by given range reference and
// sheet name. Merging cells only keeps the upper-left cell value, and
// discards the other values.
//
//export MergeCell
func MergeCell(idx int, sheet, topLeftCell, bottomRightCell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).MergeCell(C.GoString(sheet), C.GoString(topLeftCell), C.GoString(bottomRightCell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// MoveSheet moves a sheet to a specified position in the workbook. The function
// moves the source sheet before the target sheet. After moving, other sheets
// will be shifted to the left or right. If the sheet is already at the target
// position, the function will not perform any action. Not that this function
// will be ungroup all sheets after moving.
//
//export MoveSheet
func MoveSheet(idx int, source, target *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).MoveSheet(C.GoString(source), C.GoString(target)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// NewConditionalStyle provides a function to create style for conditional
// format by given style format. The parameters are the same with the NewStyle
// function.
//
//export NewConditionalStyle
func NewConditionalStyle(idx int, style *C.struct_Style) C.struct_IntErrorResult {
	var s excelize.Style
	goVal, err := cValueToGo(reflect.ValueOf(*style), reflect.TypeOf(excelize.Style{}))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(err.Error())}
	}
	s = goVal.Elem().Interface().(excelize.Style)
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(errFilePtr)}
	}
	styleID, err := f.(*excelize.File).NewConditionalStyle(&s)
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(styleID), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(styleID), err: C.CString(emptyString)}
}

// NewFile provides a function to create new file by default template.
//
//export NewFile
func NewFile() int {
	f, idx := excelize.NewFile(), 0
	files.Range(func(_, _ interface{}) bool {
		idx++
		return true
	})
	idx++
	files.Store(idx, f)
	return idx
}

// NewSheet provides the function to create a new sheet by given a worksheet
// name and returns the index of the sheets in the workbook after it appended.
// Note that when creating a new workbook, the default worksheet named
// `Sheet1` will be created.
//
//export NewSheet
func NewSheet(idx int, sheet *C.char) C.struct_IntErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(-1), err: C.CString(errFilePtr)}
	}
	idx, err := f.(*excelize.File).NewSheet(C.GoString(sheet))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(idx), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(idx), err: C.CString(emptyString)}
}

// NewStreamWriter returns stream writer struct by given worksheet name used for
// writing data on a new existing empty worksheet with large amounts of data.
// Note that after writing data with the stream writer for the worksheet, you
// must call the 'Flush' method to end the streaming writing process, ensure
// that the order of row numbers is ascending when set rows, and the normal
// mode functions and stream mode functions can not be work mixed to writing
// data on the worksheets. The stream writer will try to use temporary files on
// disk to reduce the memory usage when in-memory chunks data over 16MB, and
// you can't get cell value at this time.
//
//export NewStreamWriter
func NewStreamWriter(idx int, sheet *C.char) C.struct_IntErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(errFilePtr)}
	}
	streamWriter, err := f.(*excelize.File).NewStreamWriter(C.GoString(sheet))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(err.Error())}
	}
	var swIdx int
	sw.Range(func(_, _ interface{}) bool {
		swIdx++
		return true
	})
	swIdx++
	sw.Store(swIdx, streamWriter)
	return C.struct_IntErrorResult{val: C.int(swIdx), err: C.CString(emptyString)}
}

// Rows returns a rows iterator, used for streaming reading data for a worksheet
// with a large data. This function is concurrency safe.
//
//export Rows
func Rows(idx int, sheet *C.char) C.struct_IntErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(errFilePtr)}
	}
	rows, err := f.(*excelize.File).Rows(C.GoString(sheet))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(err.Error())}
	}
	var rIdx int
	rowsIterator.Range(func(_, _ interface{}) bool {
		rIdx++
		return true
	})
	rIdx++
	rowsIterator.Store(rIdx, rows)
	return C.struct_IntErrorResult{val: C.int(rIdx), err: C.CString(emptyString)}
}

// RowsClose closes the open worksheet XML file in the system temporary
// directory.
//
//export RowsClose
func RowsClose(rIdx int) *C.char {
	row, ok := rowsIterator.Load(rIdx)
	if !ok {
		return C.CString(errRowsIteratorPtr)
	}
	defer rowsIterator.Delete(rIdx)
	if err := row.(*excelize.Rows).Close(); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// RowsColumns return the current row's column values. This fetches the
// worksheet data as a stream, returns each cell in a row as is, and will not
// skip empty rows in the tail of the worksheet.
//
//export RowsColumns
func RowsColumns(rIdx int, opts *C.struct_Options) C.struct_StringArrayErrorResult {
	var options excelize.Options
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_StringArrayErrorResult{Err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	row, ok := rowsIterator.Load(rIdx)
	if !ok {
		return C.struct_StringArrayErrorResult{Err: C.CString(errRowsIteratorPtr)}
	}
	result, err := row.(*excelize.Rows).Columns(options)
	if err != nil {
		return C.struct_StringArrayErrorResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(result)) * C.size_t(unsafe.Sizeof(uintptr(0))))
	for i, v := range result {
		*(*unsafe.Pointer)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(uintptr(0)))) = unsafe.Pointer(C.CString(v))
	}
	return C.struct_StringArrayErrorResult{ArrLen: C.int(len(result)), Arr: (**C.char)(cArray), Err: C.CString(emptyString)}
}

// RowsError will return the error when the error occurs.
//
//export RowsError
func RowsError(rIdx int) *C.char {
	row, ok := rowsIterator.Load(rIdx)
	if !ok {
		return C.CString(errRowsIteratorPtr)
	}
	err := row.(*excelize.Rows).Error()
	if err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// RowsGetRowOpts will return the RowOpts of the current row.
//
//export RowsGetRowOpts
func RowsGetRowOpts(rIdx int) C.struct_GetRowOptsResult {
	row, ok := rowsIterator.Load(rIdx)
	if !ok {
		return C.struct_GetRowOptsResult{err: C.CString(errRowsIteratorPtr)}
	}
	opts := row.(*excelize.Rows).GetRowOpts()
	cVal, err := goValueToC(reflect.ValueOf(opts), reflect.ValueOf(&C.struct_RowOpts{}))
	if err != nil {
		return C.struct_GetRowOptsResult{err: C.CString(err.Error())}
	}
	return C.struct_GetRowOptsResult{opts: cVal.Elem().Interface().(C.struct_RowOpts), err: C.CString(emptyString)}
}

// RowsNext will return true if it finds the next row element.
//
//export RowsNext
func RowsNext(rIdx int) C.struct_BoolErrorResult {
	row, ok := rowsIterator.Load(rIdx)
	if !ok {
		return C.struct_BoolErrorResult{val: C._Bool(false), err: C.CString(errRowsIteratorPtr)}
	}
	return C.struct_BoolErrorResult{val: C._Bool(row.(*excelize.Rows).Next()), err: C.CString(emptyString)}
}

// StreamAddTable creates an Excel table for the StreamWriter using the given
// cell range and format set.
//
//export StreamAddTable
func StreamAddTable(swIdx int, table *C.struct_Table) *C.char {
	var tbl excelize.Table
	streamWriter, ok := sw.Load(swIdx)
	if !ok {
		return C.CString(errStreamWriterPtr)
	}
	goVal, err := cValueToGo(reflect.ValueOf(*table), reflect.TypeOf(excelize.Table{}))
	if err != nil {
		return C.CString(err.Error())
	}
	tbl = goVal.Elem().Interface().(excelize.Table)
	if err := streamWriter.(*excelize.StreamWriter).AddTable(&tbl); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// StreamInsertPageBreak creates a page break to determine where the printed
// page ends and where begins the next one by a given cell reference, the
// content before the page break will be printed on one page and after the page
// break on another.
//
//export StreamInsertPageBreak
func StreamInsertPageBreak(swIdx int, cell *C.char) *C.char {
	streamWriter, ok := sw.Load(swIdx)
	if !ok {
		return C.CString(errStreamWriterPtr)
	}
	if err := streamWriter.(*excelize.StreamWriter).InsertPageBreak(C.GoString(cell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// StreamMergeCell provides a function to merge cells by a given range reference
// for the StreamWriter. Don't create a merged cell that overlaps with another
// existing merged cell.
//
//export StreamMergeCell
func StreamMergeCell(swIdx int, topLeftCell, bottomRightCell *C.char) *C.char {
	streamWriter, ok := sw.Load(swIdx)
	if !ok {
		return C.CString(errStreamWriterPtr)
	}
	if err := streamWriter.(*excelize.StreamWriter).MergeCell(C.GoString(topLeftCell), C.GoString(bottomRightCell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// StreamSetColWidth provides a function to set the width of a single column or
// multiple columns for the StreamWriter. Note that you must call
// the 'StreamSetColWidth' function before the 'StreamSetRow' function.
//
//export StreamSetColWidth
func StreamSetColWidth(swIdx int, minVal, maxVal int, width float64) *C.char {
	streamWriter, ok := sw.Load(swIdx)
	if !ok {
		return C.CString(errStreamWriterPtr)
	}
	if err := streamWriter.(*excelize.StreamWriter).SetColWidth(minVal, maxVal, width); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// StreamSetPanes provides a function to create and remove freeze panes and
// split panes by giving panes options for the StreamWriter. Note that you must
// call the 'StreamSetPanes' function before the 'StreamSetRow' function.
//
//export StreamSetPanes
func StreamSetPanes(swIdx int, opts *C.struct_Panes) *C.char {
	var options excelize.Panes
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Panes{}))
	if err != nil {
		return C.CString(err.Error())
	}
	streamWriter, ok := sw.Load(swIdx)
	if !ok {
		return C.CString(errStreamWriterPtr)
	}
	options = goVal.Elem().Interface().(excelize.Panes)
	if err := streamWriter.(*excelize.StreamWriter).SetPanes(&options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// StreamSetRow writes an array to stream rows by giving starting cell reference
// and a pointer to an array of values. Note that you must call the 'StreamFlush'
// function to end the streaming writing process.
//
//export StreamSetRow
func StreamSetRow(swIdx int, cell *C.char, row *C.struct_Interface, length int) *C.char {
	streamWriter, ok := sw.Load(swIdx)
	if !ok {
		return C.CString(errStreamWriterPtr)
	}
	cells := make([]interface{}, length)
	for i, val := range unsafe.Slice(row, length) {
		cells[i] = cInterfaceToGo(val)
	}
	if err := streamWriter.(*excelize.StreamWriter).SetRow(C.GoString(cell), cells); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// StreamFlush ending the streaming writing process.
//
//export StreamFlush
func StreamFlush(swIdx int) *C.char {
	streamWriter, ok := sw.Load(swIdx)
	if !ok {
		return C.CString(errStreamWriterPtr)
	}
	defer sw.Delete(swIdx)
	if err := streamWriter.(*excelize.StreamWriter).Flush(); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// NewStyle provides a function to create the style for cells by given options.
// Note that the color field uses RGB color code.
//
//export NewStyle
func NewStyle(idx int, style *C.struct_Style) C.struct_IntErrorResult {
	var s excelize.Style
	goVal, err := cValueToGo(reflect.ValueOf(*style), reflect.TypeOf(excelize.Style{}))
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(err.Error())}
	}
	s = goVal.Elem().Interface().(excelize.Style)
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_IntErrorResult{val: C.int(0), err: C.CString(errFilePtr)}
	}
	styleID, err := f.(*excelize.File).NewStyle(&s)
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(styleID), err: C.CString(err.Error())}
	}
	return C.struct_IntErrorResult{val: C.int(styleID), err: C.CString(emptyString)}
}

// OpenFile take the name of a spreadsheet file and returns a populated
// spreadsheet file struct for it.
//
//export OpenFile
func OpenFile(filename *C.char, opts *C.struct_Options) C.struct_IntErrorResult {
	var options excelize.Options
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_IntErrorResult{val: C.int(-1), err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	f, err := excelize.OpenFile(C.GoString(filename), options)
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(-1), err: C.CString(err.Error())}
	}
	var idx int
	files.Range(func(_, _ interface{}) bool {
		idx++
		return true
	})
	idx++
	files.Store(idx, f)
	return C.struct_IntErrorResult{val: C.int(idx), err: C.CString(emptyString)}
}

// OpenReader read data stream from io.Reader and return a populated spreadsheet
// file.
//
//export OpenReader
func OpenReader(b *C.uchar, bLen C.int, opts *C.struct_Options) C.struct_IntErrorResult {
	var options excelize.Options
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_IntErrorResult{val: C.int(-1), err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	buf := C.GoBytes(unsafe.Pointer(b), bLen)
	f, err := excelize.OpenReader(bytes.NewReader(buf), options)
	if err != nil {
		return C.struct_IntErrorResult{val: C.int(-1), err: C.CString(err.Error())}
	}
	var idx int
	files.Range(func(_, _ interface{}) bool {
		idx++
		return true
	})
	idx++
	files.Store(idx, f)
	return C.struct_IntErrorResult{val: C.int(idx), err: C.CString(emptyString)}
}

// ProtectSheet provides a function to prevent other users from accidentally or
// deliberately changing, moving, or deleting data in a worksheet. The
// optional field AlgorithmName specified hash algorithm, support XOR, MD4,
// MD5, SHA-1, SHA2-56, SHA-384, and SHA-512 currently, if no hash algorithm
// specified, will be using the XOR algorithm as default.
//
//export ProtectSheet
func ProtectSheet(idx int, sheet *C.char, opts *C.struct_SheetProtectionOptions) *C.char {
	var options excelize.SheetProtectionOptions
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.SheetProtectionOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.SheetProtectionOptions)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).ProtectSheet(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// ProtectWorkbook provides a function to prevent other users from viewing
// hidden worksheets, adding, moving, deleting, or hiding worksheets, and
// renaming worksheets in a workbook. The optional field AlgorithmName
// specified hash algorithm, support XOR, MD4, MD5, SHA-1, SHA2-56, SHA-384,
// and SHA-512 currently, if no hash algorithm specified, will be using the XOR
// algorithm as default. The generated workbook only works on Microsoft Office
// 2007 and later.
//
//export ProtectWorkbook
func ProtectWorkbook(idx int, opts *C.struct_WorkbookProtectionOptions) *C.char {
	var options excelize.WorkbookProtectionOptions
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.WorkbookProtectionOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.WorkbookProtectionOptions)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).ProtectWorkbook(&options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// RemoveCol provides a function to remove single column by given worksheet
// name and column index.
//
//export RemoveCol
func RemoveCol(idx int, sheet, col *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).RemoveCol(C.GoString(sheet), C.GoString(col)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// RemovePageBreak remove a page break by given worksheet name and cell
// reference.
//
//export RemovePageBreak
func RemovePageBreak(idx int, sheet, cell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).RemovePageBreak(C.GoString(sheet), C.GoString(cell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// RemoveRow provides a function to remove single row by given worksheet name
// and Excel row number.
//
//export RemoveRow
func RemoveRow(idx int, sheet *C.char, row int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).RemoveRow(C.GoString(sheet), row); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// Save provides a function to override the spreadsheet with origin path.
//
//export Save
func Save(idx int, opts *C.struct_Options) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if opts != nil {
		var options excelize.Options
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.CString(err.Error())
		}
		options = goVal.Elem().Interface().(excelize.Options)
		if err := f.(*excelize.File).Save(options); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).Save(); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SaveAs provides a function to create or update to a spreadsheet at the
// provided path.
//
//export SaveAs
func SaveAs(idx int, name *C.char, opts *C.struct_Options) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if opts != nil {
		var options excelize.Options
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.CString(err.Error())
		}
		options = goVal.Elem().Interface().(excelize.Options)
		if err := f.(*excelize.File).SaveAs(C.GoString(name), options); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).SaveAs(C.GoString(name)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SearchSheet provides a function to get cell reference by given worksheet name,
// cell value, and regular expression. The function doesn't support searching
// on the calculated result, formatted numbers and conditional lookup
// currently. If it is a merged cell, it will return the cell reference of the
// upper left cell of the merged range reference.
//
//export SearchSheet
func SearchSheet(idx int, sheet, value *C.char, reg bool) C.struct_StringArrayErrorResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_StringArrayErrorResult{Err: C.CString(errFilePtr)}
	}
	result, err := f.(*excelize.File).SearchSheet(C.GoString(sheet), C.GoString(value), reg)
	if err != nil {
		return C.struct_StringArrayErrorResult{Err: C.CString(err.Error())}
	}
	cArray := C.malloc(C.size_t(len(result)) * C.size_t(unsafe.Sizeof(uintptr(0))))
	for i, v := range result {
		*(*unsafe.Pointer)(unsafe.Pointer(uintptr(unsafe.Pointer(cArray)) + uintptr(i)*unsafe.Sizeof(uintptr(0)))) = unsafe.Pointer(C.CString(v))
	}
	return C.struct_StringArrayErrorResult{ArrLen: C.int(len(result)), Arr: (**C.char)(cArray), Err: C.CString(emptyString)}
}

// SetActiveSheet provides a function to set the default active sheet of the
// workbook by a given index. Note that the active index is different from the
// ID returned by function GetSheetMap(). It should be greater than or equal
// to 0 and less than the total worksheet numbers.
//
//export SetActiveSheet
func SetActiveSheet(idx, index int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	f.(*excelize.File).SetActiveSheet(index)
	return C.CString(emptyString)
}

// SetAppProps provides a function to set document application properties.
//
//export SetAppProps
func SetAppProps(idx int, opts *C.struct_AppProperties) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.AppProperties{}))
	if err != nil {
		return C.CString(err.Error())
	}
	appProps := goVal.Elem().Interface().(excelize.AppProperties)
	if err := f.(*excelize.File).SetAppProps(&appProps); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCalcProps provides a function to sets calculation properties. Optional
// value of "CalcMode" property is: "manual", "auto" or "autoNoTable". Optional
// value of "RefMode" property is: "A1" or "R1C1".
//
//export SetCalcProps
func SetCalcProps(idx int, opts *C.struct_CalcPropsOptions) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.CalcPropsOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	props := goVal.Elem().Interface().(excelize.CalcPropsOptions)
	if err := f.(*excelize.File).SetCalcProps(&props); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellBool provides a function to set bool type value of a cell by given
// worksheet name, cell reference and cell value.
//
//export SetCellBool
func SetCellBool(idx int, sheet, cell *C.char, value bool) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCellBool(C.GoString(sheet), C.GoString(cell), value); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellDefault provides a function to set string type value of a cell as
// default format without escaping the cell.
//
//export SetCellDefault
func SetCellDefault(idx int, sheet, cell, value *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCellDefault(C.GoString(sheet), C.GoString(cell), C.GoString(value)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellFloat sets a floating point value into a cell. The precision
// parameter specifies how many places after the decimal will be shown
// while -1 is a special value that will use as many decimal places as
// necessary to represent the number. bitSize is 32 or 64 depending on if a
// float32 or float64 was originally used for the value.
//
//export SetCellFloat
func SetCellFloat(idx int, sheet, cell *C.char, value float64, precision, bitSize int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCellFloat(C.GoString(sheet), C.GoString(cell), value, precision, bitSize); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellFormula provides a function to set formula on the cell is taken
// according to the given worksheet name and cell formula settings. The result
// of the formula cell can be calculated when the worksheet is opened by the
// Office Excel application or can be using the "CalcCellValue" function also
// can get the calculated cell value. If the Excel application doesn't
// calculate the formula automatically when the workbook has been opened,
// please call "UpdateLinkedValue" after setting the cell formula functions.
//
//export SetCellFormula
func SetCellFormula(idx int, sheet, cell, formula *C.char, opts *C.struct_FormulaOpts) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if opts != nil {
		var options excelize.FormulaOpts
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.FormulaOpts{}))
		if err != nil {
			return C.CString(err.Error())
		}
		options = goVal.Elem().Interface().(excelize.FormulaOpts)
		if err := f.(*excelize.File).SetCellFormula(C.GoString(sheet), C.GoString(cell), C.GoString(formula), options); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).SetCellFormula(C.GoString(sheet), C.GoString(cell), C.GoString(formula)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellHyperLink provides a function to set cell hyperlink by given
// worksheet name and link URL address. LinkType defines three types of
// hyperlink "External" for website or "Location" for moving to one of cell in
// this workbook or "None" for remove hyperlink. Maximum limit hyperlinks in a
// worksheet is 65530. This function is only used to set the hyperlink of the
// cell and doesn't affect the value of the cell. If you need to set the value
// of the cell, please use the other functions such as `SetCellStyle` or
// `SetSheetRow`.
//
//export SetCellHyperLink
func SetCellHyperLink(idx int, sheet, cell, link, linkType *C.char, opts *C.struct_HyperlinkOpts) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if opts != nil {
		var options excelize.HyperlinkOpts
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.HyperlinkOpts{}))
		if err != nil {
			return C.CString(err.Error())
		}
		options = goVal.Elem().Interface().(excelize.HyperlinkOpts)
		if err := f.(*excelize.File).SetCellHyperLink(C.GoString(sheet), C.GoString(cell), C.GoString(link), C.GoString(linkType), options); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).SetCellHyperLink(C.GoString(sheet), C.GoString(cell), C.GoString(link), C.GoString(linkType)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellInt provides a function to set int type value of a cell by given
// worksheet name, cell reference and cell value.
//
//export SetCellInt
func SetCellInt(idx int, sheet, cell *C.char, value int64) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCellInt(C.GoString(sheet), C.GoString(cell), value); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellRichText provides a function to set cell with rich text by given
// worksheet name, cell reference and rich text runs.
//
//export SetCellRichText
func SetCellRichText(idx int, sheet, cell *C.char, runs *C.struct_RichTextRun, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	textRuns := make([]excelize.RichTextRun, length)
	for i, val := range unsafe.Slice(runs, length) {
		goVal, err := cValueToGo(reflect.ValueOf(val), reflect.TypeOf(excelize.RichTextRun{}))
		if err != nil {
			return C.CString(err.Error())
		}
		textRuns[i] = goVal.Elem().Interface().(excelize.RichTextRun)
	}
	if err := f.(*excelize.File).SetCellRichText(C.GoString(sheet), C.GoString(cell), textRuns); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellStr provides a function to set string type value of a cell. Total
// number of characters that a cell can contain 32767 characters.
//
//export SetCellStr
func SetCellStr(idx int, sheet, cell, value *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCellStr(C.GoString(sheet), C.GoString(cell), C.GoString(value)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellStyle provides a function to add style attribute for cells by given
// worksheet name, range reference and style ID. This function is concurrency
// safe. Note that diagonalDown and diagonalUp type border should be use same
// color in the same range. SetCellStyle will overwrite the existing
// styles for the cell, it won't append or merge style with existing styles.
//
//export SetCellStyle
func SetCellStyle(idx int, sheet, topLeftCell, bottomRightCell *C.char, styleID int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCellStyle(C.GoString(sheet), C.GoString(topLeftCell), C.GoString(bottomRightCell), styleID); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCellValue provides a function to set the value of a cell. The specified
// coordinates should not be in the first row of the table, a complex number
// can be set with string text.
//
// You can set numbers format by the SetCellStyle function. If you need to set
// the specialized date in Excel like January 0, 1900 or February 29, 1900.
// Please set the cell value as number 0 or 60, then create and bind the
// date-time number format style for the cell.
//
//export SetCellValue
func SetCellValue(idx int, sheet, cell *C.char, value *C.struct_Interface) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCellValue(C.GoString(sheet), C.GoString(cell), cInterfaceToGo(*value)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetColOutlineLevel provides a function to set outline level of a single
// column by given worksheet name and column name. The value of parameter
// 'level' is 1-7.
//
//export SetColOutlineLevel
func SetColOutlineLevel(idx int, sheet, col *C.char, level int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetColOutlineLevel(C.GoString(sheet), C.GoString(col), uint8(level)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetColStyle provides a function to set style of columns by given worksheet
// name, columns range and style ID. This function is concurrency safe. Note
// that this will overwrite the existing styles for the columns, it won't
// append or merge style with existing styles.
//
//export SetColStyle
func SetColStyle(idx int, sheet, columns *C.char, styleID int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetColStyle(C.GoString(sheet), C.GoString(columns), styleID); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetColVisible provides a function to set visible columns by given worksheet
// name, columns range and visibility. This function is concurrency safe.
//
//export SetColVisible
func SetColVisible(idx int, sheet, columns *C.char, visible bool) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetColVisible(C.GoString(sheet), C.GoString(columns), visible); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetColWidth provides a function to set the width of a single column or
// multiple columns. This function is concurrency safe.
//
//export SetColWidth
func SetColWidth(idx int, sheet, startCol, endCol *C.char, width float64) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetColWidth(C.GoString(sheet), C.GoString(startCol), C.GoString(endCol), width); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetConditionalFormat provides a function to create conditional formatting
// rule for cell value. Conditional formatting is a feature of Excel which
// allows you to apply a format to a cell or a range of cells based on certain
// criteria.
//
//export SetConditionalFormat
func SetConditionalFormat(idx int, sheet, rangeRef *C.char, opts *C.struct_ConditionalFormatOptions, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	options := make([]excelize.ConditionalFormatOptions, length)
	for i, val := range unsafe.Slice(opts, length) {
		goVal, err := cValueToGo(reflect.ValueOf(val), reflect.TypeOf(excelize.ConditionalFormatOptions{}))
		if err != nil {
			return C.CString(err.Error())
		}
		options[i] = goVal.Elem().Interface().(excelize.ConditionalFormatOptions)
	}
	if err := f.(*excelize.File).SetConditionalFormat(C.GoString(sheet), C.GoString(rangeRef), options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetCustomProps provides a function to set custom file properties by given
// property name and value. If the property name already exists, it will be
// updated, otherwise a new property will be added. The value can be of type
// int32, float64, bool, string, time.Time or nil. The property will be delete
// if the value is nil. The function returns an error if the property value is
// not of the correct type.
//
//export SetCustomProps
func SetCustomProps(idx int, prop C.struct_CustomProperty) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCustomProps(excelize.CustomProperty{
		Name: C.GoString(prop.Name), Value: cInterfaceToGo(prop.Value),
	}); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetDefaultFont provides the default font name currently set in the
// workbook. The spreadsheet generated by excelize default font is Calibri.
//
//export SetDefaultFont
func SetDefaultFont(idx int, fontName *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetDefaultFont(C.GoString(fontName)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetDefinedName provides a function to set the defined names of the workbook
// or worksheet. If not specified scope, the default scope is workbook.
//
//export SetDefinedName
func SetDefinedName(idx int, definedName *C.struct_DefinedName) *C.char {
	var df excelize.DefinedName
	goVal, err := cValueToGo(reflect.ValueOf(*definedName), reflect.TypeOf(excelize.DefinedName{}))
	if err != nil {
		return C.CString(err.Error())
	}
	df = goVal.Elem().Interface().(excelize.DefinedName)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetDefinedName(&df); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetDocProps provides a function to set document core properties.
//
//export SetDocProps
func SetDocProps(idx int, docProperties *C.struct_DocProperties) *C.char {
	var options excelize.DocProperties
	goVal, err := cValueToGo(reflect.ValueOf(*docProperties), reflect.TypeOf(excelize.DocProperties{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.DocProperties)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetDocProps(&options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetHeaderFooter provides a function to set headers and footers by given
// worksheet name and the control characters.
//
//export SetHeaderFooter
func SetHeaderFooter(idx int, sheet *C.char, opts *C.struct_HeaderFooterOptions) *C.char {
	var options excelize.HeaderFooterOptions
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.HeaderFooterOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.HeaderFooterOptions)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetHeaderFooter(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetPageLayout provides a function to sets worksheet page layout.
//
//export SetPageLayout
func SetPageLayout(idx int, sheet *C.char, opts *C.struct_PageLayoutOptions) *C.char {
	var options excelize.PageLayoutOptions
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.PageLayoutOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.PageLayoutOptions)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetPageLayout(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetPageMargins provides a function to set worksheet page margins.
//
//export SetPageMargins
func SetPageMargins(idx int, sheet *C.char, opts *C.struct_PageLayoutMarginsOptions) *C.char {
	var options excelize.PageLayoutMarginsOptions
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.PageLayoutMarginsOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.PageLayoutMarginsOptions)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetPageMargins(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetPanes provides a function to create and remove freeze panes and split panes
// by given worksheet name and panes options.
//
//export SetPanes
func SetPanes(idx int, sheet *C.char, opts *C.struct_Panes) *C.char {
	var options excelize.Panes
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Panes{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options = goVal.Elem().Interface().(excelize.Panes)
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetPanes(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetRowHeight provides a function to set the height of a single row. If the
// value of height is 0, will hide the specified row, if the value of height is
// -1, will unset the custom row height.
//
//export SetRowHeight
func SetRowHeight(idx int, sheet *C.char, row int, height float64) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetRowHeight(C.GoString(sheet), row, height); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetRowOutlineLevel provides a function to set outline level number of a
// single row by given worksheet name and Excel row number. The value of
// parameter 'level' is 1-7.
//
//export SetRowOutlineLevel
func SetRowOutlineLevel(idx int, sheet *C.char, row, level int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetRowOutlineLevel(C.GoString(sheet), row, uint8(level)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetRowStyle provides a function to set the style of rows by given worksheet
// name, row range, and style ID. Note that this will overwrite the existing
// styles for the rows, it won't append or merge style with existing styles.
//
//export SetRowStyle
func SetRowStyle(idx int, sheet *C.char, start, end, styleID int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetRowStyle(C.GoString(sheet), start, end, styleID); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetRowVisible provides a function to set visible of a single row by given
// worksheet name and Excel row number.
//
//export SetRowVisible
func SetRowVisible(idx int, sheet *C.char, row int, visible bool) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetRowVisible(C.GoString(sheet), row, visible); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetBackground provides a function to set background picture by given
// worksheet name and file path. Supported image types: BMP, EMF, EMZ, GIF,
// ICO, JPEG, JPG, PNG, SVG, TIF, TIFF, WMF and WMZ.
//
//export SetSheetBackground
func SetSheetBackground(idx int, sheet, picture *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetSheetBackground(C.GoString(sheet), C.GoString(picture)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetBackgroundFromBytes provides a function to set background picture by
// given worksheet name, extension name and image data. Supported image types:
// BMP, EMF, EMZ, GIF, ICO, JPEG, JPG, PNG, SVG, TIF, TIFF, WMF and WMZ.
//
//export SetSheetBackgroundFromBytes
func SetSheetBackgroundFromBytes(idx int, sheet, extension *C.char, picture *C.uchar, pictureLen C.int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	buf := C.GoBytes(unsafe.Pointer(picture), pictureLen)
	if err := f.(*excelize.File).SetSheetBackgroundFromBytes(C.GoString(sheet), C.GoString(extension), buf); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetCol writes an array to column by given worksheet name, starting
// cell reference and a pointer to array type 'slice'.
//
//export SetSheetCol
func SetSheetCol(idx int, sheet, cell *C.char, slice *C.struct_Interface, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	cells := make([]interface{}, length)
	for i, val := range unsafe.Slice(slice, length) {
		cells[i] = cInterfaceToGo(val)
	}
	if err := f.(*excelize.File).SetSheetCol(C.GoString(sheet), C.GoString(cell), &cells); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetDimension provides the method to set or remove the used range of the
// worksheet by a given range reference. It specifies the row and column bounds
// of used cells in the worksheet. The range reference is set using the A1
// reference style(e.g., "A1:D5"). Passing an empty range reference will remove
// the used range of the worksheet.
//
//export SetSheetDimension
func SetSheetDimension(idx int, sheet, rangeRef *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetSheetDimension(C.GoString(sheet), C.GoString(rangeRef)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetName provides a function to set the worksheet name by given source and
// target worksheet names. Maximum 31 characters are allowed in sheet title and
// this function only changes the name of the sheet and will not update the
// sheet name in the formula or reference associated with the cell. So there
// may be problem formula error or reference missing.
//
//export SetSheetName
func SetSheetName(idx int, source, target *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetSheetName(C.GoString(source), C.GoString(target)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetProps provides a function to set worksheet properties.
//
//export SetSheetProps
func SetSheetProps(idx int, sheet *C.char, opts *C.struct_SheetPropsOptions) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.SheetPropsOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options := goVal.Elem().Interface().(excelize.SheetPropsOptions)
	if err := f.(*excelize.File).SetSheetProps(C.GoString(sheet), &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetRow writes an array to row by given worksheet name, starting
// cell reference and a pointer to array type 'slice'. This function is
// concurrency safe.
//
//export SetSheetRow
func SetSheetRow(idx int, sheet, cell *C.char, row *C.struct_Interface, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	cells := make([]interface{}, length)
	for i, val := range unsafe.Slice(row, length) {
		cells[i] = cInterfaceToGo(val)
	}
	if err := f.(*excelize.File).SetSheetRow(C.GoString(sheet), C.GoString(cell), &cells); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetView sets sheet view options. The viewIndex may be negative and if
// so is counted backward (-1 is the last view).
//
//export SetSheetView
func SetSheetView(idx int, sheet *C.char, viewIndex int, opts *C.struct_ViewOptions) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.ViewOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options := goVal.Elem().Interface().(excelize.ViewOptions)
	if err := f.(*excelize.File).SetSheetView(C.GoString(sheet), viewIndex, &options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetSheetVisible provides a function to set worksheet visible by given
// worksheet name. A workbook must contain at least one visible worksheet. If
// the given worksheet has been activated, this setting will be invalidated.
// The third optional veryHidden parameter only works when visible was false.
//
//export SetSheetVisible
func SetSheetVisible(idx int, sheet *C.char, visible, veryHidden bool) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetSheetVisible(C.GoString(sheet), visible, veryHidden); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SetWorkbookProps provides a function to sets workbook properties.
//
//export SetWorkbookProps
func SetWorkbookProps(idx int, opts *C.struct_WorkbookPropsOptions) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.WorkbookPropsOptions{}))
	if err != nil {
		return C.CString(err.Error())
	}
	options := goVal.Elem().Interface().(excelize.WorkbookPropsOptions)
	if err := f.(*excelize.File).SetWorkbookProps(&options); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// SplitCellName splits cell name to column name and row number.
//
//export SplitCellName
func SplitCellName(cell *C.char, row int) C.struct_StringIntErrorResult {
	col, row, err := excelize.SplitCellName(C.GoString(cell))
	if err != nil {
		return C.struct_StringIntErrorResult{err: C.CString(err.Error())}
	}
	return C.struct_StringIntErrorResult{intVal: C.int(row), strVal: C.CString(col), err: C.CString(emptyString)}
}

// UngroupSheets provides a function to ungroup worksheets.
//
//export UngroupSheets
func UngroupSheets(idx int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).UngroupSheets(); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// UnmergeCell provides a function to unmerge a given range reference.
//
//export UnmergeCell
func UnmergeCell(idx int, sheet, topLeftCell, bottomRightCell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).UnmergeCell(C.GoString(sheet), C.GoString(topLeftCell), C.GoString(bottomRightCell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// UnprotectSheet provides a function to remove protection for a sheet,
// specified the second optional password parameter to remove sheet
// protection with password verification.
//
//export UnprotectSheet
func UnprotectSheet(idx int, sheet, password *C.char, verify bool) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if verify {
		if err := f.(*excelize.File).UnprotectSheet(C.GoString(sheet), C.GoString(password)); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).UnprotectSheet(C.GoString(sheet)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// UnprotectWorkbook provides a function to remove protection for workbook,
// specified the optional password parameter to remove workbook protection with
// password verification.
//
//export UnprotectWorkbook
func UnprotectWorkbook(idx int, password *C.char, verify bool) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if verify {
		if err := f.(*excelize.File).UnprotectWorkbook(C.GoString(password)); err != nil {
			return C.CString(err.Error())
		}
		return C.CString(emptyString)
	}
	if err := f.(*excelize.File).UnprotectWorkbook(); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// UnsetConditionalFormat provides a function to unset the conditional format
// by given worksheet name and range reference.
//
//export UnsetConditionalFormat
func UnsetConditionalFormat(idx int, sheet, rangeRef *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).UnsetConditionalFormat(C.GoString(sheet), C.GoString(rangeRef)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

// UpdateLinkedValue fix linked values within a spreadsheet are not updating in
// Office Excel application. This function will be remove value tag when met a
// cell have a linked value.
//
//export UpdateLinkedValue
func UpdateLinkedValue(idx int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).UpdateLinkedValue(); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(emptyString)
}

func main() {
}
