import argparse
import sys
import sysconfig
from pathlib import Path
from zipfile import ZipFile

def extract_wheel(whl_path, dest):
    print("Installing to", dest)
    with ZipFile(whl_path) as zf:
        zf.extractall(dest)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'wheel',
        type=Path,
        help=f'wheel to install (.whl file)',
    )
    purelib = Path(sysconfig.get_path('purelib')).resolve()
    parser.add_argument(
        '--installdir',
        '-i',
        type=Path,
        default=purelib,
        help=f'installdir directory (defaults to {purelib})',
    )

    args = parser.parse_args()

    if not args.installdir.is_dir():
        sys.exit(f"{args.installdir} is not a directory")

    extract_wheel(args.wheel, args.installdir)
