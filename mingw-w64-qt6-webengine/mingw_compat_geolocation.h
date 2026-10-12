/**
 * mingw_compat/geolocation.h - windows.devices.geolocation.h items that
 * mingw-w64 lacks. Include after <windows.devices.geolocation.h>.
 */
#ifndef CHROMIUM_MINGW_COMPAT_GEOLOCATION_H_
#define CHROMIUM_MINGW_COMPAT_GEOLOCATION_H_

#if defined(__MINGW32__) && defined(__cplusplus)

#include <windows.devices.geolocation.h>

#if WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0x10000
namespace ABI {
namespace Windows {
namespace Devices {
namespace Geolocation {
enum GeolocationAccessStatus : int {
  GeolocationAccessStatus_Unspecified = 0,
  GeolocationAccessStatus_Allowed = 1,
  GeolocationAccessStatus_Denied = 2,
};
}  // namespace Geolocation
}  // namespace Devices
}  // namespace Windows
}  // namespace ABI
#endif /* WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION >= 0x10000 */

#endif /* __MINGW32__ && __cplusplus */

#endif /* CHROMIUM_MINGW_COMPAT_GEOLOCATION_H_ */
