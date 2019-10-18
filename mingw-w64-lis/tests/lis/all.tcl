# Template `all.tcl` file which is to be put into each test module's directory
package require tcltest
namespace import ::tcltest::*
configure -testdir [file dirname [file normalize [info script]]]
eval configure $argv
runAllTests