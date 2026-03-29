/*
 * SPDX-FileCopyrightText: 2026 Mikhail Titov <mlt@gmx.us>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <delayimp.h>
#include <winternl.h>

NTSYSAPI NTSTATUS NTAPI NtRaiseHardError(
    NTSTATUS ErrorStatus,
    ULONG NumberOfParameters,
    ULONG UnicodeStringParameterMask,
    PULONG_PTR Parameters,
    ULONG ValidResponseOptions,
    PULONG Response
);

#define PREFIX L"(optional) "
#define PREFIX_LEN sizeof(PREFIX)/sizeof(wchar_t) - 1

wchar_t wDllName[PREFIX_LEN + MAX_PATH] = PREFIX;

FARPROC WINAPI DelayLoadFailureHook(unsigned dliNotify, PDelayLoadInfo pdli) {
    ULONG response;
    UNICODE_STRING usDllName;

    SetErrorMode(0);
    MultiByteToWideChar(CP_ACP, 0, pdli->szDll, -1, wDllName + PREFIX_LEN, MAX_PATH);
    RtlInitUnicodeString(&usDllName, wDllName);
    NtRaiseHardError(STATUS_DLL_NOT_FOUND, 1, 1, (PULONG_PTR)&(PUNICODE_STRING){ &usDllName }, 1, &response);

    TerminateProcess(GetCurrentProcess(), STATUS_DLL_NOT_FOUND);
    return 0;
}

PfnDliHook __pfnDliFailureHook2 = DelayLoadFailureHook;
