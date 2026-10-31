/**
 * mingw_compat/crypto.h - CNG (bcrypt.h / ncrypt.h) constants that
 * mingw-w64 lacks. Values follow the Windows SDK 10.0.26100.
 */
#ifndef CHROMIUM_MINGW_COMPAT_CRYPTO_H_
#define CHROMIUM_MINGW_COMPAT_CRYPTO_H_

#if defined(__MINGW32__)

#include <windows.h>
#include <bcrypt.h>
#include <ncrypt.h>

/* ---- bcrypt.h ---- */
#ifndef BCRYPT_ECC_CURVE_NAME
#define BCRYPT_ECC_CURVE_NAME L"ECCCurveName"
#endif
#ifndef BCRYPT_ECC_CURVE_NISTP256
#define BCRYPT_ECC_CURVE_NISTP256 L"nistP256"
#endif

/* ---- ncrypt.h ---- */
#ifndef NCRYPT_ECC_CURVE_NAME_PROPERTY
#define NCRYPT_ECC_CURVE_NAME_PROPERTY BCRYPT_ECC_CURVE_NAME
#endif
#ifndef NCRYPT_USE_VIRTUAL_ISOLATION_FLAG
#define NCRYPT_USE_VIRTUAL_ISOLATION_FLAG 0x00020000
#endif
#ifndef NCRYPT_PROTECT_TO_LOCAL_SYSTEM
#define NCRYPT_PROTECT_TO_LOCAL_SYSTEM 0x00008000
#endif

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_CRYPTO_H_ */
