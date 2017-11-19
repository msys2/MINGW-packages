@echo off

:: cobenv.cmd
:: Batch for setting GnuCOBOL Environment in Windows
:: from MSYS2 MINGW32/MINGW64
::
:: Copyright (C) 2017 Free Software Foundation, Inc.
:: Written by Simon Sobisch
::
:: This file is part of GnuCOBOL.
::
:: The GnuCOBOL compiler is free software: you can redistribute it
:: and/or modify it under the terms of the GNU General Public License
:: as published by the Free Software Foundation, either version 3 of the
:: License, or (at your option) any later version.
::
:: GnuCOBOL is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with GnuCOBOL.  If not, see <http://www.gnu.org/licenses/>.
goto :evaluate_opts


:help
  echo.
  echo %_ce_myname% - set, show, store or restore GnuCOBOL environment
  echo.
  echo usage: %_ce_myextname% [options]
  echo  note: if no option is given %_ce_myname% defaults to --setenv
  echo.
  echo options:
  echo   --help,    -h  display this information
  echo   --verbose, -v  print the environment variables set / restored
  echo   --quiet,   -q  don't print anything
  echo   --setenv       store current environment and set environment
  echo                  according to MSYS2 package structure
  echo   --save         store current environment
  echo   --restore      restore environment to previous saved point
  echo   --showenv      show current environment
goto :eof


:output
  if %_ce_verbose% lss 0 goto :eof
  if %_ce_verbose% equ 0 (
    echo %*
    goto :eof
  )
  echo %*:
  echo   PATH=%PATH%
  echo   COB_LIBRARY_PATH=%COB_LIBRARY_PATH%
  echo   COB_CONFIG_DIR=%COB_CONFIG_DIR%
  echo   COB_COPY_DIR=%COB_COPY_DIR%
goto :eof


:save
  set "_ce_restore_path=%PATH%"
  set "_ce_restore_library_path=%COB_LIBRARY_PATH%"
  set "_ce_restore_copy=%COB_COPY_DIR%"
  set "_ce_restore_config=%COB_CONFIG_DIR%"
  call :output environment saved
goto :eof


:setenv
  if %_ce_verbose% geq 0 (
     echo Setup GnuCOBOL environment - MinGW
  )
  if "%_ce_restore_path%"=="" (
    call :save
  ) else (
    call :restore silent
  )
  set "PATH=%MINGW_ROOT_PATH%\bin;%PATH%"
  if not "%COB_LIBRARY_PATH%"=="" set "COB_LIBRARY_PATH=;%COB_LIBRARY_PATH%"
  set "COB_LIBRARY_PATH=%MINGW_ROOT_PATH%\lib\gnucobol%COB_LIBRARY_PATH%"
  set "COB_COPY_DIR=%MINGW_ROOT_PATH%\share\gnucobol\copy"
  set "COB_CONFIG_DIR=%MINGW_ROOT_PATH%\share\gnucobol\config"
  if %_ce_verbose% lss 0 goto :eof
  call :output environment set
goto :eof


:unset_restore
  set "_ce_restore_path="
  set "_ce_restore_library_path="
  set "_ce_restore_copy="
  set "_ce_restore_config="
goto :eof


:restore
  if "%_ce_restore_path%"=="" (
    if %_ce_verbose% geq 0 (
      echo nothing to restore
    )
    set "COB_LIBRARY_PATH="
    set "COB_COPY_DIR="
    set "COB_CONFIG_DIR="
  )
  set "PATH=%_ce_restore_path%"
  set "COB_LIBRARY_PATH=%_ce_restore_library_path%"
  set "COB_COPY_DIR=%_ce_restore_copy%"
  set "COB_CONFIG_DIR=%_ce_restore_config%"
  if [%1]==[] (
    call :output environment restored
  )
goto :eof


:showenv
  set _ce_verbose=1
  call :output current environment
goto :eof


:evaluate_opts
set "_ce_myname=cobenv.bat"
set "_ce_myextname=%0"
set "MINGW_ROOT_PATH=%~dp0.."
set _ce_verbose=0

if /i [%2]==[--verbose] (
  set _ce_verbose=1
) else if /i [%2]==[-v] (
  set _ce_verbose=1
) else if /i [%2]==[--quiet] (
  set _ce_verbose=-1
) else if /i [%2]==[-q] (
  set _ce_verbose=-1
) else if not [%2]==[] (
  shift
) else (
  if /i [%1]==[--verbose] (
    set _ce_verbose=1  && shift
  ) else if /i [%1]==[-v] (
    set _ce_verbose=1  && shift
  ) else if /i [%1]==[--quiet] (
    set _ce_verbose=-1 && shift
  ) else if /i [%1]==[-q] (
    set _ce_verbose=-1 && shift
  )
)

if [%1]==[--restore] (
  call :restore
  call :unset_restore
) else if /i [%1]==[/restore] (
  call :restore
  call :unset_restore
) else if /i [%1]==[--save] (
  call :save
) else if /i [%1]==[/save] (
  call :save
) else if /i [%1]==[--setenv] (
  call :setenv
) else if /i [%1]==[/setenv] (
  call :setenv
) else if [%1]==[] (
  call :setenv
) else if /i [%1]==[--showenv] (
  call :showenv
) else if /i [%1]==[/showenv] (
  call :showenv
) else if /i [%1]==[--help] (
  call :help
) else if /i [%1]==[/help] (
  call :help
) else if /i [%1]==[-h] (
  call :help
) else if /i [%1]==[/?] (
  call :help
) else (
  echo unrecognized or misplaced option "%1"
  call :help
)

set "_ce_verbose="
set "_ce_myname="
set "_ce_myextname="

