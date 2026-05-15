# https://github.com/okhlybov/testme


package require Tcl


namespace eval ::buildme {
  

  namespace export sandbox shell


  proc MakeTempDir {args} {
    upvar 1 unit unit
    set roots $args
    foreach t {TMPDIR TMP} {
      if {![catch {set t [set ::env($t)]}]} {
        lappend roots $t
      }
    }
    lappend roots /tmp
    foreach r $roots {
      if {![catch {
        set prefix [file rootname [file tail [dict get $unit -source]]]
        set t [file join $r $prefix.[expr {int(rand()*999999)}]]
        file mkdir $t
      }]} {return $t}
    }
    error "failed to create temporary directory $t"
  }


  proc sandbox {code} {
    upvar 1 unit unit
    set dir [MakeTempDir]
    try {
      foreach p [glob -nocomplain -directory [dict get $unit -stage] * .*] {
        set last [lindex [file split $p] end]
        if {$last != "." && $last != ".."} {
          file copy -force -- $p $dir
        }
      }
      # Can't cd into stage dir in multithreaded environment where all threads have the same current directory
      dict set unit -stage $dir
      eval $code
    } finally {
      if {[dict get $unit -cleanup]} {
        if {[catch {file delete -force -- $dir}]} {
          puts stderr "failed to delete temporary directory $dir
        }
      }
    }
  }


  proc shell {cmd} {
    upvar 1 unit unit
    set stage [dict get $unit -stage]
    set cd "cd \"$stage\""
    puts stdout "$cd && \\\n$cmd"
    if {[catch {exec -ignorestderr $::env(SHELL) -c "$cd && $cmd" 2>@1} result opts]} {
      puts stderr $result
    } else {
      puts stdout $result
    }
    return {*}$opts $result
  }


}