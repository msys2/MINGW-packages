// Copyright 2024 The excelize Authors. All rights reserved. Use of this source
// code is governed by a BSD-style license that can be found in the LICENSE file.
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
#include <types_c.h>
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

const (
	Nil     C.int = 0
	Int     C.int = 1
	String  C.int = 2
	Float   C.int = 3
	Boolean C.int = 4
	Time    C.int = 5
)

var (
	files      = sync.Map{}
	errNil     string
	errFilePtr = "can not find file pointer"
	errArgType = errors.New("invalid argument data type")

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
			return reflect.ValueOf(C.uchar(int8(goVal.Uint()))), nil
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
		return C.CString(errNil)
	}
	if err := f.(*excelize.File).AddChart(C.GoString(sheet), C.GoString(cell), charts[0]); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
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
		return C.CString(errNil)
	}
	if err := f.(*excelize.File).AddChartSheet(C.GoString(sheet), charts[0]); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
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
	return C.CString(errNil)
}

// AddFormControl provides the method to add form control button in a worksheet
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
	return C.CString(errNil)
}

// Add picture in a sheet by given picture format set (such as offset, scale,
// aspect ratio setting and print settings) and file path, supported image
// types: BMP, EMF, EMZ, GIF, JPEG, JPG, PNG, SVG, TIF, TIFF, WMF, and WMZ.
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
		return C.CString(errNil)
	}
	if err := f.(*excelize.File).AddPicture(C.GoString(sheet), C.GoString(cell), C.GoString(name), nil); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
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
		C.CString(err.Error())
	}
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
		C.CString(err.Error())
	}
	return C.CString(errNil)
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
	return C.CString(errNil)
}

// CalcCellValue provides a function to get calculated cell value. This feature
// is currently in working processing. Iterative calculation, implicit
// intersection, explicit intersection, array formula, table formula and some
// other formulas are not supported currently.
//
//export CalcCellValue
func CalcCellValue(idx int, sheet, cell *C.char, opts *C.struct_Options) C.struct_CalcCellValueResult {
	var options excelize.Options
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_CalcCellValueResult{val: C.CString(""), err: C.CString(errFilePtr)}
	}
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_CalcCellValueResult{val: C.CString(""), err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	val, err := f.(*excelize.File).CalcCellValue(C.GoString(sheet), C.GoString(cell), options)
	if err != nil {
		return C.struct_CalcCellValueResult{val: C.CString(val), err: C.CString(err.Error())}
	}
	return C.struct_CalcCellValueResult{val: C.CString(val), err: C.CString(errNil)}
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
	return C.struct_CellNameToCoordinatesResult{col: C.int(col), row: C.int(row), err: C.CString(errNil)}
}

// ColumnNameToNumber provides a function to convert Excel sheet column name
// (case-insensitive) to int. The function returns an error if column name
// incorrect.
//
//export ColumnNameToNumber
func ColumnNameToNumber(name *C.char) C.struct_CellNameToCoordinatesResult {
	col, err := excelize.ColumnNameToNumber(C.GoString(name))
	if err != nil {
		return C.struct_CellNameToCoordinatesResult{col: C.int(col), err: C.CString(err.Error())}
	}
	return C.struct_CellNameToCoordinatesResult{col: C.int(col), err: C.CString(errNil)}
}

// ColumnNumberToName provides a function to convert the integer to Excel
// sheet column title.
//
//export ColumnNumberToName
func ColumnNumberToName(num int) C.struct_ColumnNumberToNameResult {
	col, err := excelize.ColumnNumberToName(num)
	if err != nil {
		return C.struct_ColumnNumberToNameResult{col: C.CString(col), err: C.CString(err.Error())}
	}
	return C.struct_ColumnNumberToNameResult{col: C.CString(col), err: C.CString(errNil)}
}

// CoordinatesToCellName converts [X, Y] coordinates to alpha-numeric cell name
// or returns an error.
//
//export CoordinatesToCellName
func CoordinatesToCellName(col, row int, abs bool) C.struct_CoordinatesToCellNameResult {
	cell, err := excelize.CoordinatesToCellName(col, row, abs)
	if err != nil {
		return C.struct_CoordinatesToCellNameResult{cell: C.CString(cell), err: C.CString(err.Error())}
	}
	return C.struct_CoordinatesToCellNameResult{cell: C.CString(cell), err: C.CString(errNil)}
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.struct_GetAppPropsResult{opts: cVal.Elem().Interface().(C.struct_AppProperties), err: C.CString(errNil)}
}

// GetCellFormula provides a function to get formula from cell by given
// worksheet name and cell reference in spreadsheet.
//
//export GetCellFormula
func GetCellFormula(idx int, sheet, cell *C.char) C.struct_GetCellFormulaResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetCellFormulaResult{val: C.CString(""), err: C.CString(errFilePtr)}
	}
	formula, err := f.(*excelize.File).GetCellFormula(C.GoString(sheet), C.GoString(cell))
	if err != nil {
		return C.struct_GetCellFormulaResult{val: C.CString(formula), err: C.CString(err.Error())}
	}
	return C.struct_GetCellFormulaResult{val: C.CString(formula), err: C.CString(errNil)}
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
		return C.struct_GetCellHyperLinkResult{link: false, target: C.CString(""), err: C.CString(errFilePtr)}
	}
	link, target, err := f.(*excelize.File).GetCellHyperLink(C.GoString(sheet), C.GoString(cell))
	if err != nil {
		return C.struct_GetCellHyperLinkResult{link: C._Bool(link), target: C.CString(target), err: C.CString(err.Error())}
	}
	return C.struct_GetCellHyperLinkResult{link: C._Bool(link), target: C.CString(target), err: C.CString(errNil)}
}

// GetCellValue provides a function to get formatted value from cell by given
// worksheet name and cell reference in spreadsheet. The return value is
// converted to the `string` data type. If the cell format can be applied to
// the value of a cell, the applied value will be returned, otherwise the
// original value will be returned. All cells' values will be the same in a
// merged range.
//
//export GetCellValue
func GetCellValue(idx int, sheet, cell *C.char, opts *C.struct_Options) C.struct_GetCellValueResult {
	var options excelize.Options
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetCellValueResult{val: C.CString(""), err: C.CString(errFilePtr)}
	}
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_GetCellValueResult{val: C.CString(""), err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	val, err := f.(*excelize.File).GetCellValue(C.GoString(sheet), C.GoString(cell), options)
	if err != nil {
		return C.struct_GetCellValueResult{val: C.CString(val), err: C.CString(err.Error())}
	}
	return C.struct_GetCellValueResult{val: C.CString(val), err: C.CString(errNil)}
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
func GetRows(idx int, sheet *C.char, opts *C.struct_Options) C.struct_GetRowsResult {
	type Row struct {
		Cell []string
	}
	type GetRowsResult struct {
		Row []Row
	}
	var (
		options excelize.Options
		result  GetRowsResult
	)
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_GetRowsResult{err: C.CString(errFilePtr)}
	}
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_GetRowsResult{err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	rows, err := f.(*excelize.File).GetRows(C.GoString(sheet), options)
	if err != nil {
		return C.struct_GetRowsResult{err: C.CString(err.Error())}
	}
	for _, row := range rows {
		var r Row
		r.Cell = append(r.Cell, row...)
		result.Row = append(result.Row, r)
	}
	cVal, err := goValueToC(reflect.ValueOf(result), reflect.ValueOf(&C.struct_GetRowsResult{}))
	if err != nil {
		return C.struct_GetRowsResult{err: C.CString(err.Error())}
	}
	ret := cVal.Elem().Interface().(C.struct_GetRowsResult)
	ret.err = C.CString(errNil)
	return ret
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
	return C.struct_GetStyleResult{style: cVal.Elem().Interface().(C.struct_Style), err: C.CString(errNil)}
}

// MergeCell provides a function to merge cells by given range reference and
// sheet name. Merging cells only keeps the upper-left cell value, and
// discards the other values.
//
//export MergeCell
func MergeCell(idx int, sheet, topLeftCell, bottomRightCell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString("")
	}
	if err := f.(*excelize.File).MergeCell(C.GoString(sheet), C.GoString(topLeftCell), C.GoString(bottomRightCell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
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
		return C.CString("")
	}
	if err := f.(*excelize.File).MoveSheet(C.GoString(source), C.GoString(target)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
}

// NewConditionalStyle provides a function to create style for conditional
// format by given style format. The parameters are the same with the NewStyle
// function.
//
//export NewConditionalStyle
func NewConditionalStyle(idx int, style *C.struct_Style) C.struct_NewStyleResult {
	var s excelize.Style
	goVal, err := cValueToGo(reflect.ValueOf(*style), reflect.TypeOf(excelize.Style{}))
	if err != nil {
		return C.struct_NewStyleResult{style: C.int(0), err: C.CString(err.Error())}
	}
	s = goVal.Elem().Interface().(excelize.Style)
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_NewStyleResult{style: C.int(0), err: C.CString(errFilePtr)}
	}
	styleID, err := f.(*excelize.File).NewConditionalStyle(&s)
	if err != nil {
		return C.struct_NewStyleResult{style: C.int(styleID), err: C.CString(err.Error())}
	}
	return C.struct_NewStyleResult{style: C.int(styleID), err: C.CString(errNil)}
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
func NewSheet(idx int, sheet *C.char) C.struct_NewSheetResult {
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_NewSheetResult{idx: C.int(-1), err: C.CString(errFilePtr)}
	}
	idx, err := f.(*excelize.File).NewSheet(C.GoString(sheet))
	if err != nil {
		return C.struct_NewSheetResult{idx: C.int(idx), err: C.CString(err.Error())}
	}
	return C.struct_NewSheetResult{idx: C.int(idx), err: C.CString(errNil)}
}

// NewStyle provides a function to create the style for cells by given options.
// Note that the color field uses RGB color code.
//
//export NewStyle
func NewStyle(idx int, style *C.struct_Style) C.struct_NewStyleResult {
	var s excelize.Style
	goVal, err := cValueToGo(reflect.ValueOf(*style), reflect.TypeOf(excelize.Style{}))
	if err != nil {
		return C.struct_NewStyleResult{style: C.int(0), err: C.CString(err.Error())}
	}
	s = goVal.Elem().Interface().(excelize.Style)
	f, ok := files.Load(idx)
	if !ok {
		return C.struct_NewStyleResult{style: C.int(0), err: C.CString(errFilePtr)}
	}
	styleID, err := f.(*excelize.File).NewStyle(&s)
	if err != nil {
		return C.struct_NewStyleResult{style: C.int(styleID), err: C.CString(err.Error())}
	}
	return C.struct_NewStyleResult{style: C.int(styleID), err: C.CString(errNil)}
}

// OpenFile take the name of a spreadsheet file and returns a populated
// spreadsheet file struct for it.
//
//export OpenFile
func OpenFile(filename *C.char, opts *C.struct_Options) C.struct_OptionsResult {
	var options excelize.Options
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_OptionsResult{idx: C.int(-1), err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	f, err := excelize.OpenFile(C.GoString(filename), options)
	if err != nil {
		return C.struct_OptionsResult{idx: C.int(-1), err: C.CString(err.Error())}
	}
	var idx int
	files.Range(func(_, _ interface{}) bool {
		idx++
		return true
	})
	idx++
	files.Store(idx, f)
	return C.struct_OptionsResult{idx: C.int(idx), err: C.CString(errNil)}
}

// OpenReader read data stream from io.Reader and return a populated spreadsheet
// file.
//
//export OpenReader
func OpenReader(b *C.uchar, bLen C.int, opts *C.struct_Options) C.struct_OptionsResult {
	var options excelize.Options
	if opts != nil {
		goVal, err := cValueToGo(reflect.ValueOf(*opts), reflect.TypeOf(excelize.Options{}))
		if err != nil {
			return C.struct_OptionsResult{idx: C.int(-1), err: C.CString(err.Error())}
		}
		options = goVal.Elem().Interface().(excelize.Options)
	}
	buf := C.GoBytes(unsafe.Pointer(b), bLen)
	f, err := excelize.OpenReader(bytes.NewReader(buf), options)
	if err != nil {
		return C.struct_OptionsResult{idx: C.int(-1), err: C.CString(err.Error())}
	}
	var idx int
	files.Range(func(_, _ interface{}) bool {
		idx++
		return true
	})
	idx++
	files.Store(idx, f)
	return C.struct_OptionsResult{idx: C.int(idx), err: C.CString(errNil)}
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
		return C.CString(errNil)
	}
	if err := f.(*excelize.File).Save(); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
}

// SaveAs provides a function to create or update to a spreadsheet at the
// provided path.
//
//export SaveAs
func SaveAs(idx int, name *C.char, opts *C.struct_Options) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString("")
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
		return C.CString(errNil)
	}
	if err := f.(*excelize.File).SaveAs(C.GoString(name)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
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
	return C.CString(errNil)
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
		C.CString(err.Error())
	}
	return C.CString(errNil)
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
		return C.CString("")
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
		return C.CString(errNil)
	}
	if err := f.(*excelize.File).SetCellFormula(C.GoString(sheet), C.GoString(cell), C.GoString(formula)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
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
		return C.CString("")
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
		return C.CString(errNil)
	}
	if err := f.(*excelize.File).SetCellHyperLink(C.GoString(sheet), C.GoString(cell), C.GoString(link), C.GoString(linkType)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
}

// SetCellInt provides a function to set int type value of a cell by given
// worksheet name, cell reference and cell value.
//
//export SetCellInt
func SetCellInt(idx int, sheet, cell *C.char, value int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetCellInt(C.GoString(sheet), C.GoString(cell), value); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
}

// SetCellRichText provides a function to set cell with rich text by given
// worksheet name, cell reference and rich text runs.
//
//export SetCellRichText
func SetCellRichText(idx int, sheet, cell *C.char, runs *C.struct_RichTextRun, length int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString("")
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
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
		return C.CString("")
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
	return C.CString(errNil)
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
		C.CString(err.Error())
	}
	return C.CString(errNil)
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
	return C.CString(errNil)
}

// SetSheetBackground provides a function to set background picture by given
// worksheet name and file path. Supported image types: BMP, EMF, EMZ, GIF,
// JPEG, JPG, PNG, SVG, TIF, TIFF, WMF, and WMZ.
//
//export SetSheetBackground
func SetSheetBackground(idx int, sheet, picture *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	if err := f.(*excelize.File).SetSheetBackground(C.GoString(sheet), C.GoString(picture)); err != nil {
		C.CString(err.Error())
	}
	return C.CString(errNil)
}

// SetSheetBackgroundFromBytes provides a function to set background picture by
// given worksheet name, extension name and image data. Supported image types:
// BMP, EMF, EMZ, GIF, JPEG, JPG, PNG, SVG, TIF, TIFF, WMF, and WMZ.
//
//export SetSheetBackgroundFromBytes
func SetSheetBackgroundFromBytes(idx int, sheet, extension *C.char, picture *C.uchar, pictureLen C.int) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString(errFilePtr)
	}
	buf := C.GoBytes(unsafe.Pointer(picture), pictureLen)
	if err := f.(*excelize.File).SetSheetBackgroundFromBytes(C.GoString(sheet), C.GoString(extension), buf); err != nil {
		C.CString(err.Error())
	}
	return C.CString(errNil)
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
		C.CString(err.Error())
	}
	return C.CString(errNil)
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
		C.CString(err.Error())
	}
	return C.CString(errNil)
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
		C.CString(err.Error())
	}
	return C.CString(errNil)
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
	return C.CString(errNil)
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
	return C.CString(errNil)
}

// UnmergeCell provides a function to unmerge a given range reference.
//
//export UnmergeCell
func UnmergeCell(idx int, sheet, topLeftCell, bottomRightCell *C.char) *C.char {
	f, ok := files.Load(idx)
	if !ok {
		return C.CString("")
	}
	if err := f.(*excelize.File).UnmergeCell(C.GoString(sheet), C.GoString(topLeftCell), C.GoString(bottomRightCell)); err != nil {
		return C.CString(err.Error())
	}
	return C.CString(errNil)
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
	return C.CString(errNil)
}

func main() {
}
