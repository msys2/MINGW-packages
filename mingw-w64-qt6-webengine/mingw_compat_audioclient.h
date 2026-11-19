/**
 * mingw_compat/audioclient.h - Minimal audioclientactivationparams.h for mingw-w64, which does not ship
 * this Windows SDK header. Definitions follow the public SDK declarations
 * (Windows 10 2004+ process loopback capture).
 */
#ifndef CHROMIUM_MINGW_COMPAT_AUDIOCLIENT_H_
#define CHROMIUM_MINGW_COMPAT_AUDIOCLIENT_H_

#include <windows.h>

typedef enum AUDIOCLIENT_ACTIVATION_TYPE {
  AUDIOCLIENT_ACTIVATION_TYPE_DEFAULT = 0,
  AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK = 1
} AUDIOCLIENT_ACTIVATION_TYPE;

typedef enum PROCESS_LOOPBACK_MODE {
  PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE = 0,
  PROCESS_LOOPBACK_MODE_EXCLUDE_TARGET_PROCESS_TREE = 1
} PROCESS_LOOPBACK_MODE;

typedef struct AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS {
  DWORD TargetProcessId;
  PROCESS_LOOPBACK_MODE ProcessLoopbackMode;
} AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS;

typedef struct AUDIOCLIENT_ACTIVATION_PARAMS {
  AUDIOCLIENT_ACTIVATION_TYPE ActivationType;
  union {
    AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS ProcessLoopbackParams;
  };
} AUDIOCLIENT_ACTIVATION_PARAMS;

#define VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK L"VAD\\Process_Loopback"

/* Also missing from mingw-w64 audioclient.h (SDK 22621+). Used by
 * media/audio/win/audio_low_latency_input_win.cc, the only includer. */
#if defined(__cplusplus) && !defined(__IAcousticEchoCancellationControl_INTERFACE_DEFINED__)
#define __IAcousticEchoCancellationControl_INTERFACE_DEFINED__
#include <unknwn.h>
MIDL_INTERFACE("f4ae25b5-aaa3-437d-b6b3-dbbe2d0e9549")
IAcousticEchoCancellationControl : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE SetEchoCancellationRenderEndpoint(
      LPCWSTR endpointId) = 0;
};
__CRT_UUID_DECL(IAcousticEchoCancellationControl,
                0xf4ae25b5, 0xaaa3, 0x437d,
                0xb6, 0xb3, 0xdb, 0xbe, 0x2d, 0x0e, 0x95, 0x49)
#endif

#endif /* CHROMIUM_MINGW_COMPAT_AUDIOCLIENT_H_ */
