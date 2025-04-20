package require testme


namespace eval ::xyz {


  proc unit {prefix tags args} {
    testme::unit -tags $tags -name [DescribeXYZ $prefix $tags] -xyz [DeduceXYZ $tags] -static [DeduceStatic $tags] {*}$args
  }


  proc any? {tag tags} {
    foreach t $tag {
      if {[lsearch -exact $tags $t] >= 0} {return 1}
    }
    return 0
  }


  proc DeduceXYZ {tags} {
    if {[any? debug $tags]} {set z g} else {set z o}
    if {[any? mpi $tags]} {set y m} {
      if {[any? {omp openmp} $tags]} {set y t} else {set y s}
    }
    if {[any? complex $tags]} {
      if {[any? single $tags]} {set x c} else {set x z}
    } else {
      if {[any? single $tags]} {set x s} else {set x d}
    }
    return $x$y$z
  }


  proc DescribeXYZ {prefix tags} {
    set prefix "$prefix:"
    if {[any? single $tags]} {set prefix "$prefix single-precision"}
    if {[any? double $tags]} {set prefix "$prefix double-precision"}
    if {[any? complex $tags]} {set prefix "$prefix complex"}
    if {[any? mpi $tags]} {set prefix "$prefix MPI"} else {if {[any? {omp openmp} $tags]} {set prefix "$prefix OpenMP"}}
    if {[any? debug $tags]} {set prefix "$prefix debug"}
    if {[any? c $tags]} {set prefix "$prefix C"}
    if {[any? fortran $tags]} {set prefix "$prefix FORTRAN"}
    if {[any? static $tags]} {set prefix "$prefix static"}
    set prefix "$prefix build"
    return $prefix
  }


  proc DeduceStatic {tags} {
    if {[any? static $tags]} {return {-static}} else {return {}}
  }


}