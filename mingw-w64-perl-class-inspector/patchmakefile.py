import re
import os
from pathlib import Path

prefix = "\\".join(os.getenv('MINGW_PREFIX','C:\\msys64\\mingw64').split('/')[:-1]) + '\\'
prefix = prefix.replace('\\','\\\\') # needed for regex

pkgdir = os.getenv("pkgdir").replace('/','\\')

reg = re.compile(f'(?P<key>INSTALL(\\S*)) = {prefix}(?P<value>\\S*)')

def do(a: re.Match):
    return f"{a.group('key')} = {a.group('value')}"

with open('Makefile') as f:
    c = f.read()
    c = reg.sub(do,c)
with open('Makefile','w') as f:
    f.write(c)
