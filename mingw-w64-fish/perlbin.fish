# Set path to perl scriptdirs if they exist
# https://wiki.archlinux.org/index.php/Perl_Policy#Binaries_and_scripts
# Added /usr/bin/*_perl dirs for scripts
# Remove /usr/lib/perl5/*_perl/bin in next release

function __addpath
	set -l p $argv[1]
	if test -d $p
		set PATH $PATH $p
	end
end

__addpath /usr/bin/site_perl
__addpath /usr/lib/perl5/site_perl/bin
__addpath /usr/bin/vendor_perl
__addpath /usr/lib/perl5/vendor_perl/bin
__addpath /usr/bin/core_perl

functions -e __addpath