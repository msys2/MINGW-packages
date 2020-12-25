

export PATH=/mingw64/bin:$PATH

env

uname -a

cd ice40
cmake -G "MSYS Makefiles" -DICESTORM_INSTALL_PREFIX="$1" .
make
cd ..

cd ecp5
cmake -G "MSYS Makefiles" -DTRELLIS_INSTALL_PREFIX="$1" .
make
cd ..
