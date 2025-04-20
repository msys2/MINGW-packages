tcl::tm::add [file normalize [file join [file dirname [info script]] .]]


package require xyz


foreach scalar {real complex} {
  foreach precision {double single} {
    foreach static {{} static} {
      testme::with tags [list $scalar $precision $static] {

        # Single-process
        foreach executor {{} omp} {
          testme::with tags $executor {
            # C
            testme::with tags {c ksp} {
              xyz::unit PETSc $tags {
                package require buildme
                buildme::sandbox {
                  buildme::shell "cc -o a ksp/ex1.c `pkgconf petsc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && ./a"
                }
              }
            }
            # FORTRAN
            testme::with tags {fortran ksp} {
              xyz::unit PETSc $tags {
                package require buildme
                buildme::sandbox {
                  buildme::shell "gfortran -o a ksp/ex1f.F90 `pkgconf petsc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && ./a"
                }
              }
            }

          }
        }

        # MPI
        testme::with tags mpi {
          testme::with tags c {
            # C
            xyz::unit PETSc [concat $tags ksp] {
              package require buildme
              buildme::sandbox {
                buildme::shell "mpicc -o a ksp/ex2.c `pkgconf petsc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && mpiexec -n 1 ./a"
              }
            }
            xyz::unit PETSc+Parmetis [concat $tags mat parmetis] {
              package require buildme
              buildme::sandbox {
                buildme::shell "mpicc -o a mat/ex15.c `pkgconf petsc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && mpiexec -n 1 ./a -mat_partitioning_type parmetis -N 100"
              }
            }
          }
          # FORTRAN
          testme::with tags fortran {
            xyz::unit PETSc [concat $tags ksp] {
              package require buildme
              buildme::sandbox {
                buildme::shell "mpifort -o a ksp/ex2f.F90 `pkgconf petsc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && mpiexec -n 1 ./a"
              }
            }
            xyz::unit PETSc+Parmetis [concat $tags mat parmetis] {
              package require buildme
              buildme::sandbox {
                buildme::shell "mpifort -o a mat/ex15f.F90 `pkgconf petsc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && mpiexec -n 1 ./a -mat_partitioning_type parmetis -N 100"
              }
            }
          }
        }

      }
    }
  }
}
