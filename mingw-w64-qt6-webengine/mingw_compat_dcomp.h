/**
 * mingw_compat/dcomp.h - DirectComposition items that mingw-w64 lacks
 * (dcomp.h, dcomp-preview.h uuids). Values follow the Windows SDK 10.0.26100.
 */
#ifndef CHROMIUM_MINGW_COMPAT_DCOMP_H_
#define CHROMIUM_MINGW_COMPAT_DCOMP_H_

#if defined(__MINGW32__)

#include <windows.h>
#include <dcomp.h>

#include "third_party/microsoft_dxheaders/src/include/composition/dcomp-preview.h"

/* ---- dcomp.h ---- */
#ifndef DCOMPOSITION_ERROR_SURFACE_BEING_RENDERED
#define DCOMPOSITION_ERROR_SURFACE_BEING_RENDERED ((HRESULT)0x88760822L)
#endif

typedef struct {
  LARGE_INTEGER startTime;
  LARGE_INTEGER targetTime;
  LARGE_INTEGER framePeriod;
} COMPOSITION_FRAME_STATS;

/* ---- dcomp-preview.h: MinGW ignores the IID in DECLARE_INTERFACE_IID_, so
 * __uuidof() needs explicit __mingw_uuidof<> specializations. ---- */
#ifdef __cplusplus
__CRT_UUID_DECL(PREVIEW_IDCompositionDevice5,
                0x2c6bebfe, 0xa603, 0x472f,
                0xaf, 0x34, 0xd2, 0x44, 0x33, 0x56, 0xe6, 0x1b)
__CRT_UUID_DECL(PREVIEW_IDCompositionDynamicTexture,
                0xa1de1d3f, 0x6405, 0x447f,
                0x8e, 0x95, 0x13, 0x83, 0xa3, 0x4b, 0x02, 0x77)
#endif

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_DCOMP_H_ */
