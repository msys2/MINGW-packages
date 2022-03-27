#!/bin/bash

echo
echo "Qemu examples & tests."
echo "----------------------"
echo "Created to test Msys2 Qemu package, known to work for Cygwin and Linux, too."
echo "Executed Qemu commandlines will be printed to screen."
echo
CONFIGFILE=~/.qemu.$(basename $0)
CONFIGFILENAME="$(basename $CONFIGFILE)"
if [ ! -f "$CONFIGFILE" ]
then
	DOWNLOADDIR=$(realpath .)
	echo "Configuring current directory '$DOWNLOADDIR' as download directory."
	read -p "Continue? (y|[n]) " TEST
	echo
	[ "y" == "$TEST" ] || exit
	touch "$CONFIGFILE" || exit 1
	touch "$DOWNLOADDIR/$CONFIGFILENAME" || exit 1
	echo "$DOWNLOADDIR" > "$CONFIGFILE"
fi
DOWNLOADDIR=$(cat "$CONFIGFILE")
echo "Configuration file: '$CONFIGFILE'"
echo "Download directory: '$DOWNLOADDIR'"
if ! mkdir -p "$DOWNLOADDIR" || ! touch "$DOWNLOADDIR/$CONFIGFILENAME"
then
	echo "Download directory '$DOWNLOADDIR' is not usable"
	exit 1
fi
echo
echo "On execution each test needs to download, most test only a few 10 MB or less,"
echo "but several up to some 100MB."
read -p "Only accept reasonable downloads? ([y]|n) " TEST
[ "n" != "$TEST" ] || IGNORESIZE=1
echo
read -p "Clean after execution (removes all but downloads)? (y|[n]) " TEST
[ "y" == "$TEST" ] && REMOVEEXECDIR=1
echo
read -p "Audio input (microphone) accessible? ([y]|n) " TEST
[ "n" != "$TEST" ] && MICROPHONE=1
echo
echo "Name block of qemu examples to execute."
echo "Choose year of qemu-advent-calender (2014, 2016, 2018, 2020) or qemu-desktop (DVD)"
read -p "Your choice? (2014|2016|2018|2020|[DVD]) " BLOCK
echo
if [ -n "$MINGW_PACKAGE_PREFIX" ]
then
	read -p "Execute as regression test? (y|[n]) " REGRESSION
	if [ "y" == "$REGRESSION" ]
	then
		echo "To execute as regression test, provide absolute paths of two"
	        echo "$MINGW_PACKAGE_PREFIX-qemu-Archives to compare, leave empty otherwise."
		read -p "First  $MINGW_PACKAGE_PREFIX-qemu archive? " FIRSTQEMU
		read -p "Second $MINGW_PACKAGE_PREFIX-qemu archive? " SECONDQEMU
	fi
	echo
fi

LIVE_IMAGE_FILE=openSUSE-Leap-15.3-GNOME-Live-x86_64-Media.iso
LIVE_IMAGE_URL=https://download.opensuse.org/distribution/leap/15.3/live/$LIVE_IMAGE_FILE

function download {
	local URL="$1"
	local FILE="$2"
	if [ -z "$URL" ]
	then
		echo "URL $URL is missing"
		exit 1
	fi
	[ -n "$FILE" ] || FILE=$(basename "$URL")
	[ -n "$FILE" ] || exit 1
	rm -f $FILE.tmp
	[ -f $FILE ] || wget -O $FILE.tmp $URL || exit 1
	[ -f $FILE.tmp ] && mv $FILE.tmp $FILE
}

function removeDir {
	local EXECDIR=$1
	[ -n "$REMOVEEXECDIR" ] && [ -d "$EXECDIR" ] && rm -rfv $EXECDIR
}

function testImageInDir {
	local EXECDIR=$1
	[ -d "$EXECDIR" ] || mkdir -p "$EXECDIR"
	[ -f "$EXECDIR/testimage.qcow2" ] || qemu-img create -f qcow2 "$EXECDIR/testimage.qcow2" 20G &> /dev/null
}

function qWhich {
	# Only use for identifying correct qemu-system-* path (because which fails), maybe buggy else!
	local PATHTAIL="$PATH" PATHHEAD TESTDIR BINARY
	while [[ $PATHTAIL =~ ^(:*)([^:]+)(:.*)?$ ]]
	do
		PATHHEAD="${BASH_REMATCH[2]}"
		if [ "~" == "${PATHHEAD:0:1}" ]
		then
			TESTDIR="$(echo ~)${PATHHEAD:1}"
		else
			TESTDIR="$PATHHEAD"
		fi
		BINARY=$(ls "$TESTDIR/$1" 2> /dev/null | head)
		if [ -n "$BINARY" ]
		then
			echo "$(dirname $BINARY)/$(basename $1)"
			return 0
		fi
		PATHTAIL="${PATHTAIL:${#PATHHEAD}+1}"
	done
	return 1
}

function showMsys2ArgConvEnv {
	[ -n "$MSYSTEM" ] && [ -n "$MSYS2_ARG_CONV_EXCL" ]
}

function toolexec {
	local LINE PARAM
	for PARAM in "${@}"
	do
		if [ "$PARAM" == "" ] || [[ $PARAM =~ ' ' ]]
		then
			PARAM="'$PARAM'"
		fi
		LINE="${LINE}${PARAM} "
	done
	echo "$LINE"
	"${@}"
	echo
}

function execute {
	echo "--------------------------------------------------------------------------------"
	showMsys2ArgConvEnv && echo "export MSYS2_ARG_CONV_EXCL=\"$MSYS2_ARG_CONV_EXCL\""
	local PARAM LINE INDENT PREVH CURRH
	for PARAM in "${@}"
	do
		[ "${PARAM:0:1}" == '-' ] && CURRH="-" || CURRH=""

		# Newline before "-"-param and between 2 non-"-"-params
		if [ -n "$LINE" ] && ( [ -n "$CURRH" ] || [ -z "$PREVH" ] )
		then
			echo "$LINE \\"
			LINE=""
		fi

		# Add quots to param, if param is empty or contains blanks
		if [ "$PARAM" == "" ] || [[ $PARAM =~ ' ' ]]
		then
			PARAM="'$PARAM'"
		fi

		LINE="${LINE}${INDENT}${PARAM}"

		INDENT=" "
		PREVH="$CURRH"
	done
	echo "$LINE"
	showMsys2ArgConvEnv && echo "unset MSYS2_ARG_CONV_EXCL"
	echo "--------------------------------------------------------------------------------"
	echo
	"${@}"
}

function ignoreSize {
	[ -n "$IGNORESIZE" ]
}

function isQemuSystem {
	local ARCH
	which "qemu-system-$ARCH" &> /dev/null
}

function isLinux {
	! isWindows && [ "$(uname)" == "Linux" ]
}

function isWindows {
	( [ -n "$OS" ] && [[ $OS =~ Windows ]] ) ||
	       [ -f "$(qWhich qemu-system-x86_64).exe" ]
}

function killBackgroundQemu {
	# Qemu stopped, if not exists
	if [ -f $PIDFILE ]
	then
		local PID=$(cat $PIDFILE)
		if isWindows
		then
			PID=$( ps | grep "qemu-system" | grep "\b$PID\b" |
				sed "s/^\s*//" | sed "s/\s\s*.*$//" )
		fi
		if [ -n "$PID" ]
		then
			echo "Killing Qemu PID $PID..."
			kill "$PID"
			sleep 1
		else
			echo "Couldn't kill background Qemu, stopping!"
			exit 1
		fi
	fi
	rm -f $PIDFILE
}

function cygwinXlaunch {
	if [[ $(uname) =~ CYGWIN ]] && ! ps | grep xlaunch &> /dev/null
	then
		echo "Please start xlaunch to open display!"
		read -p "Ready to procede? " TEST
	fi
}

function executeVncForBackgroundQemu {
	sleep 2
	echo
	toolexec gvncviewer localhost:5 2> /dev/null
	sleep 1
	killBackgroundQemu
}

function executeSpicyForBackgroundQemu {
	sleep 2
	echo
	if [ "$BLOCK" == "DVD" ]
	then
		echo "--------------------------------------------------------------------------------"
		echo "openSUSE Leap Live was choosen as example because it includes spice guest tools."
		echo "For optimal spice desktop experience guest tools installation is recommended:"
		echo " * Linux: spice-vdagent (included in Linux distribution)"
		echo " * Windows: spice-guest-tools - see https://www.spice-space.org/download.html"
		echo "--------------------------------------------------------------------------------"
		echo
	fi
	echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
	echo "When screen of spice display blanks, enlarge to see progress. Please be patient!"
	echo "            Escape from spice display by pressing F10 or Shift-L F12"
	echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
	echo
	toolexec spicy -h localhost -p 5905 2> /dev/null
	sleep 1
	killBackgroundQemu
}

function checkBinary {
	local PACKAGE=$1
	PACKAGE=$(echo $PACKAGE | sed "s/^-//")
	# Binary to test for package, if binary doesn't equal package name
	declare -A BINARIES
	BINARIES["qemu"]="qemu-system-x86_64"
	BINARIES["gtk-vnc"]="gvncviewer"
	BINARIES["spice-gtk"]="spicy"
	BINARIES["p7zip"]="7z"
	which $PACKAGE &> /dev/null ||
		which ${BINARIES[$PACKAGE]} &> /dev/null ||
		echo "$PACKAGE"
}

function require {
	# Don't require msys2-package even in msys2 shell!
	if [ -n "$(checkBinary qemu)" ]
	then
		echo "Qemu is not in PATH: $PATH"
		exit 1
	fi

	local MISSING_PKGS PACKAGE
	echo "Testing for expected packages..."
	for PACKAGE in "${@}"
	do
		if [ -n "$MSYSTEM" ]
		then
			pacman -Q -i $PACKAGE &> /dev/null || MISSING_PKGS="$MISSING_PKGS $PACKAGE"
		else
			PACKAGE="$(checkBinary $PACKAGE)"
			[ -z "$PACKAGE" ] || MISSING_PKGS="$MISSING_PKGS $PACKAGE"
		fi
	done
	if [ -n "$MISSING_PKGS" ]
	then
		echo "Missing packages: $MISSING_PKGS"
		if [ -n "$MSYSTEM" ]
		then
			read -p "Install? (y|[n]) " TEST
			[ "y" != "$TEST" ] || pacman --noconfirm -S $MISSING_PKGS
		else
			read -p "Procede without installing? (y|[n]) " TEST
			[ "y" != "$TEST" ] && exit 0
		fi
	else
		echo "Done."
	fi
}

function perform {
	local FUN=$1
	if [ -n "$DIR" ]
	then
		[ -d "$DIR" ] || mkdir -p "$DIR"
		cd "$DIR"
	else
		echo "DIR missing"
		exit 1
	fi
	echo 
	echo "================================================================================"
	echo 
	read -p "Execute $FUN? (y|[n]) " TEST
	[ "y" == "$TEST" ] || return 0
	if [ -f "$FIRSTQEMU" ] && [ -f "$SECONDQEMU" ]
	then
		echo
		pacman --noconfirm -U $FIRSTQEMU &> /dev/null
		echo "First part of regression test with $(basename $FIRSTQEMU)"
		echo
		$FUN
		echo
		pacman --noconfirm -U $SECONDQEMU &> /dev/null
		echo "Second part of regression test with $(basename $SECONDQEMU)"
		$FUN
	else
		echo
		$FUN
	fi
}

function extractReadme {
	local FILE=$1
	[ -f "$FILE" ] || return 0
	local TXT=0
	local LINE
	local EOF="EOF"
	# Print the first HERE-Document to screen
	cat $FILE | while read LINE
	do
		if [ "$TXT" == "1" ]
		then
			[[ $LINE =~ $EOF ]] && return || echo "$LINE"
		else
			[[ $LINE =~ $EOF ]] && TXT=1
		fi
	done
}

function qemuMinVersion {
	local MAJORPARAM="$1"
	local MINORPARAM="$2"
	local MICROPARAM="$3"
	[ -n "$MAJORPARAM" ] || MAJORPARAM="0"
	[ -n "$MINORPARAM" ] || MINORPARAM="0"
	[ -n "$MICROPARAM" ] || MICROPARAM="0"
	local VERSION="$(qemu-system-x86_64 -version 2> /dev/null | grep version)"
	local REGEX="version ([0-9]+)\.([0-9]+)\.([0-9]+)"
	[[ $VERSION =~ $REGEX ]] &&
		local MAJOR="${BASH_REMATCH[1]}" MINOR="${BASH_REMATCH[2]}" MICRO="${BASH_REMATCH[3]}" ||
		return 1
	(( MAJOR > MAJORPARAM )) ||
		( (( MAJOR == MAJORPARAM )) && (( MINOR > MINORPARAM )) ) ||
		( (( MAJOR == MAJORPARAM )) && (( MINOR == MINORPARAM )) && (( MICRO >= MICROPARAM )) )
}

function determineAccel {
	qemuMinVersion 6 0 && WHPX="whpx,kernel-irqchip=off" || WHPX=whpx
	isWindows && TESTACCELS="$WHPX hax"
	isLinux && TESTACCELS="kvm xen"
	local TESTACCEL
	for TESTACCEL in $TESTACCELS
	do
		echo "Testing Acceleration $TESTACCEL..."
		qemu-system-x86_64 -accel $TESTACCEL -display none -pidfile $PIDFILE &> /dev/null &
		sleep 1
		[ -f "$PIDFILE" ] && ACCEL=$TESTACCEL && killBackgroundQemu &> /dev/null && break
	done
	[ -z "$ACCEL" ] && ACCEL=tcg
	echo "Acceleration is $ACCEL"
	echo
}

function accel {
	echo "-accel $ACCEL"
}

# Intended to determine absolute path for qemu-provided firmware files only
function firmware {
	local FW_NAME="$1"

	local BINDIR="$(dirname $(qWhich qemu-system-x86_64))"
	local COMMON_FW="$BINDIR/../share/qemu"
	local MSYS_FW_OLD="$BINDIR/../lib/qemu"
	local DIST="$BINDIR"
	local QI_FWDSC="/usr/share/qemu/firmware"
	local CYG64_DIST="/cygdrive/c/Program Files/qemu"
	local CYG32_DIST="/cygdrive/c/Program Files (x86)/qemu"

	local FW_PATH DIR REALDIR
	for DIR in "$COMMON_FW" "$MSYS_FW_OLD" "$DIST/share" "$DIST" \
		"$( [ -L "$QI_FWDSC" ] && [ -d "$(realpath $QI_FWDSC)" ] && dirname $(realpath $QI_FWDSC) )" \
		"$CYG64_DIST/share" "$CYG64_DIST" "$CYG32_DIST/share" "$CYG32_DIST"
	do
		if [ -d "$DIR" ]
		then
			REALDIR="$( realpath -e "$DIR" 2> /dev/null )"
			FW_PATH="$( find "$REALDIR" -type f -name "$FW_NAME" 2> /dev/null | tail -n1 )"
			[ -f "$FW_PATH" ] && echo "$FW_PATH" && return
		fi
	done

	echo "$FW_NAME"
}

function firmwareAvailable {
	local FWS="${@}"
	local FW
	for FW in $FWS
	do
		if [ ! -f "$(firmware $FW)" ]
		then
			echo "Firmware not available: '$(firmware $FW)'"
			false
		fi
	done
}

function audiodev {
	local ID=$1 APPEND=""
	[ -z "$MICROPHONE" ] && APPEND=",in.voices=0"
	qemu-system-x86_64 -audio-help 2> /dev/null | grep "^-audiodev" | head -n1 |
		sed "s/ id=[a-z]*,/ id=$ID,/" | sed "s/\s*$/$APPEND/"
}

function pcspk {
	qemuMinVersion 5 1 &&
		echo "$(audiodev pcpsk0) -machine pcspk-audiodev=pcpsk0" || echo "-soundhw pcspk"
}

function audio {
	local DEVICE="$1" DRIVER="$2" HDA_BUS="$3"
	[ -z "$DEVICE" ] && DEVICE="ES1370"
	[ -z "$DRIVER" ] && DRIVER="$(audiodev audio0)" || DRIVER="-audiodev $DRIVER,id=audio0"
	[ -z "$HDA_BUS" ] && HDA_BUS="intel-hda"
	[[ $DEVICE =~ ^hda ]] && DEVICE="-device $HDA_BUS -device $DEVICE" || DEVICE="-device $DEVICE"
	qemuMinVersion 4 2 && echo "$DRIVER $DEVICE,audiodev=audio0" || echo "$DEVICE"
}

function audioq35 {
	local DEVICE="$1" DRIVER="$2"
	[ -z "$DEVICE" ] && DEVICE="hda-output"
	audio "$DEVICE" "$DRIVER" "ich9-intel-hda"
}

# UEFI-Pflash-Desktop (LiveImage)
function qemuLiveDesktopUEFI_Pflash {
local ACCEL=$( [ "$1" != "noaccel" ] && echo $(accel))
local TESTDIR="uefi_pflash$( [ "$1" != "noaccel" ] && echo "_accel" )"
download $LIVE_IMAGE_URL
testImageInDir $TESTDIR
# Fails with -accel whpx
# qemu-system-x86_64.exe: WHPX: Failed to emulate MMIO access with EmulatorReturnStatus: 2
# qemu-system-x86_64.exe: WHPX: Failed to exec a virtual processor
echo "Instanciate VARS-Firmware to VM dir for r/w pflash access:"
firmwareAvailable edk2-i386-vars.fd &&
(
echo "cp '$(firmware edk2-i386-vars.fd)' $TESTDIR/"
echo
cp "$(firmware edk2-i386-vars.fd)" $TESTDIR/
firmwareAvailable edk2-x86_64-code.fd &&
execute qemu-system-x86_64 -M q35 -m 1536 $ACCEL $(audioq35) \
	-drive "file=$(firmware edk2-x86_64-code.fd),if=pflash,format=raw,readonly=on" \
	-drive file=$TESTDIR/edk2-i386-vars.fd,if=pflash,format=raw,readonly=off \
	-cdrom $LIVE_IMAGE_FILE -drive file=$TESTDIR/testimage.qcow2,media=disk
)
removeDir $TESTDIR
}

# UEFI-Pflash-Desktop Accelerated (LiveImage)
function qemuLiveDesktopUEFI_Pflash_Noaccel {
	qemuLiveDesktopUEFI_Pflash noaccel
}

# UEFI-Bios-Desktop (LiveImage)
function qemuLiveDesktopUEFI_Bios {
local ACCEL=$( [ "$1" != "noaccel" ] && echo $(accel))
local TESTDIR="uefi_bios$( [ "$1" != "noaccel" ] && echo "_accel" )"
download $LIVE_IMAGE_URL
testImageInDir $TESTDIR
echo "Concatenate VARS- and CODE-Firmware in VM dir for usage as bios:"
firmwareAvailable edk2-i386-vars.fd edk2-x86_64-code.fd &&
(
echo "cat '$(firmware edk2-i386-vars.fd)' '$(firmware edk2-x86_64-code.fd)' > $TESTDIR/edk2-x86_64.fd"
echo
cat "$(firmware edk2-i386-vars.fd)" "$(firmware edk2-x86_64-code.fd)" > $TESTDIR/edk2-x86_64.fd
execute qemu-system-x86_64 -M q35 -m 1536 $ACCEL $(audioq35) \
	-bios $TESTDIR/edk2-x86_64.fd \
	-cdrom $LIVE_IMAGE_FILE -drive file=$TESTDIR/testimage.qcow2,media=disk
)
removeDir $TESTDIR
}

# UEFI-Bios-Desktop Accelerated (LiveImage)
function qemuLiveDesktopUEFI_Bios_Noaccel {
	qemuLiveDesktopUEFI_Bios noaccel
}

# SDL-Desktop (LiveImage)
function qemuLiveDesktopSDL {
download $LIVE_IMAGE_URL
testImageInDir sdl
execute qemu-system-x86_64 -M q35 $(accel) -m 1536 \
	-display sdl $(audioq35 hda-duplex) \
	-cdrom $LIVE_IMAGE_FILE -drive file=sdl/testimage.qcow2,media=disk
removeDir sdl
}

# GTK-Desktop (LiveImage)
function qemuLiveDesktopGTK {
download $LIVE_IMAGE_URL
testImageInDir gtk
execute qemu-system-x86_64 -M q35 $(accel) -m 1536 \
	-display gtk $(audioq35 hda-micro) \
	-cdrom $LIVE_IMAGE_FILE -drive file=gtk/testimage.qcow2,media=disk
removeDir gtk
}

# VNC-Desktop (LiveImage)
function qemuLiveDesktopVNC {
download $LIVE_IMAGE_URL
testImageInDir vnc
cygwinXlaunch
execute qemu-system-x86_64 -M q35 $(accel) -m 1536 -pidfile $PIDFILE \
	-display vnc=:05 -k de $(audioq35 hda-duplex) \
	-cdrom $LIVE_IMAGE_FILE -drive file=vnc/testimage.qcow2,media=disk &
executeVncForBackgroundQemu
removeDir vnc
}

# Spice-Desktop (LiveImage)
function qemuLiveDesktopSPICE {
download $LIVE_IMAGE_URL
testImageInDir spice
cygwinXlaunch
execute qemu-system-x86_64 -M q35 $(accel) -m 1536 -pidfile $PIDFILE $(audioq35 hda-micro) \
	-vga qxl -spice port=5905,addr=127.0.0.1,disable-ticketing=on \
	-device virtio-serial -chardev spicevmc,id=spicechannel0,name=vdagent \
	-device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 \
	-cdrom $LIVE_IMAGE_FILE -drive file=spice/testimage.qcow2,media=disk &
executeSpicyForBackgroundQemu
removeDir spice
}

# Desktop (for qemu-img)
function qemuLiveDesktopQemuImgOperations {
download $LIVE_IMAGE_URL
local ACCEL=$(accel)
local TESTDIR="qemu-img"
mkdir -p $TESTDIR
echo
toolexec qemu-img convert -f raw $LIVE_IMAGE_FILE -O qcow2 $TESTDIR/iso.qcow2
toolexec qemu-img snapshot -c CONVERTED $TESTDIR/iso.qcow2
toolexec qemu-img create -f qcow2 $TESTDIR/rebase.qcow2 20G
toolexec qemu-img rebase -F qcow2 -b rebase.qcow2 -f qcow2 $TESTDIR/iso.qcow2
toolexec qemu-img snapshot -c REBASED $TESTDIR/iso.qcow2
toolexec qemu-img commit $TESTDIR/iso.qcow2
toolexec qemu-img compare -f raw $LIVE_IMAGE_FILE -F qcow2 $TESTDIR/rebase.qcow2
toolexec qemu-system-x86_64 -M q35 -m 1536 $ACCEL $TESTDIR/rebase.qcow2
echo ">>> $TESTDIR/rebase.qcow2 was probably modified by qemu-system-x86_64 <<<"
toolexec qemu-img compare -f raw $LIVE_IMAGE_FILE -F qcow2 $TESTDIR/rebase.qcow2
toolexec qemu-img check $TESTDIR/rebase.qcow2
toolexec qemu-img snapshot -l $TESTDIR/iso.qcow2
toolexec qemu-img rebase -b '' -f qcow2 $TESTDIR/iso.qcow2
toolexec rm $TESTDIR/rebase.qcow2
toolexec qemu-img compare -f raw $LIVE_IMAGE_FILE -F qcow2 $TESTDIR/iso.qcow2
toolexec qemu-img check $TESTDIR/iso.qcow2
toolexec qemu-img snapshot -a REBASED $TESTDIR/iso.qcow2
toolexec qemu-img compare -f raw $LIVE_IMAGE_FILE -F qcow2 $TESTDIR/iso.qcow2
toolexec qemu-img snapshot -d REBASED $TESTDIR/iso.qcow2
toolexec qemu-img snapshot -l $TESTDIR/iso.qcow2
removeDir $TESTDIR
}

# Extended SDL-Desktop (HDImage)
function qemuInstalledDesktopSDL {
local IMAGE='d:\Qemu\test\test-usernet.qcow2'
[ -f "$IMAGE" ] || return 0
execute qemu-system-x86_64 -M q35 $(accel) -m 1G \
	-display sdl $(audioq35 hda-micro) \
	-netdev user,id=un0,hostfwd=tcp::2222-:22 -device virtio-net,netdev=un0 \
	-drive file="$IMAGE",media=disk,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0
}

# Extended GTK-Desktop (HDImage)
function qemuInstalledDesktopGTK {
local IMAGE='/d/Qemu/test/test-usernet.qcow2'
[ -f "$IMAGE" ] || return 0
execute qemu-system-x86_64 -M q35 $(accel) -m 1G \
	-display gtk $(audioq35 hda-duplex) \
	-netdev user,id=un0,hostfwd=tcp::2222-:22 -device virtio-net,netdev=un0 \
	-drive file="$IMAGE",media=disk,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0
}

# Extended VNC-Desktop (HDImage)
function qemuInstalledDesktopVNC1 {
local IMAGE='\Qemu\test\test-usernet.qcow2'
[ -f "$IMAGE" ] || return 0
cygwinXlaunch
execute qemu-system-x86_64 -M q35 $(accel) -m 1G -pidfile $PIDFILE \
	-display vnc=:05 -k de $(audioq35 hda-micro) \
	-netdev user,id=un0,hostfwd=tcp::2222-:22 -device virtio-net,netdev=un0 \
	-drive file="$IMAGE",media=disk,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0 &
executeVncForBackgroundQemu
}

# Extended VNC-Desktop (HDImage) with tap-Network and host configuation of tapDevices
function qemuInstalledDesktopVNC2 {
local IMAGE='d:\Qemu\test\test.qcow2'
[ -f "$IMAGE" ] || return 0
cygwinXlaunch
execute qemu-system-x86_64 -M q35 $(accel) -m 1G -pidfile $PIDFILE \
	-display vnc=:05 -k de $(audioq35 hda-micro) \
	-netdev tap,ifname=qemuTap05,id=tap0 -device virtio-net,netdev=tap0,mac=00:00:00:00:00:05 \
	-drive file="$IMAGE",media=disk,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0 &
executeVncForBackgroundQemu
}

# Extended Spice-Desktop (HDImage)
function qemuInstalledDesktopSPICE1 {
local IMAGE='/d/Qemu/test/test-usernet.qcow2'
[ -f "$IMAGE" ] || return 0
cygwinXlaunch
execute qemu-system-x86_64 -M q35 $(accel) -m 1G -pidfile $PIDFILE \
	-vga qxl -spice port=5905,addr=127.0.0.1,disable-ticketing=on $(audioq35 hda-duplex) \
	-device virtio-serial -chardev spicevmc,id=spicechannel0,name=vdagent \
	-device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 \
	-usb -device qemu-xhci \
	-chardev spicevmc,id=usbredirchardev0,name=usbredir \
	-device usb-redir,chardev=usbredirchardev0,id=usbredirdevice0 \
	-netdev user,id=un0,hostfwd=tcp::2222-:22 -device virtio-net,netdev=un0 \
	-drive file="$IMAGE",media=disk,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0 &
executeSpicyForBackgroundQemu
}

# Extended Spice-Desktop (HDImage) with tap-Network and host configuation of tapDevices
function qemuInstalledDesktopSPICE2 {
local IMAGE='/d/Qemu/test/test.qcow2'
[ -f "$IMAGE" ] || return 0
cygwinXlaunch
execute qemu-system-x86_64 -M q35 $(accel) -m 1G -pidfile $PIDFILE \
	-vga qxl -spice port=5905,addr=127.0.0.1,disable-ticketing=on $(audioq35 hda-duplex) \
	-device virtio-serial -chardev spicevmc,id=spicechannel0,name=vdagent \
	-device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 \
	-usb -device qemu-xhci \
	-chardev spicevmc,id=usbredirchardev0,name=usbredir \
	-device usb-redir,chardev=usbredirchardev0,id=usbredirdevice0 \
	-netdev tap,ifname=qemuTap05,id=tap0 -device virtio-net,netdev=tap0,mac=00:00:00:00:00:05 \
	-drive file="$IMAGE",media=disk,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0 &
executeSpicyForBackgroundQemu
}

function qemu2020day01 {
download https://www.qemu-advent-calendar.org/2020/download/day01.tar.gz
tar -xf day01.tar.gz
cat day01/adv-cal.txt
execute qemu-system-i386 -net none -drive file=day01/tweetboot.img,format=raw,if=floppy
removeDir day01
}

function qemu2020day03 {
download https://www.qemu-advent-calendar.org/2020/download/gw-basic.tar.xz
tar -xf gw-basic.tar.xz
cat gw-basic/README
execute qemu-system-i386 -m 16M -drive if=ide,format=qcow2,file=gw-basic/gwbasic.qcow2
removeDir gw-basic
}

function qemu2020day04 {
download https://www.qemu-advent-calendar.org/2020/download/day04.tar.gz
tar -xf day04.tar.gz
cat bootRogue/README
execute qemu-system-x86_64 -drive format=raw,file=bootRogue/rogue.img
removeDir bootRogue
}

function qemu2020day05 {
download https://www.qemu-advent-calendar.org/2020/download/day05.tar.gz
tar -xf day05.tar.gz
cat lights/README
execute qemu-system-x86_64 -drive format=raw,file=lights/lights.img
removeDir lights
}

function qemu2020day06 {
download https://www.qemu-advent-calendar.org/2020/download/day06.tar.gz
tar -xf day06.tar.gz
cat day06/adv-cal.txt
execute qemu-system-i386 -net none -drive file=day06/bootmine.img,format=raw,if=floppy
removeDir day06
}

function qemu2020day07 {
download https://www.qemu-advent-calendar.org/2020/download/day07.tar.gz
tar -xf day07.tar.gz
cat day07/README
execute qemu-system-x86_64 -drive file=day07/visopsys-0.9-usb.img,format=raw -nic model=ne2k_pci
removeDir day07
}

function qemu2020day08 {
download https://www.qemu-advent-calendar.org/2020/download/day08.tar.gz
tar -xf day08.tar.gz
cat day08/README
execute qemu-system-x86_64 -drive file=day08/fountain.bin,format=raw
removeDir day08
}

function qemu2020day09 {
download https://www.qemu-advent-calendar.org/2020/download/day09.tar.xz
tar -xf day09.tar.xz
cat RayTracing_QAdvent2020/README
local BYTES=$( egrep "(^[0-9 ]*$|@0x1fe)" RayTracing_QAdvent2020/run.sh )
local POS=0 LINE="" BYTE="" BYTEREGEX="^[0-9]+$" IMAGE="RayTracing_QAdvent2020/floppy"
echo
echo "Writing boot sector to $IMAGE ..."
for BYTE in $BYTES ; do
	if [[ $BYTE =~ $BYTEREGEX ]] ; 	then
		(( POS%32 == 0 )) && echo "$LINE" && LINE=""
		BYTE=$(printf '%02x' $BYTE)
		LINE="$LINE$( (( POS%2 == 0 )) && echo " " )$BYTE"
		echo -ne \\x$BYTE | dd of=$IMAGE seek=$POS bs=1 count=1 2> /dev/null
		POS=$(( POS+1 ))
	else
		while (( POS < 510 )) ; do
			(( POS%32 == 0 )) && echo "$LINE" && LINE=""
			LINE="$LINE$( (( POS%2 == 0 )) && echo " " ).."
			POS=$(( POS+1 ))
		done
	fi
done
echo "$LINE"
echo
execute qemu-system-x86_64 -drive file=$IMAGE,format=raw,if=floppy
removeDir RayTracing_QAdvent2020
}

function qemu2020day11 {
download https://www.qemu-advent-calendar.org/2020/download/milky.tar.gz
download https://milkymist.walle.cc/updates/2012-03-01/flickernoise
tar -xf milky.tar.gz
cat milky/readme.txt
execute qemu-system-lm32 -M milkymist -kernel flickernoise
removeDir milky
}

function qemu2020day12 {
download https://www.qemu-advent-calendar.org/2020/download/day12.tar.gz
tar -xf day12.tar.gz
cat gameoflife/README.md
execute qemu-system-x86_64 -drive format=raw,file=gameoflife/gameoflife.bin
removeDir gameoflife
}

function qemu2020day13 {
download https://www.qemu-advent-calendar.org/2020/download/day13.tar.xz
tar -xf day13.tar.xz
cat Invaders_QEMUAdvent2020/README
execute qemu-system-x86_64 -m 10M -drive file=Invaders_QEMUAdvent2020/invaders.img,format=raw,if=floppy
removeDir Invaders_QEMUAdvent2020
}

function qemu2020day14 {
download https://www.qemu-advent-calendar.org/2020/download/day14.tar.xz
tar -xf day14.tar.xz
cat day14/README
execute qemu-system-x86_64 -drive file=day14/eggos.img,if=virtio \
	-net user,hostfwd=::19220-:22 -net nic,model=e1000
removeDir day14
}

function qemu2020day15 {
download https://www.qemu-advent-calendar.org/2020/download/day15.tar.gz
tar -xf day15.tar.gz
cat day15/README
execute qemu-system-x86_64 -m 256M -machine q35 -monitor none -vga none \
	-drive if=pflash,format=raw,file=day15/snow.bin,readonly=on -boot a
removeDir day15
}

function qemu2020day16 {
download https://www.qemu-advent-calendar.org/2020/download/day16.tar.gz
download https://eldondev.com/openwrt-privoxy-qcow.img
tar -xf day16.tar.gz
cat day16/README
execute qemu-system-x86_64 $(accel) -drive file=openwrt-privoxy-qcow.img,id=d0,if=none \
	-snapshot -device ide-hd,drive=d0,bus=ide.0 \
	-netdev user,id=hn0 -device e1000,netdev=hn0,id=nic1 \
	-netdev user,id=hn1,hostfwd=tcp::18118-:8118 -device e1000,netdev=hn1,id=nic2
removeDir day16
}

function qemu2020day17 {
download https://www.qemu-advent-calendar.org/2020/download/day17.tar.gz
tar -xf day17.tar.gz
cat creek/adv-cal.txt
execute qemu-system-ppc -monitor none -parallel none -M mpc8544ds -kernel creek/creek.bin
removeDir creek
}

function qemu2020day18 {
download https://www.qemu-advent-calendar.org/2020/download/day18.tar.gz
tar -xf day18.tar.gz
cat doom/README
execute qemu-system-x86_64 -drive file=doom/doom.img,format=raw
removeDir doom
}

function qemu2020day19 {
download https://www.qemu-advent-calendar.org/2020/download/day19.tar.gz
tar -xf day19.tar.gz
cat aflatoxin/adv-cal.txt
execute qemu-system-i386 -net none $(pcspk) -drive file=aflatoxin/AFLAtoxin.bin,format=raw,if=floppy
removeDir aflatoxin
}

function qemu2020day20 {
download https://www.qemu-advent-calendar.org/2020/download/day20.tar.gz
tar -xf day20.tar.gz
cat kpara8/adv-cal.txt
execute qemu-system-i386 -net none -drive file=kpara8/kpara8.bin,format=raw,if=floppy
removeDir kpara8
}

function qemu2020day21 {
download https://www.qemu-advent-calendar.org/2020/download/day21.tar.gz
tar -xf day21.tar.gz
cat day21/README
execute qemu-system-x86_64 -kernel day21/vmlinuz -initrd day21/initramfs.linux_amd64.cpio \
	-append ip=dhcp -nic user,hostfwd=tcp::10222-:22
removeDir day21
}

function qemu2020day22 {
download https://www.qemu-advent-calendar.org/2020/download/day22.tar.xz
tar -xf day22.tar.xz
cat day22/README
execute qemu-system-x86_64 $(accel) -m 1G -drive if=virtio,file=day22/ventoy.qcow2
removeDir day22
}

function qemu2020day23 {
download https://www.qemu-advent-calendar.org/2020/download/day23.tar.gz
tar -xf day23.tar.gz
cat day23/README
execute qemu-system-x86_64 -L day23/bios -nodefaults -name ELKS -machine isapc -cpu 486,tsc \
	-m 1M -vga std -rtc base=utc \
	-netdev user,id=mynet,hostfwd=tcp::8080-:80,hostfwd=tcp::2323-:23 \
	-device ne2k_isa,irq=12,netdev=mynet -drive if=ide,format=raw,file=day23/hd32mbr-fat.bin \
	-drive if=ide,format=qcow2,file=day23/scratch.qcow2
removeDir day23
}

function qemu2020day24 {
download https://www.qemu-advent-calendar.org/2020/download/hippo.tar.gz
tar -xf hippo.tar.gz
cat hippo/adv-cal.txt
execute qemu-system-ppc64 -monitor none -parallel none -M virtex-ml507 -m 512 \
	-dtb hippo/virtex440-ml507.dtb -kernel hippo/hippo.linux
removeDir hippo
}

function qemu2018day01 {
download https://www.qemu-advent-calendar.org/2018/download/day01.tar.xz
tar -xf day01.tar.xz
cat day01/adv-cal.txt
execute qemu-system-i386 -net none $(pcspk) -drive file=day01/fbird.img,format=raw,if=floppy
removeDir day01
}

function qemu2018day02 {
download https://www.qemu-advent-calendar.org/2018/download/day02.tar.xz
tar -xf day02.tar.xz
cat day02/adv-cal.txt
execute qemu-system-xtensa -net none -monitor none -parallel none -M lx60 -cpu dc233c \
	-kernel day02/santas-sleigh-ride.elf
removeDir day02
}

function qemu2018day03 {
download https://www.qemu-advent-calendar.org/2018/download/day03.tar.xz
tar -xf day03.tar.xz
cat day03/readme.txt
execute qemu-system-x86_64 -m 1G $(accel) -netdev user,id=net0,bootfile=http://boot.netboot.xyz \
	-device virtio-net-pci,netdev=net0 -boot n
removeDir day03
}

function qemu2018day04 {
download https://www.qemu-advent-calendar.org/2018/download/day04.tar.xz
tar -xf day04.tar.xz
cat day04/readme.txt
execute qemu-system-ppc64 --net none --boot order=d,strict=on -g 800x600x8 --cdrom day04/snake.iso
removeDir day04
}

function qemu2018day05 {
download https://www.qemu-advent-calendar.org/2018/download/day05.tar.xz
tar -xf day05.tar.xz
cat day05/readme.txt
execute qemu-system-i386 -drive file=day05/pc-mos.img,format=raw,if=floppy \
	$(pcspk) -rtc base=1994-12-05T09:00:00
removeDir day05
}

function qemu2018day06 {
download https://www.qemu-advent-calendar.org/2018/download/day06.tar.xz
tar -xf day06.tar.xz
cat day06/adv-cal.txt
execute qemu-system-arm -net none -parallel none -M versatilepb -kernel day06/120_below.zImage \
	-dtb day06/versatile-pb.dtb
removeDir day06
}

function qemu2018day07 {
download https://www.qemu-advent-calendar.org/2018/download/day07.tar.xz
tar -xf day07.tar.xz
cat day07/adv-cal.txt
execute qemu-system-m68k -monitor none -parallel none -M mcf5208evb -kernel day07/sanity-clause.elf
removeDir day07
}

function qemu2018day08 {
download https://www.qemu-advent-calendar.org/2018/download/day08.tar.xz
tar -xf day08.tar.xz
cat day08/readme.txt
execute qemu-system-i386 -m 32 -M isapc $(accel) -cpu pentium -no-acpi $(pcspk) \
	-net nic,model=ne2k_isa -net user -drive if=ide,file=day08/hd.qcow2
removeDir day08
}

function qemu2018day09 {
download https://www.qemu-advent-calendar.org/2018/download/day09.tar.xz
tar -xf day09.tar.xz
cat day09/adv-cal.txt
execute qemu-system-sh4 -monitor none -parallel none -net none -M r2d \
	-kernel day09/zImage -append loglevel=3
removeDir day09
}

function qemu2018day10 {
download https://www.qemu-advent-calendar.org/2018/download/day10.tar.xz
tar -xf day10.tar.xz
cat day10/readme.txt
execute qemu-system-i386 -net none -M q35 $(accel) -cdrom day10/gamebro.iso
removeDir day10
}

function qemu2018day11 {
download https://www.qemu-advent-calendar.org/2018/download/day11.tar.xz
tar -xf day11.tar.xz
cat day11/adv-cal.txt
execute qemu-system-sparc -monitor none -parallel none -net none -M SS-20 -m 256 -kernel day11/zImage.elf
removeDir day11
}

function qemu2018day13 {
download https://www.qemu-advent-calendar.org/2018/download/day13.tar.xz
tar -xf day13.tar.xz
cat day13/adv-cal.txt
execute qemu-system-mips -net none -parallel none -M malta -kernel day13/vmlinux \
	-device usb-kbd -device usb-mouse -vga cirrus $(audio ES1370)
removeDir day13
}

function qemu2018day14 {
download https://www.qemu-advent-calendar.org/2018/download/day14.tar.xz
tar -xf day14.tar.xz
cat day14/adv-cal.txt
execute qemu-system-nios2 -monitor none -parallel none -net none -kernel day14/vmlinux.elf
removeDir day14
}

function qemu2018day15 {
download https://www.qemu-advent-calendar.org/2018/download/day15.tar.xz
tar -xf day15.tar.xz
cat day15/adv-cal.txt
execute qemu-system-ppc -net none -parallel none -monitor none -M g3beige -kernel day15/invaders.elf
removeDir day15
}

function qemu2018day16 {
download https://www.qemu-advent-calendar.org/2018/download/day16.tar.xz
tar -xf day16.tar.xz
cat day16/adv-cal.txt
execute qemu-system-aarch64 -net none -parallel none -monitor none -M vexpress-a9 \
	-kernel day16/winter.zImage -dtb day16/vexpress-v2p-ca9.dtb
removeDir day16
}

function qemu2018day17 {
download https://www.qemu-advent-calendar.org/2018/download/day17.tar.xz
tar -xf day17.tar.xz
cat day17/adv-cal.txt
execute qemu-system-microblaze -monitor none -parallel none -kernel day17/ballerina.bin
removeDir day17
}

function qemu2018day18 {
download https://www.qemu-advent-calendar.org/2018/download/day18.tar.xz
tar -xf day18.tar.xz
cat day18/adv-cal.txt
execute qemu-system-arm -M canon-a1100 -net none -monitor none \
	-bios day18/barebox.canon-a1100.bin
removeDir day18
}

function qemu2018day19 {
download https://www.qemu-advent-calendar.org/2018/download/day19.tar.xz
tar -xf day19.tar.xz
cat day19/adv-cal.txt
execute qemu-system-ppc64 -device VGA -monitor none -M ppce500 -cpu e5500 -net none \
	-device pci-ohci -device usb-kbd -kernel day19/uImage
removeDir day19
}

function qemu2018day20 {
download https://www.qemu-advent-calendar.org/2018/download/day20.tar.xz
tar -xf day20.tar.xz
cat day20/adv-cal.txt
execute qemu-system-or1k -net none -monitor none -parallel none -kernel day20/vmlinux
removeDir day20
}

function qemu2018day21 {
download https://www.qemu-advent-calendar.org/2018/download/day21.tar.xz
tar -xf day21.tar.xz
cat day21/README.txt
execute qemu-system-aarch64 -kernel day21/bootstrap.elf -monitor none -cpu cortex-a57 -m 1024 \
	-net none -M virt,virtualization=true
removeDir day21
}

function qemu2018day22 {
download https://www.qemu-advent-calendar.org/2018/download/day22.tar.xz
tar -xf day22.tar.xz
cat day22/adv-cal.txt
export MSYS2_ARG_CONV_EXCL='*'
execute qemu-system-mips64 -net none -parallel none -M malta $(audio ES1370) \
        -device usb-kbd -device usb-mouse -device cirrus-vga,vgamem_mb=16 \
        -hda day22/ri-li.qcow2 -kernel day22/vmlinux -append root=/dev/hda
unset MSYS2_ARG_CONV_EXCL
removeDir day22
}

function qemu2018day23 {
download https://www.qemu-advent-calendar.org/2018/download/day23.tar.xz
tar -xf day23.tar.xz
cat day23/adv-cal.txt
execute qemu-system-sparc64 -net none -parallel none -kernel day23/vmlinux
removeDir day23
}

function qemu2018day24 {
download https://www.qemu-advent-calendar.org/2018/download/day24.tar.xz
tar -xf day24.tar.xz
cat day24/adv-cal.txt
execute qemu-system-riscv64 -M virt -device virtio-gpu-device,xres=1600,yres=900 \
	-device virtio-keyboard-device -device virtio-tablet-device -serial stdio \
	-kernel day24/risk-v.elf
removeDir day24
}

function qemu2016day01 {
download https://www.qemu-advent-calendar.org/2016/download/day01.tar.xz
tar -xf day01.tar.xz
cat mikeos/readme.txt
execute qemu-system-i386 -drive file=mikeos/mikeos.flp,format=raw,if=floppy $(pcspk)
removeDir mikeos
}

function qemu2016day02 {
download https://www.qemu-advent-calendar.org/2016/download/day02.tar.xz
download https://prdownloads.sourceforge.net/syllable/SyllableDesktop-0.6.6.i586.VM.7z?download \
	SyllableDesktop-0.6.6.i586.VM.7z
tar -xf day02.tar.xz
cat syllable/readme.txt
(
cd syllable
7z x -y ../SyllableDesktop-0.6.6.i586.VM.7z > /dev/null
execute qemu-system-i386 -m 512 -vga std $(audio ES1370) -hda "Syllable 0.6.6/Syllable.vmdk"
)
removeDir syllable
}

function qemu2016day03 {
download https://www.qemu-advent-calendar.org/2016/download/day03.tar.xz
tar -xf day03.tar.xz
cat freegem/readme.txt
execute qemu-system-i386 $(accel) -m 32 -hda freegem/freegem.qcow2 $(pcspk)
removeDir freegem
}

function qemu2016day04 {
download https://www.qemu-advent-calendar.org/2016/download/day04.tar.xz
#download https://sourceforge.net/projects/reactos/files/ReactOS/0.4.3/ReactOS-0.4.3-live.zip/download \
#       ReactOS-0.4.3-live.zip
download https://iso.reactos.org/livecd/reactos-livecd-0.4.15-dev-2574-g18e95f5-x86-gcc-lin-dbg.7z
tar -xf day04.tar.xz
cat reactos/readme.txt
echo "Version 0.4.15-dev-2574-g18e95f5 replaces QEMU Advent Calender Version 0.4.3!"
echo "see https://jira.reactos.org/browse/CORE-16695"
echo "see https://reactos.org/wiki/QEMU"
echo
(
cd reactos
7z x -y ../reactos-livecd-0.4.15-dev-2574-g18e95f5-x86-gcc-lin-dbg.7z > /dev/null
execute qemu-system-i386 $(accel) -m 512 -net nic,model=rtl8139 -net user -vga std $(audio AC97) \
	-usb -device usb-tablet -serial file:reactos.log \
	-cdrom reactos-livecd-0.4.15-dev-2574-g18e95f5-x86-gcc-lin-dbg.iso
)
removeDir reactos
}

function qemu2016day05 {
download https://www.qemu-advent-calendar.org/2016/download/day05.tar.xz
tar -xf day05.tar.xz
cat hanoi/readme.txt
execute qemu-system-ppc64 --boot order=d,strict=on -g 800x600x8 --cdrom hanoi/hanoi.iso
removeDir hanoi
}

function qemu2016day06 {
download https://www.qemu-advent-calendar.org/2016/download/day06.tar.xz
download https://prdownloads.sourceforge.net/menuet/M32-086.ZIP?download \
	M32-086.ZIP
tar -xf day06.tar.xz
cat menuet32/readme.txt
(
cd menuet32
unzip -o -q ../M32-086.ZIP
execute qemu-system-i386 $(accel) $(audio AC97) -m 512 -drive file=M32-086.IMG,format=raw,if=floppy
)
removeDir menuet32
}

function qemu2016day07 {
download https://www.qemu-advent-calendar.org/2016/download/day07.tar.xz
tar -xf day07.tar.xz
cat sorry-ass/readme.txt
execute qemu-system-i386 -drive file=sorry-ass/sorryass.bin,format=raw,if=floppy $(pcspk)
removeDir sorry-ass
}

function qemu2016day08 {
download https://www.qemu-advent-calendar.org/2016/download/day08.tar.xz
# to demo resume of pminvaders, don't overwrite existing
local EXISTS=""
[ -d pminvaders ] && EXISTS=1
[ -z "$EXISTS" ] && tar -xf day08.tar.xz
cat pminvaders/description
export MSYS2_ARG_CONV_EXCL='*'
local DISK_IMG="pminvaders/main.raw"
local DISK_SIZE=$(stat -c "%s" "$DISK_IMG")
local NVDIMM_IMG="pminvaders/nvdimm.img"
[ -z "$EXISTS" ] && dd if=/dev/zero of=$NVDIMM_IMG bs=1G count=1 > /dev/null 2>&1
[ -z "$EXISTS" ] && /usr/sbin/mke2fs -q -t ext4 $NVDIMM_IMG
local NVDIMM_SIZE=$(stat -c "%s" "$NVDIMM_IMG")
execute qemu-system-x86_64 $(accel) -machine pc,nvdimm=on -smp 4 -cpu kvm64 -m 2G,slots=4,maxmem=4G \
    -object memory-backend-file,id=mem1,share=off,mem-path=$DISK_IMG,size=$DISK_SIZE \
    -device nvdimm,id=nv1,memdev=mem1 \
    -object memory-backend-file,id=mem2,share=on,mem-path=$NVDIMM_IMG,size=$NVDIMM_SIZE \
    -device nvdimm,id=nv2,memdev=mem2 \
    -kernel pminvaders/vmlinuz-4.8.7 -append "root=/dev/pmem0p1 ro console=ttyS0,115200"
unset MSYS2_ARG_CONV_EXCL
removeDir pminvaders
}

function qemu2016day09 {
download https://www.qemu-advent-calendar.org/2016/download/day09-v2.tar.xz
tar -xf day09-v2.tar.xz
cat kolibrios/readme.txt
execute qemu-system-i386 $(accel) -device e1000,netdev=u0 -netdev user,id=u0 -usb $(audio hda-duplex) \
        -boot d -cdrom kolibrios/kolibri-v2.iso
removeDir kolibrios
}

function qemu2016day10 {
download https://www.qemu-advent-calendar.org/2016/download/day10.tar.xz
tar -xf day10.tar.xz
cat epic-pinball/readme.txt
execute qemu-system-x86_64 epic-pinball/freedos2016.qcow2 $(audio sb16) \
	-vga std,retrace=precise -display sdl
removeDir epic-pinball
}

function qemu2016day11 {
download https://www.qemu-advent-calendar.org/2016/download/day11.tar.xz
tar -xf day11.tar.xz
cat genode/README
execute qemu-system-i386 -serial stdio -cdrom genode/Genode_on_seL4.iso -m 1G -vga cirrus $(accel)
removeDir genode
}

function qemu2016day12 {
download https://www.qemu-advent-calendar.org/2016/download/day12.tar.xz
tar -xf day12.tar.xz
cat tetros/readme.txt
execute qemu-system-i386 $(accel) -m 32 -drive if=ide,file=tetros/tetros.img,format=raw
removeDir tetros
}

function qemu2016day13 {
download https://www.qemu-advent-calendar.org/2016/download/day13.tar.xz
tar -xf day13.tar.xz
cat supernested/readme.txt
removeDir supernested
}

function qemu2016day14 {
download https://www.qemu-advent-calendar.org/2016/download/day14.tar.xz
tar -xf day14.tar.xz
cat acorn/readme.txt
execute qemu-system-x86_64 $(accel) -net nic,model=virtio -net user,hostfwd=tcp::8080-:80 \
	 -smp 4 -serial stdio -m 128 -drive file=acorn/acorn.img,format=raw,if=ide -k en-us
removeDir acorn
}

function qemu2016day15 {
download https://www.qemu-advent-calendar.org/2016/download/day15.tar.xz
tar -xf day15.tar.xz
cat ofpong/readme.txt
execute qemu-system-ppc64 -M pseries-2.1 -boot order=d,strict=on -cdrom ofpong/ofpong.iso
removeDir ofpong
}

function qemu2016day16 {
download https://www.qemu-advent-calendar.org/2016/download/day16.tar.xz
tar -xf day16.tar.xz
cat tccboot/readme.txt
execute qemu-system-x86_64 $(accel) -m 512 -net nic,model=rtl8139 -net user -vga std \
	$(audio AC97) -cdrom tccboot/tccboot.iso
removeDir tccboot
}

function qemu2016day17 {
download https://www.qemu-advent-calendar.org/2016/download/day17.tar.xz
tar -xf day17.tar.xz
cat minoca-os/readme.txt
execute qemu-system-x86_64 minoca-os/minoca-os.qcow2
removeDir minoca-os
}

function qemu2016day18 {
download https://www.qemu-advent-calendar.org/2016/download/day18.tar.xz
download https://github.com/redox-os/redox/releases/download/0.0.5/harddrive.bin.gz
tar -xf day18.tar.xz
cat redox/readme.txt
(
cd redox
zcat ../harddrive.bin.gz > harddrive.bin
execute qemu-system-i386 -M q35 $(accel) -vga std $(audio AC97) -smp 4 -m 1024 \
     -net nic,model=e1000 -net user -drive file=harddrive.bin,format=raw
)
removeDir redox
}

function qemu2016day19 {
download https://www.qemu-advent-calendar.org/2016/download/day19.tar.xz
tar -xf day19.tar.xz
cat bootchess/readme.txt
execute qemu-system-i386 -drive file=bootchess/BootChess.bin,format=raw,if=floppy
removeDir bootchess
}

function qemu2016day20 {
download https://www.qemu-advent-calendar.org/2016/download/day20.tar.xz
download https://prdownloads.sourceforge.net/open-beos/haiku-r1alpha4.1-vmdk.tar.xz?download \
	haiku-r1alpha4.1-vmdk.tar.xz
tar -xf day20.tar.xz
cat haiku/readme.txt
(
cd haiku
tar -xf ../haiku-r1alpha4.1-vmdk.tar.xz
qemu-img convert -f vmdk haiku-r1alpha4.vmdk -O qcow2 haiku-r1alpha4.qcow2
execute qemu-system-i386 $(accel) $(audio hda-duplex) -m 512 \
	-hda haiku-r1alpha4.qcow2 -hdb blank-bfs-2048mb.vmdk
)
removeDir haiku
}

function qemu2016day21 {
download https://www.qemu-advent-calendar.org/2016/download/day21.tar.xz
tar -xf day21.tar.xz
cat wireguard/readme.txt
execute qemu-system-i386 -nodefaults -machine q35 $(accel) -smp 2 -m 96M -monitor none \
        -kernel wireguard/wireguard-test-4f257956-d81f-43f3-8fd8-1475360f58b8.kernel -append console=hvc0
removeDir wireguard
}
		
function qemu2016day22 {
download https://www.qemu-advent-calendar.org/2016/download/day22.tar.xz
tar -xf day22.tar.xz
cat trianglix/readme.txt
execute qemu-system-x86_64 $(accel) -m 1024 -vga std -drive file=trianglix/trianglix.qcow2,format=qcow2
removeDir trianglix
}

function qemu2016day23 {
download https://www.qemu-advent-calendar.org/2016/download/day23.tar.xz
tar -xf day23.tar.xz
cat zx-spectrum/readme.txt
execute qemu-system-x86_64 $(accel) $(audio ES1370) -drive if=ide,file=zx-spectrum/zxspectrum.qcow2
removeDir zx-spectrum
}

function qemu2016day24 {
download https://www.qemu-advent-calendar.org/2016/download/day24.tar.xz
tar -xf day24.tar.xz
cat day24/readme.txt
(
	cd day24
	execute qemu-system-i386 $(accel) $(audio AC97) -device VGA,addr=07.0 \
		-kernel kernel -initrd null,null,null,music.ogg
)
removeDir day24
}

function qemu2014day24 {
download https://www.qemu-advent-calendar.org/2014/download/day24.tar.xz
tar -xf day24.tar.xz
extractReadme day24/run
(
	cd day24
	execute qemu-system-i386 $(accel) -kernel kernel \
		-initrd null,null,null,music.ogg,win.ogg,loss.ogg $(audio AC97) -vga std
)
removeDir day24
}

function qemu2014day23 {
download https://www.qemu-advent-calendar.org/2014/download/pebble-qemu-preview.tar.xz
tar -xf pebble-qemu-preview.tar.xz
extractReadme pebble-qemu-preview/run
cat pebble-qemu-preview/README
execute qemu-system-x86_64 $(accel) -rtc base=localtime -vga std -m 256 -usb \
	-hda pebble-qemu-preview/pebble_qemu_preview.vdi
removeDir pebble-qemu-preview
}

function qemu2014day22 {
download https://www.qemu-advent-calendar.org/2014/download/s390-moon-buggy.tar.xz
tar -xf s390-moon-buggy.tar.xz
extractReadme s390-moon-buggy/run
execute qemu-system-s390x -M s390-ccw-virtio -monitor none -kernel s390-moon-buggy/s390-bb.kernel \
	-initrd s390-moon-buggy/s390-moon-buggy.initrd
removeDir s390-moon-buggy
}

function qemu2014day21 {
download https://www.qemu-advent-calendar.org/2014/download/boundvariable.tar.xz
tar -xf boundvariable.tar.xz
extractReadme boundvariable/run
execute qemu-system-i386 $(accel) -m 1024 \
	-drive if=virtio,file=boundvariable/boundvariable.qcow2,format=qcow2
removeDir boundvariable
}

function qemu2014day20 {
download https://www.qemu-advent-calendar.org/2014/download/helenos.tar.xz
tar -xf helenos.tar.xz
extractReadme helenos/run
execute qemu-system-x86_64 $(accel) -net nic,model=e1000 \
	-net user,hostfwd=::2223-:2223,hostfwd=::8080-:8080 \
	-usb $(audio hda-duplex) -boot d -cdrom helenos/HelenOS-0.6.0-rc3-amd64.iso
removeDir helenos
}

function qemu2014day19 {
download https://www.qemu-advent-calendar.org/2014/download/mandelbrot.tar.xz
tar -xf mandelbrot.tar.xz
extractReadme mandelbrot/run
execute qemu-system-ppc64 -M mac99 -drive file=mandelbrot/mandelbrot.raw,format=raw
removeDir mandelbrot
}

function qemu2014day18 {
download https://www.qemu-advent-calendar.org/2014/download/ceph.tar.xz
tar -xf ceph.tar.xz
extractReadme ceph/run
execute qemu-system-x86_64 $(accel) -m 1024M -drive file=ceph/ceph.qcow2,format=qcow2 \
	-netdev user,id=net0,hostfwd=tcp::10022-:22 -device virtio-net-pci,netdev=net0
removeDir ceph
}

function qemu2014day17 {
download https://www.qemu-advent-calendar.org/2014/download/bb_debian.tar.xz
tar -xf bb_debian.tar.xz
extractReadme bb_debian/run
execute qemu-system-i386 $(accel) -m 512 -vga std $(audio hda-duplex) bb_debian/bb_debian.qcow2
removeDir bb_debian
}

function qemu2014day16 {
download https://www.qemu-advent-calendar.org/2014/download/tempest-showroom.tar.xz
tar -xf tempest-showroom.tar.xz
extractReadme tempest-showroom/run
execute qemu-system-i386 $(accel) -cdrom tempest-showroom/tempest-showroom_v0.9.7.iso
removeDir tempest-showroom
}

function qemu2014day15 {
download https://www.qemu-advent-calendar.org/2014/download/plan9.tar.xz
tar -xf plan9.tar.xz
extractReadme plan9/run
execute qemu-system-i386 $(accel) -m 1024 plan9/plan9.qcow2
removeDir plan9
}

function qemu2014day14 {
download https://www.qemu-advent-calendar.org/2014/download/invaders.tar.xz
tar -xf invaders.tar.xz
extractReadme invaders/run
execute qemu-system-x86_64 $(accel) -kernel invaders/invaders.exec
removeDir invaders
}

function qemu2014day13 {
download https://www.qemu-advent-calendar.org/2014/download/2nd-reality.tar.xz
tar -xf 2nd-reality.tar.xz
extractReadme 2nd-reality/run
execute qemu-system-i386 -vga std,retrace=precise $(audio gus) 2nd-reality/2nd-reality.qcow2
removeDir 2nd-reality
}

function qemu2014day12 {
download https://www.qemu-advent-calendar.org/2014/download/oberon.tar.xz
tar -xf oberon.tar.xz
extractReadme oberon/run
execute qemu-system-i386 $(accel) oberon/oberon.qcow2
removeDir oberon
}

function qemu2014day11 {
download https://www.qemu-advent-calendar.org/2014/download/osv-redis.tar.xz
tar -xf osv-redis.tar.xz
extractReadme osv-redis/run
execute qemu-system-x86_64 $(accel) -m 256 \
	-netdev user,id=user0,hostfwd=tcp::18000-:8000,hostfwd=tcp::16379-:6379 \
	-device virtio-net-pci,netdev=user0 osv-redis/osv-redis-memonly-v0.16.qemu.qcow2
removeDir osv-redis
}

function qemu2014day10 {
download https://www.qemu-advent-calendar.org/2014/download/512.tar.xz
tar -xf 512.tar.xz
extractReadme 512/run
execute qemu-system-x86_64 -cpu Nehalem $(accel) -vga std $(pcspk) \
	-drive file=512/512.img,if=floppy,format=raw
removeDir 512
}

function qemu2014day09 {
download https://www.qemu-advent-calendar.org/2014/download/ubuntu-core-alpha.tar.xz
tar -xf ubuntu-core-alpha.tar.xz
extractReadme ubuntu-core-alpha/run
execute qemu-system-x86_64 $(accel) -m 1024 \
	-drive if=virtio,file=ubuntu-core-alpha/ubuntu-core-alpha-01.img,format=qcow2 \
	-netdev user,id=user0,hostfwd=tcp::18000-:80,hostfwd=tcp::12222-:22 \
	-device virtio-net-pci,netdev=user0
removeDir ubuntu-core-alpha
}

function qemu2014day08 {
download https://www.qemu-advent-calendar.org/2014/download/qemu-xmas-uefi-zork.tar.xz
[ -f "zork1.zip" ] ||
	wget --no-check-certificate --user-agent='User-Agent: Mozilla/5.0' \
	--referer=https://www.infocom-if.org/downloads/downloads.html \
	https://www.infocom-if.org/downloads/zork1.zip
tar -xf qemu-xmas-uefi-zork.tar.xz
cat qemu-xmas-uefi-zork/README
(
cd qemu-xmas-uefi-zork
unzip -o -q ../zork1.zip
mkdir -p zork.img/EFI/BOOT
mv BOOTX64.EFI zork.img/EFI/BOOT/
mv -f startup.nsh Frotz.efi DATA/ZORK1.DAT zork.img/
execute qemu-system-x86_64 $(accel) -name "uefi zork" -bios OVMF-pure-efi.fd -usb \
	-device usb-storage,drive=zork -drive file=fat:rw:zork.img,id=zork,if=none,format=raw
)
removeDir qemu-xmas-uefi-zork
}

function qemu2014day07 {
download https://www.qemu-advent-calendar.org/2014/download/qemu-xmas-minix3.tar.xz
tar -xf qemu-xmas-minix3.tar.xz
extractReadme qemu-xmas-minix3/run.sh
execute qemu-system-x86_64 qemu-xmas-minix3/minix3.qcow2
removeDir qemu-xmas-minix3
}

function qemu2014day06 {
download https://www.qemu-advent-calendar.org/2014/download/fractal-mbr.tar.xz
tar -xf fractal-mbr.tar.xz
extractReadme fractal-mbr/run
execute qemu-system-i386 -drive file=fractal-mbr/phosphene.mbr,format=raw $(accel)
removeDir fractal-mbr
}

function qemu2014day05 {
download https://www.qemu-advent-calendar.org/2014/download/arm64.tar.xz
tar -xf arm64.tar.xz
extractReadme arm64/run
cat arm64/README
export MSYS2_ARG_CONV_EXCL='*'
execute qemu-system-aarch64 -m 1024 -cpu cortex-a57 -machine virt -monitor none -kernel arm64/Image \
	-append 'root=/dev/vda2 rw rootwait mem=1024M console=ttyAMA0,38400n8' \
	-drive if=none,id=image,file=arm64/armv8.qcow2 -netdev user,id=user0,hostfwd=tcp::5555-:22 \
	-device virtio-net-device,netdev=user0 -device virtio-blk-device,drive=image
unset MSYS2_ARG_CONV_EXCL
removeDir arm64
}

function qemu2014day04 {
download https://www.qemu-advent-calendar.org/2014/download/stxmas.tar.xz
tar -xf stxmas.tar.xz
extractReadme stxmas/run
execute qemu-system-i386 -drive file=stxmas/stxmas.img,format=raw $(audio ES1370)
removeDir stxmas
}

function qemu2014day03 {
download https://www.qemu-advent-calendar.org/2014/download/pi.tar.xz
tar -xf pi.tar.xz
extractReadme pi/run
execute qemu-system-i386 -drive file=pi/pi.vfd,format=raw
removeDir pi
}

function qemu2014day02 {
download https://www.qemu-advent-calendar.org/2014/download/freedos.tar.xz
tar -xf freedos.tar.xz
extractReadme freedos/run
execute qemu-system-i386 freedos/freedos.qcow2
removeDir freedos
}

function qemu2014day01 {
download https://www.qemu-advent-calendar.org/2014/download/qemu-xmas-slackware.tar.xz
tar -xf qemu-xmas-slackware.tar.xz
xzcat qemu-xmas-slackware/slackware.qcow2.xz > qemu-xmas-slackware/slackware.qcow2
extractReadme qemu-xmas-slackware/run
cat qemu-xmas-slackware/README
execute qemu-system-x86_64 $(accel) -m 16M \
	-drive if=ide,format=qcow2,file=qemu-xmas-slackware/slackware.qcow2 \
	-netdev user,id=slirp -device ne2k_isa,netdev=slirp
removeDir qemu-xmas-slackware
}

export PIDFILE=".qemupid.$(date +%s)"
determineAccel
case $BLOCK in
	2020)
		DIR="$DOWNLOADDIR/qemu-advent-calendar/2020"
		require wget
		perform qemu2020day01
		# SKIP qemu2020day02 - contains qemu-5.2.0-rc4.tar.xz
		perform qemu2020day03
		perform qemu2020day04
		perform qemu2020day05
		perform qemu2020day06
		perform qemu2020day07
		perform qemu2020day08
		perform qemu2020day09
		# SKIP qemu2020day10 - contains qemu-5.2-0.tar.xz
		isQemuSystem lm32 && perform qemu2020day11
		perform qemu2020day12
		perform qemu2020day13
		perform qemu2020day14
		perform qemu2020day15
		perform qemu2020day16
		perform qemu2020day17
		perform qemu2020day18
		perform qemu2020day19
		perform qemu2020day20
		perform qemu2020day21
		perform qemu2020day22
		perform qemu2020day23
		perform qemu2020day24
		;;
	2018)
		DIR="$DOWNLOADDIR/qemu-advent-calendar/2018"
		require wget
		perform qemu2018day01
		perform qemu2018day02
		perform qemu2018day03
		perform qemu2018day04
		perform qemu2018day05
		perform qemu2018day06
		perform qemu2018day07
		perform qemu2018day08
		perform qemu2018day09
		perform qemu2018day10
		perform qemu2018day11
		# SKIP qemu2018day12 - contains qemu-3.1.0.tar.xz
		perform qemu2018day13
		perform qemu2018day14
		perform qemu2018day15
		perform qemu2018day16
		perform qemu2018day17
		perform qemu2018day18
		perform qemu2018day19
		perform qemu2018day20
		perform qemu2018day21
		perform qemu2018day22
		perform qemu2018day23
		# qemu2018day24 fails with with current versions due to overlapping memory regions
		# see https://patchew.org/QEMU/cover.1560904640.git.alistair.francis@wdc.com/
		perform qemu2018day24
		;;
	2016)
		DIR="$DOWNLOADDIR/qemu-advent-calendar/2016"
		require wget unzip p7zip
		perform qemu2016day01
		ignoreSize && perform qemu2016day02
		perform qemu2016day03
		ignoreSize && perform qemu2016day04
		perform qemu2016day05
		perform qemu2016day06
		perform qemu2016day07
		# qemu2016day08 requires shared memory device and mke2fs (available on Linux hosts only)
		ignoreSize && isLinux && perform qemu2016day08
		perform qemu2016day09
		perform qemu2016day10
		perform qemu2016day11
		perform qemu2016day12
		# qemu2016day13 demos nested kvm (available on Linux hosts only)
		# TODO isLinux && perform qemu2016day13
		perform qemu2016day14
		perform qemu2016day15
		perform qemu2016day16
		ignoreSize && perform qemu2016day17
		perform qemu2016day18
		perform qemu2016day19
		ignoreSize && perform qemu2016day20
		perform qemu2016day21
		ignoreSize && perform qemu2016day22
		perform qemu2016day23
		perform qemu2016day24
		;;
	2014)
		DIR="$DOWNLOADDIR/qemu-advent-calendar/2014"
		require wget unzip
		perform qemu2014day01
		perform qemu2014day02
		perform qemu2014day03
		perform qemu2014day04
		ignoreSize && perform qemu2014day05
		perform qemu2014day06
		ignoreSize && perform qemu2014day07
		perform qemu2014day08
		ignoreSize && perform qemu2014day09
		perform qemu2014day10
		perform qemu2014day11
		perform qemu2014day12
		perform qemu2014day13
		perform qemu2014day14
		ignoreSize && perform qemu2014day15
		perform qemu2014day16
		ignoreSize && perform qemu2014day17
		ignoreSize && perform qemu2014day18
		perform qemu2014day19
		perform qemu2014day20
		ignoreSize && perform qemu2014day21
		perform qemu2014day22
		perform qemu2014day23
		perform qemu2014day24
		;;
	HD)
		DIR="$DOWNLOADDIR/qemu-desktop"
		require wget ${MINGW_PACKAGE_PREFIX}-spice-gtk ${MINGW_PACKAGE_PREFIX}-gtk-vnc
		perform qemuInstalledDesktopSDL
		perform qemuInstalledDesktopGTK
		perform qemuInstalledDesktopVNC1
		perform qemuInstalledDesktopVNC2
		perform qemuInstalledDesktopSPICE1
		perform qemuInstalledDesktopSPICE2
		;;
	*)
		BLOCK=DVD
		DIR="$DOWNLOADDIR/qemu-desktop"
		require wget ${MINGW_PACKAGE_PREFIX}-spice-gtk ${MINGW_PACKAGE_PREFIX}-gtk-vnc
		perform qemuLiveDesktopSPICE
		perform qemuLiveDesktopSDL
		perform qemuLiveDesktopGTK
		perform qemuLiveDesktopVNC
		perform qemuLiveDesktopUEFI_Bios
		perform qemuLiveDesktopUEFI_Pflash
		perform qemuLiveDesktopUEFI_Bios_Noaccel
		perform qemuLiveDesktopUEFI_Pflash_Noaccel
		perform qemuLiveDesktopQemuImgOperations
		;;
esac

