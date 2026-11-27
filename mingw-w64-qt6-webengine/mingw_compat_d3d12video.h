/**
 * mingw_compat/d3d12video.h - D3D12 video declarations from the
 * directx-headers package, with the uuids mingw-w64 does not register.
 * Use instead of <d3d12video.h>.
 */
#ifndef CHROMIUM_MINGW_COMPAT_D3D12VIDEO_H_
#define CHROMIUM_MINGW_COMPAT_D3D12VIDEO_H_

#if defined(__MINGW32__)

#include <directx/d3d12video.h>

/* <directx/d3d12video.h> declares these with MIDL_INTERFACE, which only
 * emits __declspec(uuid) under MSVC; register the IIDs explicitly so
 * __mingw_uuidof<>() / IID_PPV_ARGS resolve them. */
#ifdef __cplusplus
__CRT_UUID_DECL(ID3D12VideoDevice,
                0x1f052807, 0x0b46, 0x4acc,
                0x8a, 0x89, 0x36, 0x4f, 0x79, 0x37, 0x18, 0xa4);
__CRT_UUID_DECL(ID3D12VideoDecoder,
                0xc59b6bdc, 0x7720, 0x4074,
                0xa1, 0x36, 0x17, 0xa1, 0x56, 0x03, 0x74, 0x70);
__CRT_UUID_DECL(ID3D12VideoDecoderHeap,
                0x0946b7c9, 0xebf6, 0x4047,
                0xbb, 0x73, 0x86, 0x83, 0xe2, 0x7d, 0xbb, 0x1f);
__CRT_UUID_DECL(ID3D12VideoDecodeCommandList,
                0x3b60536e, 0xad29, 0x4e64,
                0xa2, 0x69, 0xf8, 0x53, 0x83, 0x7e, 0x5e, 0x53);
#endif

#endif /* __MINGW32__ */

#endif /* CHROMIUM_MINGW_COMPAT_D3D12VIDEO_H_ */
