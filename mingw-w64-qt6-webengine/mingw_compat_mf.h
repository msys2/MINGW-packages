/**
 * mingw_compat/mf.h - Media Foundation and codec API items that mingw-w64
 * lacks or only declares (mfapi.h, mfidl.h, mfobjects.h, codecapi.h,
 * strmif.h). Values follow the Windows SDK 10.0.26100.
 *
 * Include this BEFORE any <codecapi.h>: mingw-w64's codecapi.h emits
 * __CRT_UUID_DECL for its CODECAPI_* GUIDs only if guiddef.h was seen first,
 * otherwise __uuidof() on them stays undefined at link time.
 */
#ifndef CHROMIUM_MINGW_COMPAT_MF_H_
#define CHROMIUM_MINGW_COMPAT_MF_H_

#if defined(__MINGW32__)

#include <guiddef.h>
#include <windows.h>
#include <strmif.h>
#include <codecapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>

/* ---- strmif.h: ICodecAPI has no __CRT_UUID_DECL, and the interface guard
 * keeps icodecapi.h from adding one. ---- */
#ifdef __cplusplus
__CRT_UUID_DECL(ICodecAPI,
                0x901db4c7, 0x31ce, 0x41a2,
                0x85, 0xdc, 0x8f, 0xa0, 0xbf, 0x41, 0xb8, 0xda)
#endif

/* ---- codecapi.h ---- */
#if !defined(CODECAPI_AVEncAACEnableVBR)
DEFINE_CODECAPI_GUID(AVEncAACEnableVBR,
                     "e836bb98-fca3-44b6-9a39-24786be41be1",
                     0xe836bb98, 0xfca3, 0x44b6,
                     0x9a, 0x39, 0x24, 0x78, 0x6b, 0xe4, 0x1b, 0xe1)
#define CODECAPI_AVEncAACEnableVBR DEFINE_CODECAPI_GUIDNAMED(AVEncAACEnableVBR)
#endif

enum eAVEncAV1VLevel {
  eAVEncAV1VLevel2 = 0,
  eAVEncAV1VLevel2_1 = 1,
  eAVEncAV1VLevel3 = 4,
  eAVEncAV1VLevel3_1 = 5,
  eAVEncAV1VLevel4 = 8,
  eAVEncAV1VLevel4_1 = 9,
  eAVEncAV1VLevel5 = 12,
  eAVEncAV1VLevel5_1 = 13,
  eAVEncAV1VLevel5_2 = 14,
  eAVEncAV1VLevel5_3 = 15,
  eAVEncAV1VLevel6 = 16,
  eAVEncAV1VLevel6_1 = 17,
  eAVEncAV1VLevel6_2 = 18,
  eAVEncAV1VLevel6_3 = 19,
};

/* ---- mfobjects.h: newer enum values ---- */
#if !defined(MFVideoPrimaries_Display_P3)
#define MFVideoPrimaries_Display_P3 static_cast<MFVideoPrimaries>(13)
#endif
#if !defined(MFVideoTransFunc_BT1361_ECG)
#define MFVideoTransFunc_BT1361_ECG \
  static_cast<MFVideoTransferFunction>(18)
#endif
#if !defined(MFVideoTransFunc_SMPTE428)
#define MFVideoTransFunc_SMPTE428 static_cast<MFVideoTransferFunction>(19)
#endif
#if !defined(MFVideoTransferMatrix_Identity)
#define MFVideoTransferMatrix_Identity \
  static_cast<MFVideoTransferMatrix>(6)
#endif
#if !defined(MFVideoTransferMatrix_FCC47)
#define MFVideoTransferMatrix_FCC47 static_cast<MFVideoTransferMatrix>(7)
#endif
#if !defined(MFVideoTransferMatrix_YCgCo)
#define MFVideoTransferMatrix_YCgCo static_cast<MFVideoTransferMatrix>(8)
#endif
#if !defined(MFVideoTransferMatrix_SMPTE2085)
#define MFVideoTransferMatrix_SMPTE2085 \
  static_cast<MFVideoTransferMatrix>(9)
#endif

/* ---- mfidl.h: video processor ---- */
typedef enum _MF_VIDEO_PROCESSOR_ALGORITHM_TYPE {
  MF_VIDEO_PROCESSOR_ALGORITHM_DEFAULT = 0,
  MF_VIDEO_PROCESSOR_ALGORITHM_MRF_CRF_444 = 1
} MF_VIDEO_PROCESSOR_ALGORITHM_TYPE;

EXTERN_GUID(MF_VIDEO_PROCESSOR_ALGORITHM, 0x4a0a1e1f, 0x272c, 0x4fb6, 0x9e, 0xb1, 0xdb, 0x33, 0xc, 0xbc, 0x97, 0xca);
EXTERN_GUID(MF_XVP_DISABLE_FRC, 0x2c0afa19, 0x7a97, 0x4d5a, 0x9e, 0xe8, 0x16, 0xd4, 0xfc, 0x51, 0x8d, 0x8c);

/* ---- mfapi.h: GUIDs no mingw-w64 library defines, and MFTEnum2 ---- */
#ifdef __cplusplus
extern "C" {
#endif
DECLSPEC_SELECTANY extern const GUID MFT_ENUM_ADAPTER_LUID = {
    0x1d39518c, 0xe220, 0x4da8, {0xa0, 0x7f, 0xba, 0x17, 0x25, 0x52, 0xd6, 0xb1}};
DECLSPEC_SELECTANY extern const GUID MF_VIDEO_MAX_MB_PER_SEC = {
    0xe3f2e203, 0xd445, 0x4b8c, {0x92, 0x11, 0xae, 0x39, 0xd, 0x3b, 0xa0, 0x17}};
DECLSPEC_SELECTANY extern const GUID MF_DISABLE_FRAME_CORRUPTION_INFO = {
    0x7086e16c, 0x49c5, 0x4201, {0x88, 0x2a, 0x85, 0x38, 0xf3, 0x8c, 0xf1, 0x3a}};
DECLSPEC_SELECTANY extern const GUID MF_MT_VIDEO_PROFILE = {
    0xad76a80b, 0x2d5c, 0x4e0b, {0xb3, 0x75, 0x64, 0xe5, 0x20, 0x13, 0x70, 0x36}};
DECLSPEC_SELECTANY extern const GUID MF_MT_VIDEO_LEVEL = {
    0x96f66574, 0x11c5, 0x4015, {0x86, 0x66, 0xbf, 0xf5, 0x16, 0x43, 0x6d, 0xa7}};

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
STDAPI MFTEnum2(GUID guidCategory,
                UINT32 Flags,
                const MFT_REGISTER_TYPE_INFO* pInputType,
                const MFT_REGISTER_TYPE_INFO* pOutputType,
                IMFAttributes* pAttributes,
                IMFActivate*** pppMFTActivate,
                UINT32* pnumMFTActivate);
#endif
#ifdef __cplusplus
}
#endif

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_MF_H_ */
