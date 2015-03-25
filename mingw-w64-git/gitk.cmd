@REM Do not use "echo off" to not affect any child calls.

@REM Get the absolute path to the parent directory, which is assumed to be the
@REM Git installation root.
@FOR /F "delims=" %%I IN ("%~dp0..") DO @SET toplevel=%%~fI
@SET MSYSTEM=mingw64
@IF NOT EXIST "%toplevel%\%MSYSTEM%\bin\git.exe" @SET MSYSTEM=mingw32
@SET PATH=%toplevel%\%MSYSTEM%\bin;%toplevel%\usr\bin;%PATH%

@IF NOT EXIST "%HOME%" @set HOME=%HOMEDRIVE%%HOMEPATH%
@IF NOT EXIST "%HOME%" @set HOME=%USERPROFILE%

@START "gitk" wish.exe "%toplevel%\%MSYSTEM%\bin\gitk" -- %*
