#!/bin/bash

set -e

# Extract & adapt parts from MS-MPI SDK & runtime
# MS-MPI 10.1.1

source=(
  'https://download.microsoft.com/download/2/9/e/29efe9b1-16d7-4912-a229-6734b0c4e235/msmpisdk.msi'
  'https://download.microsoft.com/download/2/9/e/29efe9b1-16d7-4912-a229-6734b0c4e235/msmpisetup.exe'
)
sha256sums=(
  '6df47378c0acbb2b76989092165d7279ee096605fcaf7e455de733b2ffdf4bcc'
  '7308fd15e437d6829fd9c6ec5d801380faa3bff6e66e1dee3e8e005106fb6a68'
)

if [[ $MSYSTEM_CARCH != "x86_64" ]]; then 
	echo "ERROR: this scripts requires 64-bit MSYS2 environment"
	exit 1
fi

mkdir -p x86_64/lib i686/lib include x86_64/include i686/include

# x64
msmpi=`cygpath -m "$WINDIR/system32/msmpi.dll"`
if [[ ! -f "$msmpi" ]]; then
	echo "ERROR: 64-bit msmpi.dll is missing. Is MS-MPI runtime installed?"
	exit 1
fi
/mingw64/bin/gendef "$msmpi" 2> /dev/null
cp msmpi.def x86_64/lib

# x32
msmpi=`cygpath -m "$WINDIR/syswow64/msmpi.dll"`
if [[ ! -f "$msmpi" ]]; then
	echo "ERROR: 32-bit msmpi.dll is missing. Is MS-MPI runtime installed?"
	exit 1
fi
/mingw32/bin/gendef "$msmpi" 2> /dev/null
cp msmpi.def i686/lib
ruby -n -e '$_.strip!; puts "#{$1}@0=#{$1}\n" if ($_ != $_.upcase) && ($_ != $_.downcase) && /^([a-zA-Z0-9_]*)$/.match($_)' msmpi.def >> i686/lib/msmpi.def

rootdir=`pwd`

(
	cd "`cygpath -m "${MSMPI_INC}"`"
	if [[ ! -f mpi.h ]]; then
		echo "ERROR: \$MSMPI_INC directory does not exist or contains no mpi.h file. Is MS-MPI SDK installed?"
		exit 1
	fi
	cp mpi.h mpif.h mpi.f90 "$rootdir/include"
	cp x64/mpifptr.h "$rootdir/x86_64/include"
	cp x86/mpifptr.h "$rootdir/i686/include"
)

# Export file signatures to be embed into PKGBUILD in order to ensure build integrity
#ruby export.rb include/{mpi.h,mpif.h,mpi.f90} {x86_64,i686}/include/mpifptr.h {x86_64,i686}/lib/msmpi.def
