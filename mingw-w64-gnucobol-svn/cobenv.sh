#!/bin/sh
#
# cobenv.sh
# Shell script for setting GnuCOBOL Environment
# in MSYS2 MINGW32/MINGW64
#
# Copyright (C) 2017 Free Software Foundation, Inc.
# Written by Simon Sobisch
#
# This file is part of GnuCOBOL.
#
# The GnuCOBOL compiler is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# GnuCOBOL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GnuCOBOL.  If not, see <http://www.gnu.org/licenses/>.


_ce_help () {
  echo
  echo "$_ce_myname - set, show, store or restore GnuCOBOL environment"
  echo
  echo "usage: source $_ce_myextname [options]"
  echo "   or: .      $_ce_myextname [options]"
  echo " note: if no option is given $_ce_myname defaults to --setenv"
  echo
  echo "options:"
  echo "  --help,    -h  display this information"
  echo "  --verbose, -v  print the environment variables set / restored"
  echo "  --quiet,   -q  don't print anything"
  echo "  --setenv       store current environment and set environment"
  echo "                 according to MSYS2 package structure"
  echo "  --save         store current environment"
  echo "  --restore      restore environment to previous saved point"
  echo "  --showenv      show current environment"
}


_ce_output () {
  if test $_ce_verbose -lt 0; then return; fi
  if test $_ce_verbose -eq 0; then
    echo "$1"
  else
    echo "$1:"
    echo "  COB_LIBRARY_PATH=$COB_LIBRARY_PATH"
    echo "  COB_CONFIG_DIR=$COB_CONFIG_DIR"
    echo "  COB_COPY_DIR=$COB_COPY_DIR"
  fi
}


_ce_save () {
  export "_ce_restore_library_path=$COB_LIBRARY_PATH"
  export "_ce_restore_config=$COB_CONFIG_DIR"
  export "_ce_restore_copy=$COB_COPY_DIR"
  if test $_ce_verbose -lt 0; then return; fi
  _ce_output "environment saved"
}


_ce_setenv () {
  if test $_ce_verbose -ge 0; then
    echo "Setup GnuCOBOL environment (MinGW within MSYS2)..."
  fi
  if test "-z ${_ce_restore_config+x}"; then
    _ce_save
  else
    _ce_restore silent
  fi
  export "COB_CONFIG_DIR=`cygpath.exe -pw $MINGW_PREFIX/share/gnucobol/config`"
  export "COB_COPY_DIR=`cygpath.exe -pw $MINGW_PREFIX/share/gnucobol/copy`"
  if test "x$COB_LIBRARY_PATH" != "x"; then
    "COB_LIBRARY_PATH=:$COB_LIBRARY_PATH"
  fi
  export "COB_LIBRARY_PATH=`cygpath.exe -pw $MINGW_PREFIX/lib/gnucobol`$COB_LIBRARY_PATH"
  if test $_ce_verbose -lt 0; then return; fi
  _ce_output "environment set"
}


_ce_unset_restore () {
  unset "_ce_restore_library_path"
  unset "_ce_restore_config"
  unset "_ce_restore_copy"
}


_ce_restore () {
  if test "-z ${_ce_restore_config+x}"; then
    if test $_ce_verbose -ge 0; then
      echo nothing to restore, environment is cleaned
    fi
    unset "COB_LIBRARY_PATH"
    unset "COB_CONFIG_DIR"
    unset "COB_COPY_DIR"
  else
    export "COB_LIBRARY_PATH=$_ce_restore_library_path"
    export "COB_CONFIG_DIR=$_ce_restore_config"
    export "COB_COPY_DIR=$_ce_restore_copy"
    if test "-z ${1+x}"; then
      _ce_output "environment restored"
    fi
  fi
}


_ce_showenv () {
  _ce_verbose=1
  _ce_output "current environment"
}


# evaluate options
_ce_myname=cobenv.sh
_ce_myextname=$_ce_myname
if test ${BASH_VERSINFO[0]} -gt 2; then
  if test "${BASH_SOURCE[0]}" = "${0}"; then
    # if we know that we aren't sourced we can use the external name
    _ce_myextname=$0
    if test "$1" != "--help" -a "$1" != "-h" -a "$1" != "--showenv"; then
      echo "This shell sets the local environment and therefore needs"
      echo "to be sourced, consider calling $_ce_myname --help !"
      exit 1
    fi
  fi
fi

_ce_verbose=0
case "$2" in
  "--verbose" | "-v") _ce_verbose=1;;
  "--quiet"   | "-q") _ce_verbose=-1;;
  *)
  case "$1" in
    "--verbose" | "-v") _ce_verbose=1  && shift;;
    "--quiet"   | "-q") _ce_verbose=-1 && shift;;
  esac
  ;;
esac

case "$1" in
  "--restore")      _ce_restore && _ce_unset_restore;;
  "--save")         _ce_save;;
  "--help" | "-h")  _ce_help;;
  "--showenv")      _ce_showenv;;
  "" | "--setenv")  _ce_setenv;;
  *)
    echo unrecognized or misplaced option "$1"
    _ce_help
  ;;
esac

unset _ce_help _ce_restore _ce_save _ce_setenv _ce_showenv _ce_output
