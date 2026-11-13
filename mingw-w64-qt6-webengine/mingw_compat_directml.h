/**
 * mingw_compat/directml.h - DirectML from the directx-headers package, with
 * the uuid mingw-w64 does not register. Use instead of <DirectML.h>.
 */
#ifndef CHROMIUM_MINGW_COMPAT_DIRECTML_H_
#define CHROMIUM_MINGW_COMPAT_DIRECTML_H_

#if defined(__MINGW32__)

#include <directx/DirectML.h>

#ifdef __cplusplus
__CRT_UUID_DECL(IDMLDevice,
                0xa0884f9a, 0xd2be, 0x4355,
                0xaa, 0x5d, 0x59, 0x01, 0x28, 0x1a, 0xd1, 0xd2);
#endif

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_DIRECTML_H_ */
