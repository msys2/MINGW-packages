from urllib.request import urlopen
from json import loads
from pathlib import Path
from subprocess import check_call
from sys import stdout

PKGEXT='.pkg.tar.zst'
SRCEXT='.src.tar.gz'

ROOT = Path(__file__).resolve().parent

def test_file(pkg):
    print('\n::group::Package %s...' % pkg)
    stdout.flush()

    def run_cmd(args):
        check_call(['bash'] + args, cwd=str(ROOT/pkg))

    try:
        run_cmd(['makepkg-mingw', '--noconfirm', '--noprogressbar', '--skippgpcheck', '--nocheck', '--syncdeps', '--rmdeps', '--cleanbuild'])
        run_cmd(['makepkg', '--noconfirm', '--noprogressbar', '--skippgpcheck', '--allsource', '--config', '/etc/makepkg_mingw64.conf'])
        run_cmd(['mv', '*%s' % PKGEXT, '*%s' % SRCEXT, '../artifacts/'])
        print('::endgroup::')
        stdout.flush()
        return 0
    except:
        print('::endgroup::')
        stdout.flush()
        return 1

def pytest_generate_tests(metafunc):
    pkgs = [
        pkg['repo_path']
        for pkg in loads(urlopen("https://packages.msys2.org/api/buildqueue").read())
        if pkg['repo_url'] == 'https://github.com/msys2/MINGW-packages'
    ]
    metafunc.parametrize("pkg", pkgs)
