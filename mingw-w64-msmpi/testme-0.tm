# https://github.com/okhlybov/testme


package require Tcl


package require Thread


namespace eval ::testme {


  namespace export unit


  proc Import {source} {
    variable executor
    interp create box
    try {
      interp alias box ::testme::unit {} ::testme::unit
      box eval set ::argv0 [file normalize $source]
      box eval set ::nesting [incr $::nesting -1]
      box eval {
        cd [file dirname $::argv0]
        source [file tail $::argv0]
      }
    } finally {
      interp delete box
    }
  }


  ### Toplevel code


  try {set ::nesting} on error {} {


    package require platform


    try {


      ### CLIP code


      # package require Tcl 8.6


      namespace eval ::clip {


        # defs: { { opt(s) } -slot opt -default value -info "text" -section {common} -apply {script} }
        proc parse {argv defs} {
          set x [lsearch $argv --]
          if {$x < 0} {
            set opts $argv
            set args {}
          } else {
            set opts [lrange $argv 0 [expr {$x-1}]]
            set args [lrange $argv [expr {$x+1}] end]
          }
          set xargs {}
          lassign [NormalizeDefs $defs] short_flags short_opts long_flags long_opts
          set flagset [dict create]
          foreach f $short_flags {
            dict set flagset [dict get $f bare] $f
          }
          while {[llength $opts]} {
            set x [Next opts]
            # long option with inline value, ex. --abc=123
            if {[regexp {^-+([[:alnum:]][[:alnum:]\-\_]*)=(.*)$} $x ~ tag value]} {
              set found 0
              foreach d $long_opts {
                dict with d {
                  if {$tag == $bare} {
                    set found 1
                    break
                  }
                }
              }
              if {$found} {
                apply $apply $slot $value
                continue
              } else {
                error "unrecognized option in $x"
              }
            }
            # long/short option expecting separate value, ex. --abc 123
            if {[regexp {^-+([[:alnum:]][[:alnum:]\-\_]*)} $x ~ tag]} {
              set found 0
              foreach d [concat $long_opts $short_opts] {
                dict with d {
                  if {$tag == $bare} {
                    set value [Next opts]
                    set found 1
                    break
                  }
                }
              }
              if {$found} {
                apply $apply $slot $value
                continue
              }
            }
            # long/short separate flag, ex. -C,  --create
            if {[regexp {^-+([[:alnum:]][[:alnum:]\-\_]*)} $x ~ tag]} {
              set found 0
              foreach d [concat $long_flags $short_flags] {
                dict with d {
                  if {$tag == $bare} {
                    set value $default
                    set found 1
                    break
                  }
                }
              }
              if {$found} {
                apply $apply $slot $value
                continue
              }
            }
            # short flag within the coalesced flag set, ex. -Sxy
            if {[regexp {^-([[:alnum:]]+)$} $x ~ flags]} {
              foreach flag [split $flags {}] {
                try {
                  set d [dict get $flagset $flag]
                  dict with d {
                    apply $apply $slot $default
                  }
                } on error {} {
                  error "unrecognized short flag -$flag"
                }
              }
              continue
            }
            # stray -flag
            if {[regexp {^-.*} $x]} {
              error "unrecognized argument $x"
            }
            # the rest arguments are passed through
            lappend xargs $x
          }
          concat $xargs $args
        }


        #
        proc usage {defs {sections {}} {chan stdout}} {
          set padding [ComputePadding $defs]
          dict for {section info} $sections {
            PrintSection [ExtractSectionDefs defs $section] $info $chan $padding
          }
          PrintSection $defs "Generic options" $chan $padding
        }


        proc ComputePadding {defs} {
          set pad 0
          foreach def $defs {
            set x [string length [FormatOpts [Opts $def]]]
            if {$x > $pad} { set pad $x }
          }
          return $pad
        }


        proc ExtractSectionDefs {defsVar section} {
          upvar $defsVar defs
          set rest [list]
          set filtered [list]
          foreach def $defs {
            if {[Section $def] == $section} {
              lappend filtered $def
            } else {
              lappend rest $def
            }
          }
          set defs $rest
          return $filtered
        }


        proc FormatOpts {opts} {
          set arg 0
          set x [lmap opt $opts {
            regexp {(.*?)(=*)$} $opt ~ bare flag
            if {!$arg && $flag != {}} { set arg 1 }
            subst $bare
          }]
          set x [join $x {, }]
          if {$arg} { set x "$x <arg>" }
          return $x
        }


        proc PrintSection {defs info chan padding} {
          if {[llength $defs]} {
            incr padding 2
            if {$info != {}} { puts $chan "\n* $info:\n" }
            foreach def $defs {
              set opts [format %${padding}s [FormatOpts [Opts $def]]]
              puts $chan "$opts    [Info $def]"
            }
          }
        }


        proc Opts {def} { lindex $def 0 }


        proc Dict {def} { lrange $def 1 end }

        
        proc Default {def} {
          try { return [dict get [Dict $def] -default] } on error {} { return 1 }
        }
        
        
        proc Section {def} {
          try { return [dict get [Dict $def] -section] } on error {} { return {} }
        }


        proc Info {def} {
          try { return [dict get [Dict $def] -info] } on error {} { return {} }
        }


        proc Slot {def} {
          try { return [dict get [Dict $def] -slot] } on error {} { return {} }
        }


        proc Apply {def} {
          try { return [list {slot value} [dict get [Dict $def] -apply]] } on error {} { return {{slot value} { upvar 2 $slot x; set x $value }} }
        }


        proc Next {listVar} {
          upvar $listVar list
          if {![llength $list]} { error "not enough arguments" }
          set v [lindex $list 0]
          set list [lrange $list 1 end]
          return $v
        }


        # Flags are the parameterless options which receive 1 value when set, ex. -S
        # Single letter flags may be coalesced, ex. -Sxyz
        # Multi letter flags must come on their own, ex. -foo
        proc NormalizeDefs {defs} {
          set short_flags {}
          set short_opts {}
          set long_flags {}
          set long_opts {}
          foreach def $defs {
            set slot [Slot $def]
            foreach opt [Opts $def] {
              regexp -- {-+(.*?)=?} $opt ~ bare
              if {$slot == {}} { set slot $bare }
              switch -regexp $opt {
                {^-[[:alnum:]]$} { set kind short_flags }
                {^-+[[:alnum:]][[:alnum:]\-\_]*$} { set kind long_flags }
                {^-[[:alnum:]]=$} { set kind short_opts }
                {^-+[[:alnum:]][[:alnum:]\-\_]*=$} { set kind long_opts }
                default { error "failed to decode option descriptor $opt" }
              }
              lappend $kind [dict create bare $bare slot $slot default [Default $def] apply [Apply $def] info [Info $def] section [Section $def]]
            }
          }
          return [list $short_flags $short_opts $long_flags $long_opts]
        }


      }


      variable verbose false


      variable quiet false


      variable staging false


      variable premature false


      variable cleanup true

      
      variable jobs 0


      set opts {
        {{--jobs= -j=} -info "set maximum number of allowed threads" -default 0 -slot jobs}
        {{-v --verbose} -info "dump unit output to standard error channel" -default true -slot verbose}
        {{-T --staging} -info "manage staging directory in \$TMPDIR" -default true -slot staging}
        {{-K --keep} -info "keep temporary directories & files" -default false -slot cleanup}
        {{-e --bailout} -info "bail out on first failure" -default true -slot premature}
        {{-q --quiet} -info "suppress TAP output to standard output channel" -default true -slot quiet}
        {{-h --help} -info "print help" -apply {
          puts stderr "usage: $::argv0 {-f --flag --opt=arg --opt arg -opt arg ...} {--} {tag +tag -tag ...}"
          puts stderr {}
          puts stderr "    +tag | tag    instruct to execute only units with specified tag(s)"
          puts stderr "    -tag          instruct to skip units with specified tag(s)"
          clip::usage $testme::opts {} stderr
          exit 0
        }}
        {{--version} -info "print package version" -apply {
          puts stderr [package require testme]
          exit 0
        }}
      }


      if {[catch {
        if {[llength $::argv]} {set ::argv [clip::parse $::argv $opts]}
      } return]} {
        puts stderr $return
        exit 1
      }


      # TODO
      if {$jobs == 0} {
        set jobs 4
        switch -glob [platform::identify] {
          linux-* {catch {set jobs [exec nproc --all]}}
          win32-* {catch {set jobs $env(NUMBER_OF_PROCESSORS)}}
        }
      }


      set +tags [list]
      set -tags [list]


      foreach arg $::argv {
        switch -glob $arg {
          +* {lappend +tags [string trimleft $arg +]}
          -* {lappend -tags [string trimleft $arg -]}
          default {lappend +tags $arg}
        }
      }


      set ::nesting -1


      set paths "eval tcl::tm::path add [tcl::tm::path list]"


      set static {


        ### Execution thread code


        proc lshift {var} {
          upvar 1 $var x
          set r [lindex $x 0]
          set x [lrange $x 1 end]
          return $r
        }


        proc lfront {list} {
          return [lindex $list 0]
        }


        rename puts ::tcl::puts


        proc puts {args} {
          variable stdout
          variable stderr
          set xargs $args
          if {[llength $args] > 1 && [lfront $args] == "-nonewline"} {
            lshift args
            set newline 0
          } else {
            set newline 1
          }
          switch [llength $args] {
            1 {set chan stdout}
            default {set chan [lshift args]}
          }
          if {$chan == "stdout" || $chan == "stderr"} {
            if {$newline} {
              foreach arg $args {lappend $chan $arg}
            } else {
              set $chan [concat [lrange [set $chan] 0 end-1] "[lindex [set $chan] end]$args"]
            }
          } else {
            tcl::puts {*}$xargs
          }
        }


        proc skip {{reason {}}} {return -code 1073741823 -level 0 $reason}


        proc process-unit {unit} {
          variable stdout [list]
          variable stderr [list]
          interp create unit
          try {
            interp alias unit puts {} puts
            interp alias unit skip {} skip
            unit eval tcl::tm::path add {*}[tcl::tm::path list]
            catch {unit eval "apply {{unit} {[dict get $unit -code]}} {$unit}"} return opts
            return [dict merge $opts [dict create -return $return -stdout $stdout -stderr $stderr -id [dict get $unit -id]]]
          } finally {
            interp delete unit
          }
        }


      }


      variable executor [tpool::create -maxworkers $jobs -initcmd "$paths; $static;"]


      variable pending [list]


      variable skipped [list]


      variable units [dict create]


      variable id 1


      proc union {as bs} {
        set result $as
        foreach elem $bs {
          if {[lsearch -exact $as $elem] == -1} {
            lappend result $elem
          }
        }
        return $result
      }


      proc intersection {as bs} {
        set result {}
        foreach elem $bs {
          if {[lsearch -exact $as $elem] >= 0} {
            lappend result $elem
          }
        }
        return $result
      }


      proc unit {args} {
        variable executor
        variable pending
        variable skipped
        variable units
        variable +tags
        variable -tags
        variable cleanup
        variable id
        set s [llength $args]
        if {$s < 1 || $s % 2 == 0} {error "usage: testme::unit ?-name ...? ?-tags ...? {...}"}
        set opts [lrange $args 0 end-1]
        set code [lindex [lrange $args end end] 0]
        set name unit[llength $pending]
        set tags [list]
        catch {set name [dict get $opts -name]}
        catch {set tags [dict get $opts -tags]}
        set tags [lsearch -inline -all -not -exact $tags {}]; # Squeeze out empty {} tags
        set unit [dict create {*}$opts -stage [pwd] -name $name -tags $tags -code $code -id $id -source $::argv0 -cleanup $cleanup]
        dict set units $id $unit
        if {([llength ${+tags}] == 0 || [llength [intersection ${+tags} $tags]] > 0) && [llength [intersection ${-tags} $tags]] == 0} {
          lappend pending [tpool::post $executor "process-unit {$unit}"]
        } else {
          lappend skipped $id
        }
        incr id
      }


      if {$staging} {


        proc MakeTempDir {args} {
          set roots $args
          foreach t {TMPDIR TMP} {
            if {![catch {set t [set ::env($t)]}]} {
              lappend roots $t
            }
          }
          lappend roots /tmp
          foreach r $roots {
            if {![catch {
              set t [file join $r [expr {int(rand()*999999)}]]
              file mkdir $t
            }]} {
              return $t
            }
          }
          error "failed to create temporary directory $t"
        }  


        set staging [set ::env(TMPDIR) [MakeTempDir]]


        if {$verbose} {puts stderr "created temporary directory $staging"}

      }


      try {


        Import $::argv0


        if {!$quiet} {
          puts "TAP version 14"
          puts "1..[dict size $units]"
          foreach s $skipped {
            set u [dict get $units $s]
            puts "ok [dict get $u -id] - [dict get $u -name] # SKIP due to tagging"
          }
        }


        while {[llength $pending]} {
          foreach f [tpool::wait $executor $pending pending] {
            set pending [lsearch -inline -all -not -exact $pending $f]
            set return [tpool::get $executor $f]
            set u [dict get $units [dict get $return -id]]
            set name [dict get $u -name]
            set id [dict get $u -id]
            if {!$quiet} {
              switch [dict get $return -code] {
                0 {puts "ok $id - $name"}
                1073741823 {puts "ok $id - $name # SKIP [dict get $return -return]"}
                default {
                  puts "not ok $id - $name"
                  puts "  ---"
                  set lines [split [dict get $return -return] "\n"]
                  set r [lindex $lines 0]
                  if {[llength $lines] > 1} {set r "$r >>>"}
                  puts "  tags: [dict get $u -tags]"
                  puts "  source: [dict get $u -source]"
                  puts "  return: $r"
                  puts "  ..."
                }
              }
            }
            if {$verbose} {
              set stdout [dict get $return -stdout]
              set stderr [dict get $return -stderr]
              if {[llength $stdout] + [llength $stderr] > 0} {
                puts stderr {}
                puts stderr "- $name"
                if {[llength $stdout] > 0} {
                  puts stderr "-- stdout:"
                  foreach line $stdout {puts stderr $line}
                }
                if {[llength $stderr] > 0} {
                  puts stderr "-- stderr:"
                  foreach line $stderr {puts stderr $line}
                }
              }
            }
            flush stdout
            flush stderr
            if {$premature && [dict get $return -code] != 0} {error "bailing out on failure"}
          }
        }


      } finally {


        if {$cleanup && $staging != {false}} {
          if {[catch {file delete -force -- $staging}]} {
            if {$verbose} {puts stderr "failed to remove temporary directory $staging"}
          } else {
            if {$verbose} {puts stderr "removed temporary directory $staging"}
          }
        }


      }


      exit 0


    } on error {return opts} {
      if {$verbose} {
        puts stderr $return
        puts stderr [dict get $opts -errorinfo]
      }
      if {!$quiet} {puts "Bail out!"}
      exit 1
    }


  }


  ### Nested code


  if {$::nesting != 0} {
    set wd [pwd]
    foreach source [glob -nocomplain [file join * [file tail $::argv0]]] {
      try {Import $source} finally {cd $wd}
    }
  }


  ### Custom code which slurped the testme package's code
  
  
  proc with {var list code} {
    upvar 1 $var v
    if {[catch {set v}]} {set v [list]}
    set list [concat $v $list]
    apply [list $var $code] $list
  }


}