/**
 * mingw_compat/win.h - Windows core declarations missing from mingw-w64
 * (winnt.h, tpcshrd.h, mdmregistration.h, lmjoin.h, CRT).
 * Values and signatures follow the Windows SDK 10.0.26100.
 */
#ifndef CHROMIUM_MINGW_COMPAT_WIN_H_
#define CHROMIUM_MINGW_COMPAT_WIN_H_

#if defined(__MINGW32__)

#include <windows.h>
#include <stdlib.h>

/* ---- winnt.h ---- */
#ifndef DEVICEFAMILYDEVICEFORM_UNKNOWN
#define DEVICEFAMILYDEVICEFORM_UNKNOWN 0x00000000
#define DEVICEFAMILYDEVICEFORM_TABLET 0x00000002
#define DEVICEFAMILYDEVICEFORM_CONVERTIBLE 0x00000005
#define DEVICEFAMILYDEVICEFORM_DETACHABLE 0x00000006
#endif

/* ---- tpcshrd.h ---- */
#ifndef MICROSOFT_TABLETPENSERVICE_PROPERTY
#define MICROSOFT_TABLETPENSERVICE_PROPERTY L"MicrosoftTabletPenServiceProperty"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- mdmregistration.h (header missing; resolved with GetProcAddress) ---- */
HRESULT WINAPI IsDeviceRegisteredWithManagement(
    BOOL* pfIsDeviceRegisteredWithManagement,
    DWORD cchUPN,
    LPWSTR pszUPN);

/* ---- lmjoin.h Azure AD join API (resolved with GetProcAddress) ---- */
typedef struct _DSREG_JOIN_INFO DSREG_JOIN_INFO, *PDSREG_JOIN_INFO;
HRESULT WINAPI NetGetAadJoinInformation(LPCWSTR pcszTenantId,
                                        PDSREG_JOIN_INFO* ppJoinInfo);
VOID WINAPI NetFreeAadJoinInformation(PDSREG_JOIN_INFO pJoinInfo);

/* ---- CRT: exported by msvcrt and UCRT but not declared by mingw-w64 ---- */
_invalid_parameter_handler __cdecl
_set_thread_local_invalid_parameter_handler(_invalid_parameter_handler);

#ifdef __cplusplus
}
#endif

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_WIN_H_ */
