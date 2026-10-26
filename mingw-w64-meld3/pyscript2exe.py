"""
Creates an exe launcher for Python scripts for the executing interpreter.
foobar.py -> foobar.exe + foobar-script.py
"""

import sys
import re
import os
import shutil
try:
    from setuptools.command.easy_install import get_win_launcher
except ImportError:
    # v80+
    from setuptools._scripts import get_win_launcher

path = sys.argv[1]
root, ext = os.path.splitext(path)
with open(path, "rb") as f:
    data = f.read()
with open(path, "wb") as f:
    f.write(re.sub(b"^#![^\n\r]*", b'#!python.exe', data))
with open(root + ".exe", "wb") as f:
    f.write(get_win_launcher("cli"))
shutil.copy(path, root + "-script.py")
with open(path, "wb") as f:
    f.write(re.sub(b"^#![^\n\r]*", b'#!pythonw.exe', data))
with open(root + "w.exe", "wb") as f:
    f.write(get_win_launcher("gui"))
os.rename(path, root + "w-script.pyw")
