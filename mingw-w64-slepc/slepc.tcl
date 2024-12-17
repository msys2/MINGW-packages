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
            testme::with tags {c} {
              xyz::unit SLEPc $tags {
                package require buildme
                buildme::sandbox {
                  buildme::shell "cc -o a ex1.c `pkgconf slepc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && ./a"
                }
              }
            }
            # FORTRAN
            testme::with tags {fortran} {
              xyz::unit SLEPc $tags {
                package require buildme
                buildme::sandbox {
                  buildme::shell "gfortran -o a ex1f.F90 `pkgconf slepc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && ./a"
                }
              }
            }

          }
        }

        # MPI
        testme::with tags mpi {
          testme::with tags c {
            # C
            xyz::unit SLEPc $tags {
              package require buildme
              buildme::sandbox {
                buildme::shell "mpicc -o a ex1.c `pkgconf slepc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && mpiexec -n 1 ./a"
              }
            }
          }
          # FORTRAN
          testme::with tags fortran {
            xyz::unit SLEPc $tags {
              package require buildme
              buildme::sandbox {
                buildme::shell "mpifort -o a ex1f.F90 `pkgconf slepc-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && mpiexec -n 1 ./a"
              }
            }
          }
        }

      }
    }
  }
}
