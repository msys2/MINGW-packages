# Using PoCL in MSYS2

In a non-elevated process, select PoCL before starting an OpenCL application:

```sh
export OCL_ICD_FILENAMES="$(cygpath -w "${MINGW_PREFIX}/bin/pocl.dll")"
```

The Khronos loader ignores `OCL_ICD_FILENAMES` in elevated processes. Those
applications require a `REG_DWORD` value under
`HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\OpenCL\Vendors`.

See the [loader documentation](https://github.com/KhronosGroup/OpenCL-ICD-Loader#registering-icds).
