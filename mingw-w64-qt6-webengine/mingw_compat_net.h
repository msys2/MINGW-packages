/**
 * mingw_compat/net.h - Windows SDK declarations that mingw-w64 headers lack, needed by chromium
 * net/ (network change notifiers, sockets, DNS, CNG platform keys).
 * Values follow the Windows SDK 10.0.26100 headers (ws2def.h, ws2ipdef.h,
 * ncrypt.h, nldef.h, netioapi.h).
 */
#ifndef CHROMIUM_MINGW_COMPAT_NET_H_
#define CHROMIUM_MINGW_COMPAT_NET_H_

#if defined(__MINGW32__)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <ocidl.h> /* IConnectionPoint(Container); MSVC gets it transitively. */

/* ---- ws2def.h ---- */
#ifndef SO_RANDOMIZE_PORT
#define SO_RANDOMIZE_PORT 0x3005
#endif
#ifndef AI_DNS_ONLY
#define AI_DNS_ONLY 0x00000010
#endif

/* ---- ws2ipdef.h ---- */
#ifndef IP_RECVECN
#define IP_RECVECN 50
#endif
#ifndef IPV6_RECVECN
#define IPV6_RECVECN 50
#endif

/* ---- ncrypt.h ---- */
#ifndef NCRYPT_PCP_PSS_SALT_SIZE_PROPERTY
#define NCRYPT_PCP_PSS_SALT_SIZE_PROPERTY L"PSS Salt Size"
#endif
#ifndef NCRYPT_TPM_PSS_SALT_SIZE_UNKNOWN
#define NCRYPT_TPM_PSS_SALT_SIZE_UNKNOWN 0x00000000
#endif
#ifndef NCRYPT_TPM_PSS_SALT_SIZE_MAXIMUM
#define NCRYPT_TPM_PSS_SALT_SIZE_MAXIMUM 0x00000001
#endif
#ifndef NCRYPT_TPM_PSS_SALT_SIZE_HASHSIZE
#define NCRYPT_TPM_PSS_SALT_SIZE_HASHSIZE 0x00000002
#endif

/* ---- nldef.h / netioapi.h (Windows 10 2004+) ---- */
typedef enum _NL_NETWORK_CONNECTIVITY_LEVEL_HINT {
  NetworkConnectivityLevelHintUnknown = 0,
  NetworkConnectivityLevelHintNone,
  NetworkConnectivityLevelHintLocalAccess,
  NetworkConnectivityLevelHintInternetAccess,
  NetworkConnectivityLevelHintConstrainedInternetAccess,
  NetworkConnectivityLevelHintHidden,
} NL_NETWORK_CONNECTIVITY_LEVEL_HINT;

typedef enum _NL_NETWORK_CONNECTIVITY_COST_HINT {
  NetworkConnectivityCostHintUnknown = 0,
  NetworkConnectivityCostHintUnrestricted,
  NetworkConnectivityCostHintFixed,
  NetworkConnectivityCostHintVariable,
} NL_NETWORK_CONNECTIVITY_COST_HINT;

typedef struct _NL_NETWORK_CONNECTIVITY_HINT {
  NL_NETWORK_CONNECTIVITY_LEVEL_HINT ConnectivityLevel;
  NL_NETWORK_CONNECTIVITY_COST_HINT ConnectivityCost;
  BOOLEAN ApproachingDataLimit;
  BOOLEAN OverDataLimit;
  BOOLEAN Roaming;
} NL_NETWORK_CONNECTIVITY_HINT;

/* Exported by libiphlpapi.a; chromium resolves it with GetProcAddress. */
#ifdef __cplusplus
extern "C" {
#endif
DWORD WINAPI GetNetworkConnectivityHint(
    NL_NETWORK_CONNECTIVITY_HINT* ConnectivityHint);
#ifdef __cplusplus
}
#endif

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_NET_H_ */
