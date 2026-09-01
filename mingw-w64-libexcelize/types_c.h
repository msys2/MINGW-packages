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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct Interface
{
    int Type;
    int Integer;
    int32_t Integer32;
    char *String;
    double Float64;
    bool Boolean;
};

// Options define the options for opening and reading the spreadsheet.
//
// MaxCalcIterations specifies the maximum iterations for iterative
// calculation, the default value is 0.
//
// Password specifies the password of the spreadsheet in plain text.
//
// RawCellValue specifies if apply the number format for the cell value or get
// the raw value.
//
// UnzipSizeLimit specifies to unzip size limit in bytes on open the
// spreadsheet, this value should be greater than or equal to
// UnzipXMLSizeLimit, the default size limit is 16GB.
//
// UnzipXMLSizeLimit specifies the memory limit on unzipping worksheet and
// shared string table in bytes, worksheet XML will be extracted to system
// temporary directory when the file size is over this value, this value
// should be less than or equal to UnzipSizeLimit, the default value is
// 16MB.
//
// TmpDir specifies the temporary directory for creating temporary files, if the
// value is empty, the system default temporary directory will be used.
//
// ShortDatePattern specifies the short date number format code. In the
// spreadsheet applications, date formats display date and time serial numbers
// as date values. Date formats that begin with an asterisk (*) respond to
// changes in regional date and time settings that are specified for the
// operating system. Formats without an asterisk are not affected by operating
// system settings. The ShortDatePattern used for specifies apply date formats
// that begin with an asterisk.
//
// LongDatePattern specifies the long date number format code.
//
// LongTimePattern specifies the long time number format code.
//
// CultureInfo specifies the country code for applying built-in language number
// format code these effect by the system's local language settings.
struct Options
{
    unsigned int MaxCalcIterations;
    char *Password;
    bool RawCellValue;
    long int UnzipSizeLimit;
    long int UnzipXMLSizeLimit;
    char *TmpDir;
    char *ShortDatePattern;
    char *LongDatePattern;
    char *LongTimePattern;
    unsigned char CultureInfo;
};

// AppProperties directly maps the document application properties.
struct AppProperties
{
    char *Application;
    bool ScaleCrop;
    int DocSecurity;
    char *Company;
    bool LinksUpToDate;
    bool HyperlinksChanged;
    char *AppVersion;
};

// CalcPropsOptions defines the collection of properties the application uses to
// record calculation status and details.
struct CalcPropsOptions
{
    unsigned int *CalcID;
    char **CalcMode;
    bool *FullCalcOnLoad;
    char **RefMode;
    bool *Iterate;
    unsigned int *IterateCount;
    double *IterateDelta;
    bool *FullPrecision;
    bool *CalcCompleted;
    bool *CalcOnSave;
    bool *ConcurrentCalc;
    unsigned int *ConcurrentManualCount;
    bool *ForceFullCalc;
};

// CustomProperty directly maps the custom property of the workbook. The value
// date type may be one of the following: int32, float64, string, bool,
// time.Time, or nil.
struct CustomProperty
{
    char *Name;
    struct Interface Value;
};

// Cell can be used directly in StreamWriter.SetRow to specify a style and
// a value.
struct Cell
{
    int StyleID;
    char *Formula;
    struct Interface Value;
};

// DocProperties directly maps the document core properties.
struct DocProperties
{
    char *Category;
    char *ContentStatus;
    char *Created;
    char *Creator;
    char *Description;
    char *Identifier;
    char *Keywords;
    char *LastModifiedBy;
    char *Modified;
    char *Revision;
    char *Subject;
    char *Title;
    char *Language;
    char *Version;
};

// RowOpts define the options for the set row, it can be used directly in
// StreamWriter.SetRow to specify the style and properties of the row.
struct RowOpts
{
    double Height;
    bool Hidden;
    int StyleID;
    int OutlineLevel;
};

// Border directly maps the border settings of the cells.
struct Border
{
    char *Type;
    char *Color;
    int Style;
};

// Fill directly maps the fill settings of the cells.
struct Fill
{
    char *Type;
    int Pattern;
    int ColorLen;
    char **Color;
    int Shading;
    int Transparency;
};

// Font directly maps the font settings of the fonts.
struct Font
{
    bool Bold;
    bool Italic;
    char *Underline;
    char *Family;
    double Size;
    bool Strike;
    char *Color;
    int ColorIndexed;
    int *ColorTheme;
    double ColorTint;
    char *VertAlign;
    int *Charset;
};

// Alignment directly maps the alignment settings of the cells.
struct Alignment
{
    char *Horizontal;
    int Indent;
    bool JustifyLastLine;
    unsigned int ReadingOrder;
    int RelativeIndent;
    bool ShrinkToFit;
    int TextRotation;
    char *Vertical;
    bool WrapText;
};

// AutoFilterOptions directly maps the auto filter settings.
struct AutoFilterOptions
{
    char *Column;
    char *Expression;
};

// FormulaOpts can be passed to SetCellFormula to use other formula types.
struct FormulaOpts
{
    char **Type;
    char **Ref;
};

// HeaderFooterOptions directly maps the settings of header and footer.
struct HeaderFooterOptions
{
    bool *AlignWithMargins;
    bool DifferentFirst;
    bool DifferentOddEven;
    bool *ScaleWithDoc;
    char *OddHeader;
    char *OddFooter;
    char *EvenHeader;
    char *EvenFooter;
    char *FirstHeader;
    char *FirstFooter;
};

// HeaderFooterImageOptions defines the settings for an image to be accessible
// from the worksheet header and footer options.
struct HeaderFooterImageOptions
{
    unsigned char Position;
    int FileLen;
    unsigned char *File;
	bool IsFooter;
	bool FirstPage;
    char *Extension;
    char *Width;
    char *Height;
};

// HyperlinkOpts can be passed to SetCellHyperlink to set optional hyperlink
// attributes (e.g. display value)
struct HyperlinkOpts
{
    char **Display;
    char **Tooltip;
};

// Protection directly maps the protection settings of the cells.
struct Protection
{
    bool Hidden;
    bool Locked;
};

// Style directly maps the style settings of the cells.
struct Style
{
    int BorderLen;
    struct Border *Border;
    struct Fill Fill;
    struct Font *Font;
    struct Alignment *Alignment;
    struct Protection *Protection;
    int NumFmt;
    int *DecimalPlaces;
    char **CustomNumFmt;
    bool NegRed;
};

// GraphicOptions directly maps the format settings of the picture.
struct GraphicOptions
{
    char *AltText;
    char *Name;
    bool *PrintObject;
    bool *Locked;
    bool LockAspectRatio;
    bool AutoFit;
    bool AutoFitIgnoreAspect;
    int OffsetX;
    int OffsetY;
    double ScaleX;
    double ScaleY;
    char *Hyperlink;
    char *HyperlinkType;
    char *Positioning;
};

// PageLayoutMarginsOptions directly maps the settings of page layout margins.
struct PageLayoutMarginsOptions
{
    double *Bottom;
    double *Footer;
    double *Header;
    double *Left;
    double *Right;
    double *Top;
    bool *Horizontally;
    bool *Vertically;
};

// PageLayoutOptions directly maps the settings of page layout.
struct PageLayoutOptions
{
    int *Size;
    char **Orientation;
    unsigned int *FirstPageNumber;
    unsigned int *AdjustTo;
    int *FitToHeight;
    int *FitToWidth;
    bool *BlackAndWhite;
    char **PageOrder;
};

// Picture maps the format settings of the picture.
struct Picture
{
    char *Extension;
    int FileLen;
    unsigned char *File;
    struct GraphicOptions *Format;
    unsigned char InsertType;
};

// Selection directly maps the settings of the worksheet selection.
struct Selection
{
    char *SQRef;
    char *ActiveCell;
    char *Pane;
};

// Panes directly maps the settings of the panes.
struct Panes
{
    bool Freeze;
    bool Split;
    int XSplit;
    int YSplit;
    char *TopLeftCell;
    char *ActivePane;
    int SelectionLen;
    struct Selection *Selection;
};

// RichTextRun directly maps the settings of the rich text run.
struct RichTextRun
{
    struct Font *Font;
    char *Text;
};

// Comment directly maps the comment information.
struct Comment
{
    char *Author;
    int AuthorID;
    char *Cell;
    char *Text;
    unsigned int Width;
    unsigned int Height;
    int ParagraphLen;
    struct RichTextRun *Paragraph;
};

// ConditionalFormatOptions directly maps the conditional format settings of the cells.
struct ConditionalFormatOptions
{
    char *Type;
    bool AboveAverage;
    bool Percent;
    int *Format;
    char *Criteria;
    char *Value;
    char *MinType;
    char *MidType;
    char *MaxType;
    char *MinValue;
    char *MidValue;
    char *MaxValue;
    char *MinColor;
    char *MidColor;
    char *MaxColor;
    char *BarColor;
    char *BarBorderColor;
    char *BarDirection;
    bool BarOnly;
    bool BarSolid;
    char *IconStyle;
    bool ReverseIcons;
    bool IconsOnly;
    bool StopIfTrue;
};

// FormControl directly maps the form controls information.
struct FormControl
{
    char *Cell;
    char *Macro;
    unsigned int Width;
    unsigned int Height;
    bool Checked;
    unsigned int CurrentVal;
    unsigned int MinVal;
    unsigned int MaxVal;
    unsigned int IncChange;
    unsigned int PageChange;
    bool Horizontally;
    char *CellLink;
    char *Text;
    int ParagraphLen;
    struct RichTextRun *Paragraph;
    unsigned char Type;
    struct GraphicOptions Format;
};

// ChartNumFmt directly maps the number format settings of the chart.
struct ChartNumFmt
{
    char *CustomNumFmt;
    bool SourceLinked;
};

// ChartAxis directly maps the format settings of the chart axis.
struct ChartAxis
{
    bool None;
    bool DropLines;
    bool HighLowLines;
    bool MajorGridLines;
    bool MinorGridLines;
    double MajorUnit;
    unsigned char TickLabelPosition;
    int TickLabelSkip;
    bool ReverseOrder;
    bool Secondary;
    double *Maximum;
    double *Minimum;
    struct Alignment Alignment;
    struct Font Font;
    double LogBase;
    struct ChartNumFmt NumFmt;
    int TitleLen;
    struct RichTextRun *Title;
};

// ChartDataLabel directly maps the format settings of the chart labels.
struct ChartDataLabel
{
    struct Alignment Alignment;
    struct Font Font;
    struct Fill Fill;
};

// ChartDimension directly maps the dimension of the chart.
struct ChartDimension
{
    unsigned int Width;
    unsigned int Height;
};

// ChartLine directly maps the format settings of the chart line.
struct ChartLine
{
    unsigned char Type;
    unsigned char Dash;
    struct Fill Fill;
    bool Smooth;
    double Width;
};

// ChartUpDownBar directly maps the format settings of the stock chart up bars
// and down bars.
struct ChartUpDownBar
{
    struct Fill Fill;
    struct ChartLine Border;
};

// ChartPlotArea directly maps the format settings of the plot area.
struct ChartPlotArea
{
    int SecondPlotValues;
    bool ShowBubbleSize;
    bool ShowCatName;
    bool ShowDataTable;
    bool ShowDataTableKeys;
    bool ShowLeaderLines;
    bool ShowPercent;
    bool ShowSerName;
    bool ShowVal;
    struct Fill Fill;
    struct ChartUpDownBar UpBars;
    struct ChartUpDownBar DownBars;
    struct ChartNumFmt NumFmt;
};

// ChartLegend directly maps the format settings of the chart legend.
struct ChartLegend
{
    char *Position;
    bool ShowLegendKey;
    struct Font *Font;
};

// ChartMarker directly maps the format settings of the chart marker.
struct ChartMarker
{
    struct ChartLine Border;
    struct Fill Fill;
    char *Symbol;
    int Size;
};

// ChartDataPoint directly maps the format settings of the chart data point for
// doughnut, pie and 3D pie charts.
struct ChartDataPoint
{
    int Index;
    struct Fill Fill;
};

// ChartSeries directly maps the format settings of the chart series.
struct ChartSeries
{
    char *Name;
    char *Categories;
    char *Values;
    char *Sizes;
    struct Fill Fill;
    struct ChartLegend Legend;
    struct ChartLine Line;
    struct ChartMarker Marker;
    struct ChartDataLabel DataLabel;
    unsigned char DataLabelPosition;
    int DataPointLen;
    struct ChartDataPoint *DataPoint;
};

// Chart directly maps the format settings of the chart.
struct Chart
{
    unsigned char Type;
    int SeriesLen;
    struct ChartSeries *Series;
    struct GraphicOptions Format;
    struct ChartDimension Dimension;
    struct ChartLegend Legend;
    int TitleLen;
    struct RichTextRun *Title;
    bool *VaryColors;
    struct ChartAxis XAxis;
    struct ChartAxis YAxis;
    struct ChartPlotArea PlotArea;
    struct Fill Fill;
    struct ChartLine Border;
    char *ShowBlanksAs;
    int BubbleSize;
    int HoleSize;
    unsigned int *GapWidth;
    int *Overlap;
};

// PivotTableField directly maps the field settings of the pivot table.
struct PivotTableField
{
    bool Compact;
    char *Data;
    char *Name;
    bool Outline;
    bool ShowAll;
    bool InsertBlankRow;
    char *Subtotal;
    bool DefaultSubtotal;
    int NumFmt;
};

// PivotTableOptions directly maps the format settings of the pivot table.
struct PivotTableOptions
{
    char *DataRange;
    char *PivotTableRange;
    char *Name;
    int RowsLen;
    struct PivotTableField *Rows;
    int ColumnsLen;
    struct PivotTableField *Columns;
    int DataLen;
    struct PivotTableField *Data;
    int FilterLen;
    struct PivotTableField *Filter;
    bool RowGrandTotals;
    bool ColGrandTotals;
    bool ShowDrill;
    bool UseAutoFormatting;
    bool PageOverThenDown;
    bool MergeItem;
    bool ClassicLayout;
    bool CompactData;
    bool ShowError;
    bool ShowRowHeaders;
    bool ShowColHeaders;
    bool ShowRowStripes;
    bool ShowColStripes;
    bool ShowLastColumn;
    bool FieldPrintTitles;
    bool ItemPrintTitles;
    char *PivotTableStyleName;
};

// ShapeLine directly maps the line settings of the shape.
struct ShapeLine
{
    char *Color;
    double *Width;
};

// Shape directly maps the format settings of the shape.
struct Shape
{
    char *Cell;
    char *Type;
    char *Macro;
    unsigned int Width;
    unsigned int Height;
    struct GraphicOptions Format;
    struct Fill Fill;
    struct ShapeLine Line;
    int ParagraphLen;
    struct RichTextRun *Paragraph;
};

// SheetPropsOptions directly maps the settings of sheet view.
struct SheetPropsOptions
{
    char **CodeName;
    bool *EnableFormatConditionsCalculation;
    bool *Published;
    bool *AutoPageBreaks;
    bool *FitToPage;
    int *TabColorIndexed;
    char **TabColorRGB;
    int *TabColorTheme;
    double *TabColorTint;
    bool *OutlineSummaryBelow;
    bool *OutlineSummaryRight;
    unsigned int *BaseColWidth;
    double *DefaultColWidth;
    double *DefaultRowHeight;
    bool *CustomHeight;
    bool *ZeroHeight;
    bool *ThickTop;
    bool *ThickBottom;
};

// SheetProtectionOptions directly maps the settings of worksheet protection.
struct SheetProtectionOptions
{
    char *AlgorithmName;
    bool AutoFilter;
    bool DeleteColumns;
    bool DeleteRows;
    bool EditObjects;
    bool EditScenarios;
    bool FormatCells;
    bool FormatColumns;
    bool FormatRows;
    bool InsertColumns;
    bool InsertHyperlinks;
    bool InsertRows;
    char *Password;
    bool PivotTables;
    bool SelectLockedCells;
    bool SelectUnlockedCells;
    bool Sort;
};

// SlicerOptions represents the settings of the slicer.
struct SlicerOptions
{
    char *Name;
    char *Cell;
    char *TableSheet;
    char *TableName;
    char *Caption;
    char *Macro;
    unsigned int Width;
    unsigned int Height;
    bool *DisplayHeader;
    bool ItemDesc;
    struct GraphicOptions Format;
};

// SparklineOptions directly maps the settings of the sparkline.
struct SparklineOptions
{
    int LocationLen;
    char **Location;
    int RangeLen;
    char **Range;
    int Max;
    int CustMax;
    int Min;
    int CustMin;
    char *Type;
    double Weight;
    bool DateAxis;
    bool Markers;
    bool High;
    bool Low;
    bool First;
    bool Last;
    bool Negative;
    bool Axis;
    bool Hidden;
    bool Reverse;
    int Style;
    char *SeriesColor;
    char *NegativeColor;
    char *MarkersColor;
    char *FirstColor;
    char *LastColor;
    char *HightColor;
    char *LowColor;
    char *EmptyCells;
};

// Table directly maps the format settings of the table.
struct Table
{
    char *Range;
    char *Name;
    char *StyleName;
    bool ShowColumnStripes;
    bool ShowFirstColumn;
    bool *ShowHeaderRow;
    bool ShowLastColumn;
    bool *ShowRowStripes;
};

// ViewOptions directly maps the settings of sheet view.
struct ViewOptions
{
    bool *DefaultGridColor;
    bool *RightToLeft;
    bool *ShowFormulas;
    bool *ShowGridLines;
    bool *ShowRowColHeaders;
    bool *ShowRuler;
    bool *ShowZeros;
    char **TopLeftCell;
    char **View;
    double *ZoomScale;
};

// DefinedName directly maps the name for a cell or cell range on a
// worksheet.
struct DefinedName
{
    char *Name;
    char *Comment;
    char *RefersTo;
    char *Scope;
};

// WorkbookPropsOptions directly maps the settings of workbook proprieties.
struct WorkbookPropsOptions
{
    bool *Date1904;
    bool *FilterPrivacy;
    char **CodeName;
};

// WorkbookProtectionOptions directly maps the settings of workbook protection.
struct WorkbookProtectionOptions
{
    char *AlgorithmName;
    char *Password;
    bool LockStructure;
    bool LockWindows;
};

struct StringErrorResult
{
    char *val;
    char *err;
};

struct StringIntErrorResult
{
    char *strVal;
    int intVal;
    char *err;
};

struct IntErrorResult
{
    int val;
    char *err;
};

struct BoolErrorResult
{
    bool val;
    char *err;
};

struct Float64ErrorResult
{
    double val;
    char *err;
};

struct StringArrayErrorResult
{
    int ArrLen;
    char **Arr;
    char *Err;
};

struct IntStringResult
{
    int K;
    char *V;
};

struct GetCalcPropsResult
{
    struct CalcPropsOptions opts;
    char *err;
};

struct GetCellHyperLinkResult
{
    bool link;
    char *target;
    char *err;
};

struct CellNameToCoordinatesResult
{
    int col;
    int row;
    char *err;
};

struct GetAppPropsResult
{
    struct AppProperties opts;
    char *err;
};

struct Cells
{
    int CellLen;
    char **Cell;
};

struct StringMatrixErrorResult
{
    int RowLen;
    struct Cells *Row;
    char *err;
};

struct GetCellRichTextResult
{
    int RunsLen;
    struct RichTextRun *Runs;
    char *Err;
};

struct GetSlicersResult
{
    int SlicersLen;
    struct SlicerOptions *Slicers;
    char *Err;
};

struct GetStyleResult
{
    struct Style style;
    char *err;
};

struct GetTablesResult
{
    int TablesLen;
    struct Table *Tables;
    char *Err;
};

struct GetWorkbookPropsResult
{
    struct WorkbookPropsOptions opts;
    char *err;
};

struct GetSheetPropsResult
{
    struct SheetPropsOptions opts;
    char *err;
};

struct GetSheetProtectionResult
{
    struct SheetProtectionOptions opts;
    char *err;
};

struct GetSheetMapResult
{
    int ArrLen;
    struct IntStringResult *Arr;
    char *Err;
};

struct GetSheetViewResult
{
    struct ViewOptions opts;
    char *err;
};


struct GetCommentsResult
{
    int CommentsLen;
    struct Comment *Comments;
    char *Err;
};

struct GetCustomPropsResult
{
    int CustomPropsLen;
    struct CustomProperty *CustomProps;
    char *Err;
};

struct GetDefinedNameResult
{
    int DefinedNamesLen;
    struct DefinedName *DefinedNames;
    char *Err;
};

struct GetDocPropsResult
{
    struct DocProperties opts;
    char *err;
};

struct GetFormControlsResult
{
    int FormControlsLen;
    struct FormControl *FormControls;
    char *Err;
};

struct GetPageLayoutResult
{
    struct PageLayoutOptions opts;
    char *err;
};

struct GetPageMarginsResult
{
    struct PageLayoutMarginsOptions opts;
    char *err;
};

struct GetPicturesResult
{
    int PicturesLen;
    struct Picture *Pictures;
    char *Err;
};

struct GetPivotTablesResult
{
    int PivotTablesLen;
    struct PivotTableOptions *PivotTables;
    char *Err;
};

struct GetRowOptsResult
{
    struct RowOpts opts;
    char *err;
};
