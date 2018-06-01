#/bin/sh
# generates the patch to replace __{in,out} with ___{in,out} in the headers
wget https://ftp.gnu.org/gnu/gcc/gcc-8.1.0/gcc-8.1.0.tar.gz
tar xzvf gcc-8.1.0.tar.gz
cp -Rf gcc-8.1.0 gcc-8.1.0-orig
cd gcc-8.1.0
sed -ri 's/\b(__in|__out)\b/_&/g' $(egrep -rl '\b(__in|__out)\b' libstdc++-v3/{include,config})
cd ..
diff -aurp gcc-8.1.0-orig gcc-8.1.0 > 0130-libstdc++-in-out.patch
