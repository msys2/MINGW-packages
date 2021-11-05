/*
 * Copyright (c) 2018 Martin Storsjo
 *
 * This file is part of llvm-mingw.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "native-wrapper.h"

#ifndef DEFAULT_TARGET
#define DEFAULT_TARGET "x86_64-w64-mingw32"
#endif
#ifndef SYSROOT
#define SYSROOT "/clang64"
#endif

int _tmain(int argc, TCHAR* argv[]) {
    const TCHAR *dir;
    const TCHAR *target;
    split_argv(argv[0], &dir, NULL, &target, NULL);
    if (!target)
        target = _T(DEFAULT_TARGET);

    int max_arg = argc + 4;
    const TCHAR **exec_argv = malloc((max_arg + 1) * sizeof(*exec_argv));
    int arg = 0;
    exec_argv[arg++] = concat(dir, _T("llvm-windres"));
    exec_argv[arg++] = _T("--target");
    exec_argv[arg++] = target;
    exec_argv[arg++] = _T("--preprocessor-arg");
    /* something is eating backslashes */
    TCHAR * p = (TCHAR *)dir;
    while ((p = _tcschr(p, _T('\\'))))
        *p = _T('/');
    exec_argv[arg++] = concat(_T("--sysroot="), concat(dir, _T("../..") _T(SYSROOT)));

    for (int i = 1; i < argc; i++)
        exec_argv[arg++] = argv[i];

    exec_argv[arg] = NULL;
    if (arg > max_arg) {
        fprintf(stderr, "Too many options added\n");
        abort();
    }

    return run_final(exec_argv[0], exec_argv);
}
