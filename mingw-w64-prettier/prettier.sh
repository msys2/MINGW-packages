#!/usr/bin/env bash
_node_exe="$(dirname "${BASH_SOURCE[0]}")/node.exe"
if [ ! -f "$_node_exe" ]; then
  _node_exe="node"
fi
_script="$(dirname "${BASH_SOURCE[0]}")/../lib/node_modules/prettier/bin/prettier.cjs"

if [ -t 0 -a -t 1 -a -x /usr/bin/winpty ]; then
  /usr/bin/winpty "$_node_exe" "$_script" "$@"
else
  exec "$_node_exe" "$_script" "$@"
fi
