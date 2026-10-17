/**
 * mingw_compat/winrt.h - Windows Runtime base declarations that mingw-w64
 * lacks or declares differently (winerror.h, asyncinfo.h).
 */
#ifndef CHROMIUM_MINGW_COMPAT_WINRT_H_
#define CHROMIUM_MINGW_COMPAT_WINRT_H_

#if defined(__MINGW32__)

#include <winerror.h>
#include <asyncinfo.h>

/* ---- winerror.h ---- */
#ifndef E_CHANGED_STATE
#define E_CHANGED_STATE 0x8000000C
#endif

#ifdef __cplusplus
/* mingw-w64's asyncinfo.h declares these only in the global namespace; the
 * Windows SDK also provides them in ABI::Windows::Foundation. */
namespace ABI::Windows::Foundation {
using ::AsyncStatus;
using ::IAsyncInfo;
}  // namespace ABI::Windows::Foundation

/* asyncinfo.h only declares IID_IAsyncInfo; no linked library defines it. */
extern "C" {
DECLSPEC_SELECTANY extern const GUID IID_IAsyncInfo = {
    0x00000036, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
}
#endif /* __cplusplus */

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_WINRT_H_ */
