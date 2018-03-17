#!/bin/bash

shopt -s extglob
set +f


echo "=== Performing PETSc test builds ==="
echo

total=0
passed=0

pkg=petsc
exdir=src/build-${MINGW_CHOST}/petsc-*/src/snes/examples/tutorials

for build in dso dmo zso zmo; do
	for lang in c f; do
		for link in static dynamic; do
			cflags=""
			libs=""
			pcflags="$pkg-$build"
			case $lang in
				c)
					src=$exdir/ex3.c
				;;
				f)
					src=$exdir/ex5f.F90
					#cflags="$cflags -fno-range-check"
				;;
				*)
				exit 1
				;;
			esac
			case $build in
				?m?)
					comp=mpicc; [[ $lang = f ]] && comp=mpif90
				;;
				?s?)
					comp=gcc; [[ $lang = f ]] && comp=gfortran
				;;
				*)
				exit 1
				;;
			esac
			case $link in
				static)
					cflags="-static $cflags"
					pcflags="$pcflags --static"
				;;
				dynamic)
				;;
				*)
				exit 1
				;;
			esac
			cflags="$cflags `pkg-config $pcflags --cflags`"
			libs="`pkg-config $pcflags --libs` $libs"
			exe="$lang-$link-$build.exe"
			cmd="$comp -o $exe $cflags $src $libs"
			rm -rf $exe
			echo $cmd
			{ $cmd; }
			if [[ $? = 0 ]]; then
				let passed+=1
			else
				echo "*** FALIED"
			fi
			echo
			let total+=1
		done
	done
done

if [[ $total = $passed ]]; then
	echo "=== All $total builds were successful ==="
	exit 0
else
	echo "*** Passed $passed of $total builds ***"
	exit 1
fi

#