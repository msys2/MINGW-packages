#!/bin/sh

PATH="/opt/julia${MINGW_PREFIX}/bin:$PATH"; export PATH
julia $@
