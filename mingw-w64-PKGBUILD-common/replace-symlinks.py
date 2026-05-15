#!/usr/bin/env python

# SPDX-FileCopyrightText: Hannah von Reth <vonreth@kde.org> and others
# SPDX-License-Identifier: BSD-2-Clause
# Source: https://invent.kde.org/packaging/craft/-/blob/master/bin/utils.py
# Replace symlinks with target file and directories after extracting tarball

import os
import shutil
import sys
from pathlib import Path


def createDir(path):
    """Recursive directory creation function. Makes all intermediate-level directories needed to contain the leaf directory"""
    if not os.path.lexists(path):
        os.makedirs(path)
    return True


def copyFile(
    src: Path,
    dest: Path,
    linkOnly=False,
):
    """copy file from src to dest"""
    src = Path(src)
    dest = Path(dest)
    if dest.is_dir():
        dest = dest / src.name
    else:
        createDir(dest.parent)
    if os.path.lexists(dest):
        print(f"Warning: Overriding:\t{dest} with\n\t\t{src}")
        if src == dest:
            print(f"Error: Can't copy a file into itself {src}=={dest}")
            return False
        OsUtils.rm(dest, True)
    # don't link to links
    if linkOnly and not os.path.islink(src):
        try:
            os.link(src, dest)
            return True
        except:
            print("Warning: Failed to create hardlink %s for %s" % (dest, src))
    try:
        shutil.copy2(src, dest, follow_symlinks=False)
    except PermissionError as e:
        if CraftCore.compiler.isWindows and dest.exists():
            return deleteFile(dest) and copyFile(src, dest)
        else:
            raise e
    except Exception as e:
        print(f"Error: Failed to copy file:\n{src} to\n{dest}", exc_info=e)
        return False
    return True


def copyDir(
    srcdir,
    destdir,
    linkOnly=False,
    copiedFiles=None,
):
    """copy directory from srcdir to destdir"""

    srcdir = Path(srcdir)
    if not srcdir.exists():
        print(f"Warning: copyDir called. srcdir: {srcdir} does not exists")
        return False
    destdir = Path(destdir)
    if not destdir.exists():
        createDir(destdir)

    try:
        with os.scandir(srcdir) as scan:
            for entry in scan:
                dest = (
                    destdir / Path(entry.path).parent.relative_to(srcdir) / entry.name
                )
                if entry.is_dir():
                    if entry.is_symlink():
                        # copy the symlinks without resolving them
                        if not copyFile(entry.path, dest, linkOnly=False):
                            return False
                        if copiedFiles is not None:
                            copiedFiles.append(str(dest))
                    else:
                        if not copyDir(
                            entry.path, dest, copiedFiles=copiedFiles, linkOnly=linkOnly
                        ):
                            return False
                else:
                    # symlinks to files are included in `files`
                    if not copyFile(entry.path, dest, linkOnly=linkOnly):
                        return False
                    if copiedFiles is not None:
                        copiedFiles.append(str(dest))
    except Exception as e:
        print(f"Error: Failed to copy dir:\n{srcdir} to\n{destdir}", exc_info=e)
        return False
    return True


def replaceSymlinksWithCopies(path, _replaceDirs=False):
    def resolveLink(path):
        while os.path.islink(path):
            toReplace = os.readlink(path)
            if not os.path.isabs(toReplace):
                path = os.path.join(os.path.dirname(path), toReplace)
            else:
                path = toReplace
        return path

    # symlinks to dirs are resolved after we resolved the files
    dirsToResolve = []
    ok = True
    for root, _, files in os.walk(path):
        for svg in files:
            if not ok:
                return False
            path = os.path.join(root, svg)
            if os.path.islink(path):
                toReplace = resolveLink(path)
                if not os.path.exists(toReplace):
                    print(
                        f"Error: Resolving {path} failed: {toReplace} does not exists."
                    )
                    continue
                if toReplace != path:
                    if os.path.isdir(toReplace):
                        if not _replaceDirs:
                            dirsToResolve.append(path)
                        else:
                            os.unlink(path)
                            ok = copyDir(toReplace, path)
                    else:
                        os.unlink(path)
                        ok = copyFile(toReplace, path)
    while dirsToResolve:
        d = dirsToResolve.pop()
        if not os.path.exists(resolveLink(d)):
            print(f"Warning: Delay replacement of {d}")
            dirsToResolve.append(d)
            continue
        if not replaceSymlinksWithCopies(os.path.dirname(d), _replaceDirs=True):
            return False
    return True


replaceSymlinksWithCopies(sys.argv[1])
