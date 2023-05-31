#!/bin/sh

PATH="${MINGW_PREFIX}/libexec/julia/bin:$PATH"; export PATH
julia.exe $@
