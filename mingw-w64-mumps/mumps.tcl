tcl::tm::add [file normalize [file join [file dirname [info script]] .]]


package require xyz


# This fixes the issue related to the PkgConf-GFORTRAN interaction.
# Specifically, pkgconf (contrary to the original pkg-config utility) omits system
# directories (/ucrt64/include and alike) from command line options by default
# while gfortran (unlike gcc) maintains no default lookup directories and thus requires it.
set ::env(PKG_CONFIG_ALLOW_SYSTEM_CFLAGS) 1


foreach static {{} static} {
  foreach executor {{} omp} {
    xyz::unit MUMPS [concat $executor $static c double] {
      package require buildme
      buildme::sandbox {
        buildme::shell "cc -o a c_example.c `pkgconf mumps-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && ./a"
      }
    }
  }
  foreach executor {mpi} {
    xyz::unit MUMPS [concat $executor $static c double] {
      package require buildme
      buildme::sandbox {
        buildme::shell "mpicc -o a c_example.c `pkgconf mumps-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && mpiexec -n 1 ./a"
      }
    }
  }
}

foreach precision {double single} {
  foreach scalar {real complex} {
    if {$scalar == "complex"} {set input input_simpletest_cmplx} else {set input input_simpletest_real}
    foreach static {{} static} {
      foreach executor {{} omp} {
        xyz::unit MUMPS [list $executor $static $precision $scalar fortran] -input $input {
          package require buildme
          buildme::sandbox {
            buildme::shell "gfortran -o a [string index [dict get $unit -xyz] 0]simpletest.F `pkgconf mumps-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && ./a < [dict get $unit -input]"
          }
        }
      }
      foreach executor {mpi} {
        xyz::unit MUMPS [list $executor $static $precision $scalar fortran] -input $input {
          package require buildme
          buildme::sandbox {
            buildme::shell "mpifort -o a [string index [dict get $unit -xyz] 0]simpletest.F `pkgconf mumps-[dict get $unit -xyz] --cflags --libs [dict get $unit -static]` [dict get $unit -static] && mpiexec -n 1 ./a < [dict get $unit -input]"
          }
        }
      }
    }
  }
}