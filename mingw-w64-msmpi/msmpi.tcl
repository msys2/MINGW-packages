tcl::tm::add [file normalize [file join [file dirname [info script]] .]]


package require testme


# These tests check the MPI compiler driver mpi{c,c++,fc} functionality

testme::unit -name "MPI C compiler driver test" -tags {mpi c} {
	package require buildme
	buildme::sandbox {
		buildme::shell "mpicc -o a hello_mpi.c && mpiexec -np 1 ./a"
	}
}

testme::unit -name "MPI C++ compiler driver test" -tags {mpi c++} {
	package require buildme
	buildme::sandbox {
		buildme::shell "mpicxx -o a hello_mpi.cpp && mpiexec -np 1 ./a"
	}
}

testme::unit -name "MPI FORTRAN77 compiler driver test" -tags {mpi fortran} {
	package require buildme
	buildme::sandbox {
		buildme::shell "mpif77 -o a hello_mpi.F && mpiexec -np 1 ./a"
	}
}

testme::unit -name "MPI FORTRAN90 compiler driver test" -tags {mpi fortran} {
	package require buildme
	buildme::sandbox {
		buildme::shell "mpif90 -o a hello_mpi.f90 && mpiexec -np 1 ./a"
	}
}

# These tests check the MPI's PkgConfig module to be used as a dependency

# This fixes the issue related to the PkgConf-GFORTRAN interaction.
# Specifically, pkgconf (contrary to the original pkg-config utility) omits system
# directories (/ucrt64/include and alike) from command line options by default
# while gfortran (unlike gcc) maintains no default lookup directories and thus requires it.
set ::env(PKG_CONFIG_ALLOW_SYSTEM_CFLAGS) 1

testme::unit -name "MPI C dependency test" -tags {mpi c} {
	package require buildme
	buildme::sandbox {
		buildme::shell "cc -o a hello_mpi.c `pkgconf msmpi --cflags --libs` && mpiexec -np 1 ./a"
	}
}

testme::unit -name "MPI C++ dependency test" -tags {mpi c++} {
	package require buildme
	buildme::sandbox {
		buildme::shell "c++ -o a hello_mpi.cpp `pkgconf msmpi --cflags --libs` && mpiexec -np 1 ./a"
	}
}

testme::unit -name "MPI FORTRAN77 dependency test" -tags {mpi fortran} {
	package require buildme
	buildme::sandbox {
		buildme::shell "gfortran -o a hello_mpi.F `pkgconf msmpi --cflags --libs` && mpiexec -np 1 ./a"
	}
}

testme::unit -name "MPI FORTRAN90 dependency test" -tags {mpi fortran} {
	package require buildme
	buildme::sandbox {
		buildme::shell "gfortran -o a hello_mpi.f90 `pkgconf msmpi --cflags --libs` && mpiexec -np 1 ./a"
	}
}