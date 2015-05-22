::
:: Copyright (c) 2012 Martin Ridgers
::
::
:: Permission is hereby granted, free of charge, to any person obtaining a copy
:: of this software and associated documentation files (the "Software"), to deal
:: in the Software without restriction, including without limitation the rights
:: to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
:: copies of the Software, and to permit persons to whom the Software is
:: furnished to do so, subject to the following conditions:
::
:: The above copyright notice and this permission notice shall be included in
:: all copies or substantial portions of the Software.
::
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
:: IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
:: FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
:: AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
:: LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
:: OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
:: SOFTWARE.
::

@echo off

:: Pass through to appropriate loader.
::
if /i "%PROCESSOR_ARCHITECTURE%"=="x86" (
    call :loader_x86 %*
) else if /i "%PROCESSOR_ARCHITECTURE%"=="amd64" (
    call :loader_x64 %*
)

:end
goto :eof

:: Helper functions to avoid cmd.exe's issues with brackets.
:loader_x86
if exist "%~dp0../../mingw32/bin/%~n0_x86.exe" (
    "%~dp0../../mingw32/bin/%~n0_x86.exe" %*
    exit /b 0
) else (
    echo Cannot find "%~dp0../../mingw32/bin/%~n0_x86.exe" >&2
    exit /b 1
)

:loader_x64
if exist "%~dp0../../mingw64/bin/%~n0_x64.exe" (
    "%~dp0../../mingw64/bin/%~n0_x64.exe" %*
    exit /b 0
) else (
    echo Cannot find "%~dp0../../mingw64/bin/%~n0_x64.exe" >&2
    exit /b 1
)

exit /b 0
