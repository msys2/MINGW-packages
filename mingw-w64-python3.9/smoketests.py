#!/usr/bin/env python3
# Copyright 2017 Christoph Reiter
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

"""The goal of this test suite is collect tests for update regressions
and to test msys2 related modifications like for path handling.
Feel free to extend.
"""

import os
import unittest

if "MSYSTEM" in os.environ:
    SEP = "/"
else:
    SEP = "\\"


class Tests(unittest.TestCase):

    def test_sep(self):
        self.assertEqual(os.sep, SEP)

    def test_module_file_path(self):
        import asyncio
        import zlib
        self.assertEqual(zlib.__file__, os.path.normpath(zlib.__file__))
        self.assertEqual(asyncio.__file__, os.path.normpath(asyncio.__file__))

    def test_importlib_frozen_path_sep(self):
        import importlib._bootstrap_external
        self.assertEqual(importlib._bootstrap_external.path_sep, SEP)

    def test_os_commonpath(self):
        self.assertEqual(
            os.path.commonpath(
                [os.path.join("C:", os.sep, "foo", "bar"),
                 os.path.join("C:", os.sep, "foo")]),
                 os.path.join("C:", os.sep, "foo"))

    def test_pathlib(self):
        import pathlib

        p = pathlib.Path("foo") / pathlib.Path("foo")
        self.assertEqual(str(p), os.path.normpath(p))

    def test_modules_import(self):
        import sqlite3
        import ssl
        import ctypes

    def test_socket_inet_ntop(self):
        import socket
        self.assertTrue(hasattr(socket, "inet_ntop"))

    def test_socket_inet_pton(self):
        import socket
        self.assertTrue(hasattr(socket, "inet_pton"))

    def test_multiprocessing_queue(self):
        from multiprocessing import Queue
        Queue(0)

    def test_socket_timout_normal_error(self):
        import urllib.request
        from urllib.error import URLError

        try:
            urllib.request.urlopen(
                'http://localhost', timeout=0.0001).close()
        except URLError:
            pass

    def test_threads(self):
        from concurrent.futures import ThreadPoolExecutor

        with ThreadPoolExecutor(1) as pool:
            for res in pool.map(lambda *x: None, range(10000)):
                pass

    def test_sysconfig(self):
        import sysconfig
        # This should be able to execute without exceptions
        sysconfig.get_config_vars()

    def test_sqlite_enable_load_extension(self):
        # Make sure --enable-loadable-sqlite-extensions is used
        import sqlite3
        self.assertTrue(sqlite3.Connection.enable_load_extension)

    def test_venv_creation(self):
        import tempfile
        import venv
        import subprocess
        import shutil
        with tempfile.TemporaryDirectory() as tmp:
            builder = venv.EnvBuilder()
            builder.create(tmp)
            assert os.path.exists(os.path.join(tmp, "bin", "activate"))
            assert os.path.exists(os.path.join(tmp, "bin", "python.exe"))
            assert os.path.exists(os.path.join(tmp, "bin", "python3.exe"))
            subprocess.check_call([shutil.which("bash.exe"), os.path.join(tmp, "bin", "activate")])

    def test_has_mktime(self):
        from time import mktime, gmtime
        mktime(gmtime())


def suite():
    return unittest.TestLoader().loadTestsFromName(__name__)


if __name__ == '__main__':
    unittest.main(defaultTest='suite')
