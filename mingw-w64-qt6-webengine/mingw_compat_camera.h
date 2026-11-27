/**
 * mingw_compat/camera.h - Media Foundation / KS camera declarations that mingw-w64 headers lack,
 * needed by chromium media/capture/video/win (Media Foundation capture).
 * Values and interface layouts follow the Windows SDK 10.0.26100 headers
 * (mfidl.h, ksmedia.h, ks.h). Only what chromium uses is declared.
 */
#ifndef CHROMIUM_MINGW_COMPAT_CAMERA_H_
#define CHROMIUM_MINGW_COMPAT_CAMERA_H_

#if defined(__MINGW32__) && defined(__cplusplus)

#include <windows.h>
#include <unknwn.h>
#include <mfidl.h>

/* ---- ksmedia.h ---- */

/* Partial: chromium only uses these members. SDK values. */
typedef enum {
  KSPROPERTY_CAMERACONTROL_EXTENDED_EYEGAZECORRECTION = 40,
  KSPROPERTY_CAMERACONTROL_EXTENDED_BACKGROUNDSEGMENTATION = 41,
  KSPROPERTY_CAMERACONTROL_EXTENDED_DIGITALWINDOW = 43,
} KSPROPERTY_CAMERACONTROL_EXTENDED_PROPERTY;

#define KSCAMERA_EXTENDEDPROP_EYEGAZECORRECTION_OFF 0x0000000000000000
#define KSCAMERA_EXTENDEDPROP_EYEGAZECORRECTION_ON 0x0000000000000001
#define KSCAMERA_EXTENDEDPROP_EYEGAZECORRECTION_STARE 0x0000000000000002
#define KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_OFF 0x0000000000000000
#define KSCAMERA_EXTENDEDPROP_BACKGROUNDSEGMENTATION_BLUR 0x0000000000000001
#define KSCAMERA_EXTENDEDPROP_DIGITALWINDOW_MANUAL 0x0000000000000000
#define KSCAMERA_EXTENDEDPROP_DIGITALWINDOW_AUTOFACEFRAMING 0x0000000000000001

inline constexpr GUID KSPROPERTYSETID_ExtendedCameraControl = {
    0x1cb79112, 0xc0d2, 0x4213,
    {0x9c, 0xa6, 0xcd, 0x4f, 0xdb, 0x92, 0x79, 0x72}};

/* ---- ks.h ---- */

inline constexpr GUID KSCATEGORY_SENSOR_CAMERA = {
    0x24e552d7, 0x6523, 0x47f7,
    {0xa6, 0x47, 0xd3, 0x46, 0x5b, 0xf1, 0xf5, 0xca}};

/* ---- mfidl.h ---- */

inline constexpr GUID KSPROPERTYSETID_ANYCAMERACONTROL = {
    0x94dd0c30, 0x28c7, 0x4efb,
    {0x9d, 0x6b, 0x81, 0x23, 0x00, 0xfb, 0x0c, 0x7f}};

typedef enum MFSensorDeviceMode {
  MFSensorDeviceMode_Controller = 0,
  MFSensorDeviceMode_Shared = 1
} MFSensorDeviceMode;

MIDL_INTERFACE("39DC7F4A-B141-4719-813C-A7F46162A2B8")
IMFSensorProcessActivity : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE GetProcessId(ULONG* pPID) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetStreamingState(BOOL* pfStreaming) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetStreamingMode(
      MFSensorDeviceMode* pMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetReportTime(FILETIME* pft) = 0;
};
__CRT_UUID_DECL(IMFSensorProcessActivity, 0x39dc7f4a, 0xb141, 0x4719,
                0x81, 0x3c, 0xa7, 0xf4, 0x61, 0x62, 0xa2, 0xb8)

MIDL_INTERFACE("3E8C4BE1-A8C2-4528-90DE-2851BDE5FEAD")
IMFSensorActivityReport : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE GetFriendlyName(LPWSTR FriendlyName,
                                                    ULONG cchFriendlyName,
                                                    ULONG* pcchWritten) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetSymbolicLink(LPWSTR SymbolicLink,
                                                    ULONG cchSymbolicLink,
                                                    ULONG* pcchWritten) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProcessCount(ULONG* pcCount) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProcessActivity(
      ULONG Index, IMFSensorProcessActivity** ppProcessActivity) = 0;
};
__CRT_UUID_DECL(IMFSensorActivityReport, 0x3e8c4be1, 0xa8c2, 0x4528,
                0x90, 0xde, 0x28, 0x51, 0xbd, 0xe5, 0xfe, 0xad)

MIDL_INTERFACE("683F7A5E-4A19-43CD-B1A9-DBF4AB3F7777")
IMFSensorActivitiesReport : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE GetCount(ULONG* pcCount) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetActivityReport(
      ULONG Index, IMFSensorActivityReport** sensorActivityReport) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetActivityReportByDeviceName(
      LPCWSTR SymbolicName, IMFSensorActivityReport** sensorActivityReport) = 0;
};
__CRT_UUID_DECL(IMFSensorActivitiesReport, 0x683f7a5e, 0x4a19, 0x43cd,
                0xb1, 0xa9, 0xdb, 0xf4, 0xab, 0x3f, 0x77, 0x77)

MIDL_INTERFACE("DE5072EE-DBE3-46DC-8A87-B6F631194751")
IMFSensorActivitiesReportCallback : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE OnActivitiesReport(
      IMFSensorActivitiesReport* sensorActivitiesReport) = 0;
};
__CRT_UUID_DECL(IMFSensorActivitiesReportCallback, 0xde5072ee, 0xdbe3,
                0x46dc, 0x8a, 0x87, 0xb6, 0xf6, 0x31, 0x19, 0x47, 0x51)
inline constexpr IID IID_IMFSensorActivitiesReportCallback = {
    0xde5072ee, 0xdbe3, 0x46dc,
    {0x8a, 0x87, 0xb6, 0xf6, 0x31, 0x19, 0x47, 0x51}};

MIDL_INTERFACE("D0CEF145-B3F4-4340-A2E5-7A5080CA05CB")
IMFSensorActivityMonitor : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE Start(void) = 0;
  virtual HRESULT STDMETHODCALLTYPE Stop(void) = 0;
};
__CRT_UUID_DECL(IMFSensorActivityMonitor, 0xd0cef145, 0xb3f4, 0x4340,
                0xa2, 0xe5, 0x7a, 0x50, 0x80, 0xca, 0x05, 0xcb)

MIDL_INTERFACE("E8F2540D-558A-4449-8B64-4863467A9FE8")
IMFCameraControlNotify : public IUnknown {
 public:
  virtual void STDMETHODCALLTYPE OnChange(REFGUID controlSet, UINT32 id) = 0;
  virtual void STDMETHODCALLTYPE OnError(HRESULT hrStatus) = 0;
};
__CRT_UUID_DECL(IMFCameraControlNotify, 0xe8f2540d, 0x558a, 0x4449,
                0x8b, 0x64, 0x48, 0x63, 0x46, 0x7a, 0x9f, 0xe8)
inline constexpr IID IID_IMFCameraControlNotify = {
    0xe8f2540d, 0x558a, 0x4449,
    {0x8b, 0x64, 0x48, 0x63, 0x46, 0x7a, 0x9f, 0xe8}};

MIDL_INTERFACE("4D46F2C9-28BA-4970-8C7B-1F0C9D80AF69")
IMFCameraControlMonitor : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE Start(void) = 0;
  virtual HRESULT STDMETHODCALLTYPE Stop(void) = 0;
  virtual HRESULT STDMETHODCALLTYPE AddControlSubscription(GUID controlSet,
                                                           UINT32 id) = 0;
  virtual HRESULT STDMETHODCALLTYPE RemoveControlSubscription(GUID controlSet,
                                                              UINT32 id) = 0;
  virtual void STDMETHODCALLTYPE Shutdown(void) = 0;
};
__CRT_UUID_DECL(IMFCameraControlMonitor, 0x4d46f2c9, 0x28ba, 0x4970,
                0x8c, 0x7b, 0x1f, 0x0c, 0x9d, 0x80, 0xaf, 0x69)

MIDL_INTERFACE("38E33520-FCA1-4845-A27A-68B7C6AB3789")
IMFExtendedCameraControl : public IUnknown {
 public:
  virtual ULONGLONG STDMETHODCALLTYPE GetCapabilities(void) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetFlags(ULONGLONG ulFlags) = 0;
  virtual ULONGLONG STDMETHODCALLTYPE GetFlags(void) = 0;
  virtual HRESULT STDMETHODCALLTYPE LockPayload(BYTE** ppPayload,
                                                ULONG* pulPayload) = 0;
  virtual HRESULT STDMETHODCALLTYPE UnlockPayload(void) = 0;
  virtual HRESULT STDMETHODCALLTYPE CommitSettings(void) = 0;
};
__CRT_UUID_DECL(IMFExtendedCameraControl, 0x38e33520, 0xfca1, 0x4845,
                0xa2, 0x7a, 0x68, 0xb7, 0xc6, 0xab, 0x37, 0x89)

MIDL_INTERFACE("B91EBFEE-CA03-4AF4-8A82-A31752F4A0FC")
IMFExtendedCameraController : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE GetExtendedCameraControl(
      DWORD dwStreamIndex, ULONG ulPropertyId,
      IMFExtendedCameraControl** ppControl) = 0;
};
__CRT_UUID_DECL(IMFExtendedCameraController, 0xb91ebfee, 0xca03, 0x4af4,
                0x8a, 0x82, 0xa3, 0x17, 0x52, 0xf4, 0xa0, 0xfc)

/* Both are resolved at runtime with GetProcAddress by chromium. */
STDAPI MFCreateSensorActivityMonitor(
    IMFSensorActivitiesReportCallback* pCallback,
    IMFSensorActivityMonitor** ppActivityMonitor);
STDAPI MFCreateCameraControlMonitor(LPCWSTR symbolicLink,
                                    IMFCameraControlNotify* callback,
                                    IMFCameraControlMonitor** ppCameraControlMonitor);

#endif /* __MINGW32__ && __cplusplus */

#endif /* CHROMIUM_MINGW_COMPAT_CAMERA_H_ */
