# To the extent possible under law, the author(s) have dedicated all
# copyright and related and neighboring rights to this software to the
# public domain worldwide. This software is distributed without any warranty.
# You should have received a copy of the CC0 Public Domain Dedication along
# with this software.
# If not, see <https://creativecommons.org/publicdomain/zero/1.0/>.


# System-wide profile file

# Setup some default paths. Note that this order will allow user installed
# software to override 'system' software.
# Modifying these default path settings can be done in different ways.
# To learn more about startup files, refer to your shell's man page.

set MSYS2_PATH /usr/local/bin /usr/bin /bin
set -x MANPATH /usr/local/man /usr/share/man /usr/man /share/man
set -x INFOPATH /usr/local/info /usr/share/info /usr/info /share/info

if not set -q MSYS2_PATH_TYPE
	set MSYS2_PATH_TYPE minimal
end

switch $MSYS2_PATH_TYPE
case strict
    # Do not inherit any path configuration, and allow for full customization
    # of external path. This is supposed to be used in special cases such as
    # debugging without need to change this file, but not daily usage.
    set -ex ORIGINAL_PATH
case inherit
    # Inherit previous path. Note that this will make all of the Windows path
    # available in current shell, with possible interference in project builds.
	if not set -q ORIGINAL_PATH
		set -x ORIGINAL_PATH $PATH
	end
case '*'
    # Do not inherit any path configuration but configure a default Windows path
    # suitable for normal usage with minimal external interference.
    set -l PATH $MSYS2_PATH 2>/dev/null
	set WIN_ROOT (cygpath -Wu)
    set -x ORIGINAL_PATH $WIN_ROOT/System32 $WIN_ROOT $WIN_ROOT/System32/Wbem $WIN_ROOT/System32/WindowsPowerShell/v1.0/
end

set -e MINGW_MOUNT_POINT
source '/clang64/etc/fish/msystem.fish'
switch $MSYSTEM
case MINGW32
  set MINGW_MOUNT_POINT $MINGW_PREFIX
  set -x PATH $MINGW_MOUNT_POINT/bin $MSYS2_PATH $ORIGINAL_PATH 2>/dev/null
  set -x PKG_CONFIG_PATH "$MINGW_MOUNT_POINT/lib/pkgconfig:$MINGW_MOUNT_POINT/share/pkgconfig"
  set ACLOCAL_PATH "$MINGW_MOUNT_POINT/share/aclocal:/usr/share/aclocal"
  set -x MANPATH $MINGW_MOUNT_POINT/local/man $MINGW_MOUNT_POINT/share/man $MANPATH
case MINGW64
  set MINGW_MOUNT_POINT $MINGW_PREFIX
  set -x PATH $MINGW_MOUNT_POINT/bin $MSYS2_PATH $ORIGINAL_PATH 2>/dev/null
  set -x PKG_CONFIG_PATH "$MINGW_MOUNT_POINT/lib/pkgconfig:$MINGW_MOUNT_POINT/share/pkgconfig"
  set ACLOCAL_PATH "$MINGW_MOUNT_POINT/share/aclocal:/usr/share/aclocal"
  set -x MANPATH $MINGW_MOUNT_POINT/local/man $MINGW_MOUNT_POINT/share/man $MANPATH
case CLANG64
  set MINGW_MOUNT_POINT $MINGW_PREFIX
  set -x PATH $MINGW_MOUNT_POINT/bin $MSYS2_PATH $ORIGINAL_PATH 2>/dev/null
  set -x PKG_CONFIG_PATH "$MINGW_MOUNT_POINT/lib/pkgconfig:$MINGW_MOUNT_POINT/share/pkgconfig"
  set ACLOCAL_PATH "$MINGW_MOUNT_POINT/share/aclocal:/usr/share/aclocal"
  set -x MANPATH $MINGW_MOUNT_POINT/local/man $MINGW_MOUNT_POINT/share/man $MANPATH
case CLANGARM64
  set MINGW_MOUNT_POINT $MINGW_PREFIX
  set -x PATH $MINGW_MOUNT_POINT/bin $MSYS2_PATH $ORIGINAL_PATH 2>/dev/null
  set -x PKG_CONFIG_PATH "$MINGW_MOUNT_POINT/lib/pkgconfig:$MINGW_MOUNT_POINT/share/pkgconfig"
  set ACLOCAL_PATH "$MINGW_MOUNT_POINT/share/aclocal:/usr/share/aclocal"
  set -x MANPATH $MINGW_MOUNT_POINT/local/man $MINGW_MOUNT_POINT/share/man $MANPATH
case UCRT64
  set MINGW_MOUNT_POINT $MINGW_PREFIX
  set -x PATH $MINGW_MOUNT_POINT/bin $MSYS2_PATH $ORIGINAL_PATH 2>/dev/null
  set -x PKG_CONFIG_PATH "$MINGW_MOUNT_POINT/lib/pkgconfig:$MINGW_MOUNT_POINT/share/pkgconfig"
  set ACLOCAL_PATH "$MINGW_MOUNT_POINT/share/aclocal:/usr/share/aclocal"
  set -x MANPATH $MINGW_MOUNT_POINT/local/man $MINGW_MOUNT_POINT/share/man $MANPATH
case '*'
  set -x PATH $MSYS2_PATH /opt/bin $ORIGINAL_PATH 2>/dev/null
  set -x PKG_CONFIG_PATH "/usr/lib/pkgconfig:/usr/share/pkgconfig:/lib/pkgconfig"
end

# TMP and TEMP as defined in the Windows environment must be kept
# for windows apps, even if started from msys2. However, leaving
# them set to the default Windows temporary directory or unset
# can have unexpected consequences for msys2 apps, so we define
# our own to match GNU/Linux behaviour.
#
# Note: this uppercase/lowercase workaround does not seem to work.
# In fact, it has been removed from Cygwin some years ago. See:
#
#     * https://cygwin.com/git/gitweb.cgi?p=cygwin-apps/base-files.git;a=commitdiff;h=3e54b07
#     * https://cygwin.com/git/gitweb.cgi?p=cygwin-apps/base-files.git;a=commitdiff;h=7f09aef
#
if not set -q ORIGINAL_TMP
	set ORIGINAL_TMP $TMP
end
if not set -q ORIGINAL_TEMP
	set ORIGINAL_TEMP $TEMP
end
set -e TMP
set -e TEMP
set -x tmp (cygpath -w "$ORIGINAL_TMP" 2> /dev/null)
set -x temp (cygpath -w "$ORIGINAL_TEMP" 2> /dev/null)
set -x TMP "/tmp"
set -x TEMP "/tmp"

# Define default printer
set p '/proc/registry/HKEY_CURRENT_USER/Software/Microsoft/Windows NT/CurrentVersion/Windows/Device'
if test -e $p
	#Can't use read -d, fish too old
	set PRINTER (cat $p)
	set -x PRINTER (string split ',' $PRINTER)[1]
end
set -e p

/usr/bin/hostname | read HOSTNAME
set -x HOSTNAME $HOSTNAME
source /clang64/etc/fish/perlbin.fish

if set -q ACLOCAL_PATH
  set -x ACLOCAL_PATH $ACLOCAL_PATH
end

export USER PRINTER PS1 SHELL
set -e PATH_SEPARATOR
