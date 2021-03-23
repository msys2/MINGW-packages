#
# TclTest extenstion to test source code compilation & running
#
# https://github.com/okhlybov/tclbuildtest
#

package provide tclbuildtest 0.1.0

package require Tcl 8.6
package require tcltest 2.5.1

namespace eval ::tcltest {}; # Make pkg_mkIndex happy

# https://github.com/tcl2020/named-parameters

namespace eval ::np {
	#
	# proc_args_to_dict - given a proc name and declared
	#   proc arguments (variable names with optional
	#   default values and a -- and possible some more
	#   stuff, create and return a dict containing that
	#   info in a way that's convenient and quicker
	#   for us at runtime:
	#   * we store a list of positional parameters
	#   * we store a list of named parameters
	#   * we store a list of var-value defaults
	#   * we get a tricked-out error message in errmsg
	#
	::proc proc_args_to_dict {name procArgs} {
		set seenDashes 0
		dict set d defaults [list]
		dict set d positional [list]
		dict set d named [list]
		set errmsg "wrong # args: should be \"$name "

		foreach arg $procArgs {
			if {$arg eq "--"} {
				set seenDashes 1
				append errmsg "?--? "
				continue
			}

			set var [lindex $arg 0]
			dict lappend d [expr {$seenDashes ? "positional" : "named"}] $var

			if {[llength $arg] == 2} {
				dict lappend d defaults $var [lindex $arg 1]
				if {$seenDashes} {
					append errmsg "?$var? "
				} else {
					append errmsg "?-$var val? "
				}
			} elseif {$var eq "args"} {
				dict lappend d defaults $var [list]
				append errmsg "?arg ...? "
			} else {
				if {$seenDashes} {
					append errmsg "$var "
				} else {
					append errmsg "-$var val "
				}
			}
		}
		dict set d errmsg "[string range $errmsg 0 end-1]\""
		return $d
	}

	#
	# np_handler - look at an argument dict created by proc_args_to_dict
	#   and look at the real arguments to a function (args), and sort
	#   out the named and positional parameters to behave in the
	#   expected way.
	#
	::proc np_handler {argd realArgs} {
		set named [dict get $argd named]
		set positional [dict get $argd positional]

		# process named parameters
		while {[llength $realArgs] > 0} {
			set arg [lindex $realArgs 0]

			# if arg is --, flip to positional
			if {$arg eq "--"} {
				set realArgs [lrange $realArgs 1 end]
				break
			}

			# if "var" doesn't start with a dash, flip to positional
			if {[string index $arg 0] ne "-"} {
				#puts "possible var '$arg' doesn't start with a dash, flip to positional"
				break
			}

			# if "var" isn't known to us as a named parameter, flip to positional
			set var [string range $arg 1 end]
			if {[lsearch $named $var] < 0} {
				#puts "'var' '$arg' not recognized, flip to positional"
				break
			}

			# if there isn't at least one more element in the arg list,
			# we are missing a value for one of our named parameters
			if {[llength $realArgs] == 0} {
				#puts "realArgs is empty but i expect something for $var"
				error [dict get $argd errmsg] "" [list TCL WRONGARGS]
			}

			# we're good, set the named parameter into the variable sets
			#puts [list set vsets($var) [lindex $realArgs 1]]

			# but don't allow the same variable to be set twice
			if {[info exists vsets($var)]} {
				error [dict get $argd errmsg] "" [list TCL WRONGARGS]
			}

			set vsets($var) [lindex $realArgs 1]
			set realArgs [lrange $realArgs 2 end]
		}

		# fill in defaults for all the vars with defaults that
		# didn't get set to a value
		foreach "var value" [dict get $argd defaults] {
			if {![info exists vsets($var)]} {
				set vsets($var) $value
			}
		}

		foreach var $positional {
			if {$var eq "args"} {
				set vsets($var) $realArgs
				set realArgs [list]
				break
			}

			if {[llength $realArgs] > 0} {
				set vsets($var) [lindex $realArgs 0]
				set realArgs [lrange $realArgs 1 end]
			}

			# no arguments left.  if this var doesn't
			# have a default value, it's a wrong args error
			if {![info exists vsets($var)]} {
				error [dict get $argd errmsg] "" [list TCL WRONGARGS]
			}
		}

		# make sure all the named parameters have been set, either
		# by defaults or explicitly, any not set is an error
		foreach var $named {
			if {![info exists vsets($var)]} {
				#puts "required named parameter '-$var' is not set"
				error [dict get $argd errmsg] "" [list TCL WRONGARGS]
			}
		}

		# are there too many arguments?
		if {[llength $realArgs] > 0} {
			#puts "leftover arguments (too many) '$realArgs'"
			error [dict get $argd errmsg] "" [list TCL WRONGARGS]
		}

		# now iterate through the var-value pairs and set them into
		# the caller's frame
		foreach "var value" [array get vsets] {
			#puts "set '$var' '$value'"
			upvar $var myvar
			set myvar $value
		}
		return
	}

	#
	# np::proc - same as proc except if -- is in the argv
	#   then it will generate a proc that has extra code
	#   at the beginning to wrangle the named parameters
	#
	proc proc {name argv body} {
		# handle the case where there are no named parameters
		if {[lsearch $argv --] < 0} {
			uplevel [list ::proc $name $argv $body]
			return
		}

		if {[lsearch $argv --] == 0} {
			return -code error "-- cannot be the first argument for named parameters. Use positional parameters."
		}

		set d [proc_args_to_dict $name $argv]
		set newbody "::proc $name args {\n"
		append newbody "    ::np::np_handler [list $d] \$args\n"
		append newbody $body
		append newbody "\n}"
		#puts $newbody
		uplevel $newbody
	}
}

namespace eval ::tclbuildtest {
	
	variable system-count 0
	variable build-count 0

	# Standard predefined constraints
	foreach ct {
		c cxx fortran
		single double
		real complex
		openmp thread hybrid mpi
		static
		debug
	} {::tcltest::testConstraint $ct 1}

	# Return a value of the environment variable or {} if no such variable is set
	proc env {var} {
		try {return [set ::env($var)]} on error {} {return {}}
	}

	# MpiExec detection
	proc mpiexec {} {
		variable mpiexec
		try {set mpiexec} on error {} {
			set mpiexec {}
			foreach x [collect [env MPIEXEC] mpiexec mpirun] {
				if {![catch {exec {*}$x}]} {
					set mpiexec $x
					break
				}
			}
		}
		if {$mpiexec == {}} {error {failed to detect MpiExec or equivalent}}
		return $mpiexec
	}

	# PkgConfig detection
	proc pkg-config {} {
		variable pc
		try {set pc} on error {} {
			set pc {}
			foreach x [collect [env PKG_CONFIG] pkg-config pkgconf] {
				if {![catch {exec {*}$x --version}]} {
					set pc $x
					break
				}
			}
		}
		if {$pc == {}} {error {failed to detect PkgConfig or equivalent}}
		return $pc
	}

	# C compiler detection
	proc cc {} {
		variable cc
		try {set cc} on error {} {
			set cc {}
			foreach x [collect [env CC] gcc] {
				if {![catch {exec {*}$x --version}]} {
					set cc $x
					break
				}
			}
		}
		if {$cc == {}} {error {failed to detect C compiler}}
		return $cc
	}

	# C++ compiler detection
	proc cxx {} {
		variable cxx
		try {set cxx} on error {} {
			set cxx {}
			foreach x [collect [env CXX] g++] {
				if {![catch {exec {*}$x --version}]} {
					set cxx $x
					break
				}
			}
		}
		if {$cxx == {}} {error {failed to detect C++ compiler}}
		return $cxx
	}

	# FORTRAN compiler detection
	proc fc {} {
		variable fc
		try {set fc} on error {} {
			set fc {}
			foreach x [collect [env FC] gfortran] {
				if {![catch {exec {*}$x --version}]} {
					set fc $x
					break
				}
			}
		}
		if {$fc == {}} {error {failed to detect FORTRAN compiler}}
		return $fc
	}

	# MPI C compiler detection
	proc mpicc {} {
		variable mpicc
		try {set mpicc} on error {} {
			set mpicc {}
			foreach x [collect [env MPICC] mpicc] {
				if {![catch {exec {*}$x --version}]} {
					set mpicc $x
					break
				}
			}
		}
		if {$mpicc == {}} {error {failed to detect MPI C compiler}}
		return $mpicc
	}

	# MPI C++ compiler detection
	proc mpicxx {} {
		variable mpicxx
		try {set mpicxx} on error {} {
			set mpicxx {}
			foreach x [collect [env MPICXX] mpicxx mpic++] {
				if {![catch {exec {*}$x --version}]} {
					set mpicxx $x
					break
				}
			}
		}
		if {$mpicxx == {}} {error {failed to detect MPI C++ compiler}}
		return $mpicxx
	}

	# MPI FORTRAN compiler detection
	proc mpifc {} {
		variable mpifc
		try {set mpifc} on error {} {
			set mpifc {}
			foreach x [collect [env MPIFC] [env MPIFORT] mpifort mpif90 mpif77] {
				if {![catch {exec {*}$x --version}]} {
					set mpifc $x
					break
				}
			}
		}
		if {$mpifc == {}} {error {failed to detect MPI FORTRAN compiler}}
		return $mpifc
	}

	# Create temporary directory
	proc mktempdir {} {
		set t [file join $::env(TEMP) [file rootname [file tail [info script]]][expr {int(rand()*9999)}]]
		file mkdir $t
		return $t
	}

	# Delete directory tree
	proc rmdir {dir} {
		try {
			file delete -force $dir
		} on error {} {
			# This command fires up a background sanitizing process which does its best to delete
			# staging directory in spite of executable locks or what's not which can happend on Windows
			# This code should work in real UNIX environments or UNIX-like Windows environments
			# such as Cygwin or MSYS(2).
			exec -ignorestderr sh -c "nohup \${SHELL} -c \" while \[ -d '$dir' \]; do rm -rf '$dir' || sleep 3; done\" > /dev/null 2>&1 &"
		}
	}

	# Execute script from a temporary location
	# Script file is copied to the location along with all residing files
	# The location gets deleted afterwards
	proc sandbox {script} {
		variable stagedir [mktempdir]
		try {
			::tcltest::configure {*}$::argv
			::tcltest::workingDirectory $stagedir
			file copy -force {*}[glob -directory [file dirname [file normalize [info script]]] -nocomplain *] $stagedir
			eval $script
		} finally {
			::tcltest::cleanupTests
			rmdir $stagedir
		}
	}

	# Obtain compilation and linking flags for the specified packages via PkgConfig
	proc require {args} {
		if {[constraint? static]} {set flags --static} else {set flags {}}
		xflags {*}[lindex [dict get [system [pkg-config] {*}$args --cflags {*}$flags] stdout] 0]
		ldflags {*}[lindex [dict get [system [pkg-config] {*}$args --libs {*}$flags] stdout] 0]
		return
	}

	# Deduce source code language from command line arguments
	proc deduce-language {opts} {
		switch -regexp -nocase [lindex $opts [lsearch -glob -not $opts {-*}]] {
			{\.c$} {return c}
			{\.(cxx|cpp|cc)$} {return cxx}
			{\.(f|for|f\d+)$} {return fortran}
			default {error {failed to deduce source language from the command line agruments}}
		}
	}

	# Deduce compilation command from command line arguments
	proc deduce-compiler-proc {opts} {
		set lang [deduce-language $opts]
		if {[constraint? mpi]} {
			switch $lang {
				c {return mpicc}
				cxx {return mpicxx}
				fortran {return mpifc}
			}
		} else {
			switch $lang {
				c {return cc}
				cxx {return cxx}
				fortran {return fc}
			}
		}
	}

	# Deduce compilation flags command from command line arguments
	proc deduce-compile-flags-proc {opts} {
		switch [deduce-language $opts] {
			c {return cflags}
			cxx {return cxxflags}
			fortran {return fflags}
		}
	}
	
	# C preprocessor command line arguments
	proc cppflags {args} {
		variable cppflags
		try {set cppflags} on error {} {
			set cppflags [lsqueeze [env CPPFLAGS]]
			if {![constraint? debug]} {lappend cppflags -DNDEBUG}
		}
		lappend cppflags {*}$args
	}

	# Language-agnostic compilation command line arguments
	proc xflags {args} {
		variable xflags
		try {set xflags} on error {} {
			set xflags {}
			if {[constraint? openmp]} {lappend xflags -fopenmp}
			if {[constraint? thread]} {lappend xflags -pthread}
			if {[constraint? debug]} {lappend xflags -Og} else {lappend xflags -O2}
		}
		lappend xflags {*}$args
	}

	# C-specific compilation command line arguments
	proc cflags {args} {
		variable cflags
		try {set cflags} on error {} {
			set cflags [lsqueeze [env CFLAGS]]
		}
		lappend cflags {*}$args
	}

	
	# C++-specific compilation command line arguments
	proc cxxflags {args} {
		variable cxxflags
		try {set cxxflags} on error {} {
			set cxxflags [lsqueeze [env CXXFLAGS]]
		}
		lappend cxxflags {*}$args
	}

	# FORTRAN-specific compilation command line arguments
	proc fflags {args} {
		variable fflags
		try {set fflags} on error {} {
			set fflags [lsqueeze [env FFLAGS]]
		}
		lappend fflags {*}$args
	}

	# Linker command line arguments
	proc ldflags {args} {
		variable ldflags
		try {set ldflags} on error {} {
			set ldflags [lsqueeze [env LDFLAGS]]
			if {[constraint? static]} {lappend ldflags -static}
			if {[constraint? openmp]} {lappend ldflags -fopenmp}
			if {[constraint? thread]} {lappend ldflags -pthread}
		}
		lappend ldflags {*}$args
	}

	# Linked libraries command line arguments
	proc libs {args} {
		variable libs
		try {set libs} on error {} {
			set libs [lsqueeze [env LIBS]]
			if {[constraint? cxx]} {lappend libs -lstdc++}
			if {[constraint? fortran]} {lappend libs -lgfortran -lquadmath}
		}
		lappend libs {*}$args
	}

	# Perform source code compilation into executable
	# Returns the executable name
	proc build {args} {
		set args [lsqueeze $args]
		set exe [executable]
		system {*}[concat \
			[[deduce-compiler-proc $args]] \
			-o $exe \
			[cppflags] \
			[xflags] \
			[[deduce-compile-flags-proc $args]] \
			$args \
			[ldflags] \
			[libs] \
		]
		return $exe
	}

	# Perform running of the specified executable with supplied command line arguments
	proc run {args} {
		if {[constraint? mpi]} {set runner [mpiexec]} else {set runner {}}
		system {*}[list $runner {*}$args]
	}

	# Execute command line built from command line arguments
	proc system {args} {
		variable system-count
		incr system-count
		set stdout stdout${system-count}
		set stderr stderr${system-count}
		set args [lsqueeze $args]
		set command [join $args]
		if {[lsearch [::tcltest::verbose] exec] < 0} {set verbose 0} else {set verbose 1}
		if {$verbose} {::puts [::tcltest::outputChannel] "> $command"}
		try {
			exec -ignorestderr -- {*}$args > $stdout 2> $stderr
			set options {}
			set status 0
			set code ok
		} trap CHILDSTATUS {results options} {
			set status [lindex [dict get $options -errorcode] 2]
			set code error
		} finally {
			try {
				set out [read-file $stdout]
				set err [read-file $stderr]
			} finally {
				file delete -force $stdout $stderr
			}
		}
		if {$verbose} {
			foreach x $out {::puts [::tcltest::outputChannel] $x}
			if {[llength $out] > 0 && [llength $err] > 0} {::puts [::tcltest::outputChannel] ----}
			foreach x $err {::puts [::tcltest::outputChannel] $x}
		}
		return -code $code [dict create command $command status $status stdout $out stderr $err options $options]
	}

	# Construct a scalar type ID from the constraints
	proc x {} {
		if {[constraint? complex]} {
			if {[constraint? double]} {return z}
			if {[constraint? single]} {return c}
		} else {
			if {[constraint? double]} {return d}
			if {[constraint? single]} {return s}
		}
		error {failed to contstruct the scalar type}
	}

	# Construct an execution model ID from the constraints
	proc y {} {
		if {[constraint? mpi]} {return m}
		if {[constraint-any? openmp thread]} {return t}
		if {[constraint? hybrid]} {return h}
		return s
	}

	# Construct an build type ID from the constraints
	proc z {} {
		if {[constraint? debug]} {return g}
		return o
	}

	# Construct a 3-letter XYZ build code from the constraints
	proc xyz {} {
		 return [x][y][z]
	}

	# Construct the test name based on the constraints set
	proc name {} {
		join [list [file rootname [file tail [info script]]] {*}[constraints]] -
	}

	# Construct new unique executable name
	proc executable {} {
		variable build-count
		variable executable
		return [set executable [name]-[incr build-count].exe]
	}

	# Construct human-readable description of the test according to the contraints set
	proc description {} {
		set t {}
		switch [intersection {c cxx fortran} [constraints]] {
			c {lappend t C}
			cxx {lappend t C++}
			fortran {lappend t FORTRAN}
		}
		try {
			switch [y] {
				s {lappend t sequential}
				m {lappend t MPI}
				t {lappend t multithreaded}
				h {lappend t heterogeneous}
			}
		} on error {} {}
		try {
			switch [x] {
				s {lappend t "single precision"}
				d {lappend t "double precision"}
				c {lappend t "single precision complex"}
				z {lappend t "double precision complex"}
			}
		} on error {} {}
		switch [z] {
			o {lappend t optimized}
			g {lappend t debugging}
		}
		if {[constraint? static]} {lappend t static}
		join $t
	}

	# Set constraints
	proc constraints {args} {
		variable constraints
		try {set constraints} on error {} {
			set constraints {}
		}
		lappend constraints {*}[lsqueeze $args]
	}

	# Return true if any of specified contraints is set
	proc constraint-any? {args} {
		foreach ct $args {
			if {[constraint? $ct]} {return 1}
		}
		return 0
	}

	# Return true if all scpecified constraints are set
	proc constraint-all? {args} {
		foreach ct $args {
			if {![constraint? $ct]} {return 0}
		}
		return 1
	}

	# Return true if specified constraint is set
	proc constraint? {ct} {
		variable constraints
		expr {[lsearch $constraints $ct] >= 0}
	}

	# Test failure is triggered by throwing an exception
	::tcltest::customMatch exception {return 1; #}

	# Main test command
	::np::proc test {{name {}} {description {}} {match {exception}} -- cts script} {
		foreach v {executable constraints cppflags xflags cflags cxxflags fflags ldflags libs} {variable $v; catch {unset $v}}
		set cts [constraints {*}$cts]
		if {$name == {}} {set name [name]}
		if {$description == {}} {set description "[description] build"}
		::tcltest::test $name $description -constraints $cts -body $script -match $match
		# Attempt to delete the created executable if any to conserve space in the stage dir
		# It's OK for the operation to fail at this point as the executable may still be locked
		variable executable; try {file delete -force $executable} on error {} {}
	}

	# To be used in {all.tcl}
	proc suite {args} {
		::tcltest::configure -testdir [file dirname [file normalize [info script]]] {*}$args
		::tcltest::runAllTests
		::tcltest::cleanupTests
	}

	# Quick & dirty hack to introduce extra verbosity option(s)
	# Override the proc from in tcltest-*.tm
	# Original code corresponds to version 2.5.1
    proc ::tcltest::AcceptVerbose { level } {
	set level [AcceptList $level]
	set levelMap {
		x exec
	    l list
	    p pass
	    b body
	    s skip
	    t start
	    e error
	    l line
	    m msec
	    u usec
	}
	set levelRegexp "^([join [dict values $levelMap] |])\$"
	if {[llength $level] == 1} {
	    if {![regexp $levelRegexp $level]} {
		# translate single characters abbreviations to expanded list
		set level [string map $levelMap [split $level {}]]
	    }
	}
	set valid [list]
	foreach v $level {
	    if {[regexp $levelRegexp $v]} {
		lappend valid $v
	    }
	}
	return $valid
    }

	# Read text file and return list of lines
	proc read-file {file} {
		set f [open $file r]
		try {
			return [split [read -nonewline $f] \n]
		} finally {
			close $f
		}
	}

	# Return a new list from the specified list entries squashing {} values
	proc lsqueeze {list} {
		set out [list]
		foreach x $list {
			if {$x != {}} {lappend out $x}
		}
		return $out
	}

	# Return a new list from the specified arguments squashing {} values
	proc collect {args} {
		lsqueeze $args
	}

	# Return intersection of two lists
	proc intersection {a b} {
		set x {}
		foreach i $a {
			if {[lsearch -exact $b $i] != -1} {lappend x $i}
		}
		return $x
	}
}