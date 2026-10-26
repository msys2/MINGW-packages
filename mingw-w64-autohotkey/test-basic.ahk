; test-basic.ahk - Smoke tests for MinGW-built AutoHotkey
; Exercises DllCall (int, double, string), CallbackCreate, and basic
; script execution.  Exits 0 on success, 1 on any failure.
#Requires AutoHotkey v2.0

errors := 0
Fail(msg) {
    global errors
    FileAppend "FAIL: " msg "`n", "*"
    errors++
}

; --- Basic arithmetic and strings ---
if (2 + 3 != 5)
    Fail("2 + 3 != 5")
if ("Hello" . " " . "World" != "Hello World")
    Fail("string concatenation")

; --- DllCall: integer return ---
pid := DllCall("GetCurrentProcessId", "UInt")
if (pid = 0)
    Fail("GetCurrentProcessId returned 0")

; --- DllCall: double return (atan2) ---
; This specifically tests the XMM0 preservation fix (patch 0011).
result := DllCall("msvcrt\atan2", "Double", 1.0, "Double", 1.0, "Double")
expected := 0.7853981633974483
if (Abs(result - expected) > 0.0001)
    Fail("atan2(1,1) = " result " (expected ~0.7854)")

; --- DllCall: double return (pow) ---
result := DllCall("msvcrt\pow", "Double", 2.0, "Double", 10.0, "Double")
if (Abs(result - 1024.0) > 0.001)
    Fail("pow(2,10) = " result " (expected 1024)")

; --- DllCall: string output buffer ---
buf := Buffer(260 * 2)
len := DllCall("GetSystemDirectoryW", "Ptr", buf, "UInt", 260, "UInt")
if (len = 0)
    Fail("GetSystemDirectoryW returned 0")
else {
    sysdir := StrGet(buf, "UTF-16")
    if !InStr(sysdir, "system32", false)
        Fail("GetSystemDirectoryW returned '" sysdir "'")
}

; --- CallbackCreate: exercises x64stub.S assembly ---
AddTwo(a, b, *) {
    return a + b
}
cb := CallbackCreate(AddTwo, , 4)
if (cb = 0)
    Fail("CallbackCreate returned 0")
else {
    result := DllCall(cb, "Int", 10, "Int", 20, "Int", 0, "Int", 0, "Int")
    if (result != 30)
        Fail("Callback(10,20) = " result " (expected 30)")
    CallbackFree(cb)
}

; --- Report ---
if (errors > 0) {
    FileAppend errors " test(s) FAILED`n", "*"
    ExitApp 1
}
FileAppend "All tests passed`n", "*"
ExitApp 0
