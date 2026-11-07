/**
 * mingw_compat/ksmedia.h - Kernel streaming (ksmedia.h) items that mingw-w64
 * lacks or only declares. Values follow the Windows SDK 10.0.26100.
 */
#ifndef CHROMIUM_MINGW_COMPAT_KSMEDIA_H_
#define CHROMIUM_MINGW_COMPAT_KSMEDIA_H_

#if defined(__MINGW32__)

#include <windows.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

/* ---- Speaker layouts ---- */
#ifndef KSAUDIO_SPEAKER_2POINT1
#define KSAUDIO_SPEAKER_2POINT1 \
  (KSAUDIO_SPEAKER_STEREO | SPEAKER_LOW_FREQUENCY)
#endif
#ifndef KSAUDIO_SPEAKER_3POINT0
#define KSAUDIO_SPEAKER_3POINT0 \
  (KSAUDIO_SPEAKER_STEREO | SPEAKER_FRONT_CENTER)
#endif
#ifndef KSAUDIO_SPEAKER_3POINT1
#define KSAUDIO_SPEAKER_3POINT1 \
  (KSAUDIO_SPEAKER_3POINT0 | SPEAKER_LOW_FREQUENCY)
#endif
#ifndef KSAUDIO_SPEAKER_5POINT0
#define KSAUDIO_SPEAKER_5POINT0 \
  (KSAUDIO_SPEAKER_3POINT0 | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT)
#endif
#ifndef KSAUDIO_SPEAKER_7POINT0
#define KSAUDIO_SPEAKER_7POINT0 \
  (KSAUDIO_SPEAKER_5POINT0 | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
#endif

/* ---- Node types: ksmedia.h only declares these; no import library defines
 * them (MSVC gets them from ksguid.lib). ---- */
#ifdef __cplusplus
extern "C" {
#endif
DECLSPEC_SELECTANY extern const GUID KSNODETYPE_VIDEO_CAMERA_TERMINAL = {
    STATIC_KSNODETYPE_VIDEO_CAMERA_TERMINAL};
DECLSPEC_SELECTANY extern const GUID KSNODETYPE_VIDEO_PROCESSING = {
    STATIC_KSNODETYPE_VIDEO_PROCESSING};
#ifdef __cplusplus
}
#endif

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_KSMEDIA_H_ */
