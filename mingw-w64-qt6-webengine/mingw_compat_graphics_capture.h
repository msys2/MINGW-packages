/**
 * mingw_compat/graphics_capture.h - windows.graphics.capture.h items that
 * mingw-w64 lacks. The Windows SDK declares the interface IIDs inside the
 * ABI namespace; mingw-w64 only has the global C names, so alias them to
 * __uuidof() (no link-time GUID definition needed).
 */
#ifndef CHROMIUM_MINGW_COMPAT_GRAPHICS_CAPTURE_H_
#define CHROMIUM_MINGW_COMPAT_GRAPHICS_CAPTURE_H_

#if defined(__MINGW32__) && defined(__cplusplus)

#include <windows.graphics.capture.h>

namespace ABI {
namespace Windows {
namespace Graphics {
namespace Capture {
#ifndef IID_IGraphicsCaptureSession2
inline const IID& IID_IGraphicsCaptureSession2 =
    __uuidof(IGraphicsCaptureSession2);
#endif
#ifndef IID_IGraphicsCaptureSession3
inline const IID& IID_IGraphicsCaptureSession3 =
    __uuidof(IGraphicsCaptureSession3);
#endif
}  // namespace Capture
}  // namespace Graphics
}  // namespace Windows
}  // namespace ABI

#endif /* __MINGW32__ && __cplusplus */

#endif /* CHROMIUM_MINGW_COMPAT_GRAPHICS_CAPTURE_H_ */
