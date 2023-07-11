project('msys2-pathtools', 'c', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'buildtype=debugoptimized',
  ]
)

test('main', executable('main', 'main.c', 'msys2_relocate.c', 'pathtools.c'))
