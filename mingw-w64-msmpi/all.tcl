# Template {all.tcl} file which is to be put into each test module's directory.
# Either [source] the bundled {tclbuildtest.tcl} to force load the specific code or
# alter the $auto_path list to add location of {pkgIndex.tcl,tclbuildtest.tcl}.
# The same scheme is to be applied to all .test files.
source [file join [file dirname [file normalize [info script]]] tclbuildtest.tcl]
package require tclbuildtest
::tclbuildtest::suite {*}$::argv