#!/bin/bash

echo
echo "Qemu examples/tests prepared to be executed with qemu/win32."
echo "Executed commandlines will be printed to screen."
echo
echo "On execution each test needs to download, most test only a few 10 MB or less,"
echo "but several up to some 100MB."
read -p "Only accept reasonable downloads? ([y]|n) " TEST
[ "n" != "$TEST" ] || IGNORESIZE=1
echo
echo "Name block of qemu examples to execute."
echo "Choose year of qemu-advent-calender (2014, 2016, 2018, 2020) or qemu-desktop (DVD)"
read -p "Your choice? (2014|2016|2018|2020|[DVD]) " BLOCK
echo
read -p "Execute as regression test? (y|[n]) " REGRESSION
if [ "y" == "$REGRESSION" ]
then
	echo "To execute as regression test, provide absolute paths of two"
        echo "$MINGW_PACKAGE_PREFIX-qemu-Archives to compare, leave empty otherwise."
	read -p "First  $MINGW_PACKAGE_PREFIX-qemu archive? " FIRSTQEMU
	read -p "Second $MINGW_PACKAGE_PREFIX-qemu archive? " SECONDQEMU
fi
echo

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

function execute {
	echo "--------------------------------------------------------------------------------"
	[ -n "$MSYS2_ARG_CONV_EXCL" ] && echo "export MSYS2_ARG_CONV_EXCL=\"$MSYS2_ARG_CONV_EXCL\""
	echo ${@} | fold -s -
	[ -n "$MSYS2_ARG_CONV_EXCL" ] && echo "unset MSYS2_ARG_CONV_EXCL"
	echo "--------------------------------------------------------------------------------"
	if [ -n "$KILLMSG" ]
	then
		echo
		echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
		echo "To continue, kill Qemu process (eg. using Windows Task Manager)"
		echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
		echo
		sleep 5
	fi
	"${@}"
}

function killvm {
	local ORIGIN=$1
	# $ ps aux
	#      PID    PPID    PGID     WINPID   TTY         UID    STIME COMMAND
	#     2053    1918    1918       2032  pty0      197609 18:53:23 /usr/bin/bash
	#     2060    2053    1918       5904  pty0      197609 18:53:23 /mingw64/lib/qemu/qemu-system-x86_64
	# VMPID is either PID of wrapper which called qemu-system-* or qemu-system-*
	# In both cases it allows to filter for the correct qemu-system-*
	# Filter for qemu-system-* and given VMPID and extract QEMUPID from line:
	local QEMUPID=$(ps aux | grep -v grep | grep qemu-system | grep "\b$VMPID\b" | sed "s/^\s*//" | sed "s/\s.*//")
	if [ -n "$VMPID" ] && [ -n "$QEMUPID" ]
	then
		echo "Killing identified $ORIGIN Qemu process $QEMUPID"
		kill $QEMUPID
	elif ps aux | grep -v grep | grep qemu-system
	then
		echo
		echo "Couldn't identify $ORIGIN Qemu process, but found unknown Qemu processes."
		echo "Stopping $0"
		echo
		exit 1
	fi
	sleep 2
}

function executeVnc {
	# Previous cmd was 'execute qemu-system-*'
	local VMPID=$!
	sleep 2
	echo
	echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
	echo "Execute external vnc client using port 5905 - Kill VM / Continue pressing RETURN"
	echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
	echo
	read TEST
	killvm VNC
}

function executeSpicy {
	# Previous cmd was 'execute qemu-system-*'
	local VMPID=$!
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
	spicy -h localhost -p 5905 2> /dev/null
	sleep 1
	killvm SPICE
}

function installPackage {
	local PACKAGE=$1
	if [ "$MINGW_PREFIX" == "" ]
	then
		echo "$PACKAGE is not available in $MSYSTEM"
		exit 1
	fi
	if ! pacman -Q -i ${MINGW_PACKAGE_PREFIX}-$PACKAGE &> /dev/null
	then
		read -p "$PACKAGE is missing. Install? (y|[n]) " TEST
		if [ "$TEST" != "y" ]
		then
			echo "$PACKAGE is mandatory for $0"
			exit 1
		fi
		pacman -S ${MINGW_PACKAGE_PREFIX}-$PACKAGE
	fi
}

function perform {
	local FUN=$1
	if [ -n "$DIR" ]
	then
		[ -d "$DIR" ] || mkdir -p $DIR
		cd $DIR
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
	local EOF
	local LINE
	cat $FILE | while read LINE
	do
		echo "$LINE" | grep EOF &> /dev/null && EOF=1 || EOF=0
		if [ "$TXT" == "1" ]
		then
			[ "$EOF" == "1" ] && return || echo "$LINE"
		else
			[ "$EOF" == "1" ] && TXT=1
		fi
	done
}

function accel {
	# WHPX with qemu 6.0.0 produces error message "injection failed", if APIC is set
	# some textmode operations (eg CD menu) are not navigatable
	# -M kernel-irqchip=off mitigates these problems
	# 
	# Output while APIC is set:
	# WHPX: setting APIC emulation mode in the hypervisor
	# whpx: injection failed, MSI (0, 0) delivery: 0, dest_mode: 0, trigger mode: 0, vector: 0, lost (c0350005)
	local WHPX
	qemu-system-x86_64.exe -version 2>&1 | grep 6.0.0 > /dev/null && WHPX="whpx,kernel-irqchip=off" || WHPX="whpx"
	echo "-accel $WHPX -accel hax -accel tcg"
}

# UEFI-Desktop (LiveImage: Very Slow) - fails with whpx => drop acceleration:
# qemu-system-x86_64.exe: WHPX: Failed to emulate MMIO access with EmulatorReturnStatus: 2
# qemu-system-x86_64.exe: WHPX: Failed to exec a virtual processor
function qemuLiveDesktopUEFI {
download $LIVE_IMAGE_URL
rm -f testtemp.qcow2
qemu-img create -f qcow2 testtemp.qcow2 20G
execute qemu-system-x86_64 -m 2G \
	-drive file=$MINGW_PREFIX/lib/qemu/edk2-x86_64-code.fd,if=pflash,format=raw \
	-device intel-hda -device hda-duplex \
	-cdrom $LIVE_IMAGE_FILE -drive file=testtemp.qcow2,media=disk
}

# SDL-Desktop (LiveImage)
function qemuLiveDesktopSDL {
download $LIVE_IMAGE_URL
rm -f testtemp.qcow2
qemu-img create -f qcow2 testtemp.qcow2 20G
execute qemu-system-x86_64 $(accel) -display sdl -m 2G \
	-device intel-hda -device hda-duplex \
	-cdrom $LIVE_IMAGE_FILE -drive file=testtemp.qcow2,media=disk
}

# GTK-Desktop (LiveImage)
function qemuLiveDesktopGTK {
download $LIVE_IMAGE_URL
rm -f testtemp.qcow2
qemu-img create -f qcow2 testtemp.qcow2 20G
execute qemu-system-x86_64 $(accel) -display gtk -m 2G \
	-device intel-hda -device hda-duplex \
	-cdrom $LIVE_IMAGE_FILE -drive file=testtemp.qcow2,media=disk
}

# VNC-Desktop (LiveImage)
function qemuLiveDesktopVNC {
download $LIVE_IMAGE_URL
rm -f testtemp.qcow2
qemu-img create -f qcow2 testtemp.qcow2 20G
execute qemu-system-x86_64 $(accel) -m 2G -display vnc=:05 -k de \
	-device intel-hda -device hda-duplex \
	-cdrom $LIVE_IMAGE_FILE -drive file=testtemp.qcow2,media=disk &
executeVnc
}

# Spice-Desktop (LiveImage)
function qemuLiveDesktopSPICE {
download $LIVE_IMAGE_URL
rm -f testtemp.qcow2
qemu-img create -f qcow2 testtemp.qcow2 20G
execute qemu-system-x86_64 $(accel) -m 2G -vga qxl -spice port=5905,addr=127.0.0.1,disable-ticketing=on \
	-device intel-hda -device hda-duplex \
	-device virtio-serial -chardev spicevmc,id=spicechannel0,name=vdagent \
	-device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 \
	-cdrom $LIVE_IMAGE_FILE -drive file=testtemp.qcow2,media=disk &
executeSpicy
}

# Extended SDL-Desktop (HDImage)
function qemuInstalledDesktopSDL {
local IMAGE='d:\Qemu\test\test-usernet.qcow2'
[ -f "$IMAGE" ] || return 0
execute qemu-system-x86_64 $(accel) -m 1G -display sdl \
	-device intel-hda -device hda-duplex \
	-drive file="$IMAGE",index=0,media=disk,format=qcow2,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0
}

# Extended GTK-Desktop (HDImage)
function qemuInstalledDesktopGTK {
local IMAGE='/d/Qemu/test/test-usernet.qcow2'
[ -f "$IMAGE" ] || return 0
execute qemu-system-x86_64 $(accel) -m 1G -display gtk \
	-device intel-hda -device hda-duplex \
	-drive file="$IMAGE",index=0,media=disk,format=qcow2,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0
}

# Extended VNC-Desktop (HDImage)
function qemuInstalledDesktopVNC1 {
local IMAGE='\Qemu\test\test-usernet.qcow2'
[ -f "$IMAGE" ] || return 0
execute qemu-system-x86_64 $(accel) -m 1G -display vnc=:05 -k de \
	-device intel-hda -device hda-duplex \
	-drive file="$IMAGE",index=0,media=disk,format=qcow2,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0 &
executeVnc
}

# Extended VNC-Desktop (HDImage) with tap-Network and host configuation of tapDevices
function qemuInstalledDesktopVNC2 {
local IMAGE='d:\Qemu\test\test.qcow2'
[ -f "$IMAGE" ] || return 0
execute qemu-system-x86_64 $(accel) -m 1G -display vnc=:05 -k de \
	-device intel-hda -device hda-duplex \
	-netdev tap,ifname=qemuTap05,id=tap0 -device virtio-net,netdev=tap0,mac=00:00:00:00:00:05 \
	-drive file="$IMAGE",index=0,media=disk,format=qcow2,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0 &
executeVnc
}

# Extended Spice-Desktop (HDImage)
function qemuInstalledDesktopSPICE1 {
local IMAGE='/d/Qemu/test/test-usernet.qcow2'
[ -f "$IMAGE" ] || return 0
execute qemu-system-x86_64 $(accel) -m 1G -vga qxl -spice port=5905,addr=127.0.0.1,disable-ticketing=on \
	-device intel-hda -device hda-duplex \
	-device virtio-serial -chardev spicevmc,id=spicechannel0,name=vdagent \
	-device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 \
	-drive file="$IMAGE",index=0,media=disk,format=qcow2,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0 &
executeSpicy
}

# Extended Spice-Desktop (HDImage) with tap-Network and host configuation of tapDevices
function qemuInstalledDesktopSPICE2 {
local IMAGE='/d/Qemu/test/test.qcow2'
[ -f "$IMAGE" ] || return 0
execute qemu-system-x86_64 $(accel) -m 1G -vga qxl -spice port=5905,addr=127.0.0.1,disable-ticketing=on \
	-device intel-hda -device hda-duplex \
	-device virtio-serial -chardev spicevmc,id=spicechannel0,name=vdagent \
	-device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 \
	-netdev tap,ifname=qemuTap05,id=tap0 -device virtio-net,netdev=tap0,mac=00:00:00:00:00:05 \
	-drive file="$IMAGE",index=0,media=disk,format=qcow2,if=none,id=drive0,discard=unmap,detect-zeroes=unmap \
	-device virtio-scsi,id=scsi0 -device scsi-hd,bus=scsi0.0,drive=drive0 &
executeSpicy
}

function qemu2020day04 {
download https://www.qemu-advent-calendar.org/2020/download/day04.tar.gz
tar -xf day04.tar.gz
cat bootRogue/README
execute qemu-system-x86_64 -hda bootRogue/rogue.img
}

function qemu2020day05 {
download https://www.qemu-advent-calendar.org/2020/download/day05.tar.gz
tar -xf day05.tar.gz
cat lights/README
execute qemu-system-x86_64 -hda lights/lights.img
}

function qemu2020day06 {
download https://www.qemu-advent-calendar.org/2020/download/day06.tar.gz
tar -xf day06.tar.gz
cat day06/adv-cal.txt
execute qemu-system-x86_64 -net none -drive file=day06/bootmine.img,format=raw,if=floppy
}

function qemu2020day07 {
download https://www.qemu-advent-calendar.org/2020/download/day07.tar.gz
tar -xf day07.tar.gz
cat day07/README
execute qemu-system-i386 -drive file=day07/visopsys-0.9-usb.img,format=raw -nic model=ne2k_pci
}

function qemu2020day08 {
download https://www.qemu-advent-calendar.org/2020/download/day08.tar.gz
tar -xf day08.tar.gz
cat day08/README
execute qemu-system-x86_64 -drive file=day08/fountain.bin,format=raw
}

function qemu2020day12 {
download https://www.qemu-advent-calendar.org/2020/download/day12.tar.gz
tar -xf day12.tar.gz
cat gameoflife/README.md
execute qemu-system-x86_64 -drive format=raw,file=gameoflife/gameoflife.bin
}

function qemu2020day13 {
download https://www.qemu-advent-calendar.org/2020/download/day13.tar.xz
tar -xf day13.tar.xz
cat Invaders_QEMUAdvent2020/README
execute qemu-system-x86_64 -m 10M -drive file=Invaders_QEMUAdvent2020/invaders.img,format=raw,if=floppy
}

function qemu2020day14 {
download https://www.qemu-advent-calendar.org/2020/download/day14.tar.xz
tar -xf day14.tar.xz
cat day14/README
execute qemu-system-x86_64 -drive file=day14/eggos.img,if=virtio \
	-net user,hostfwd=::19220-:22 -net nic,model=e1000
}

function qemu2020day15 {
download https://www.qemu-advent-calendar.org/2020/download/day15.tar.gz
tar -xf day15.tar.gz
cat day15/README
execute qemu-system-x86_64 -m 256M -machine q35 -serial mon:stdio -vga none \
	-drive if=pflash,format=raw,file=day15/snow.bin -boot a
}

function qemu2020day16 {
download https://www.qemu-advent-calendar.org/2020/download/day16.tar.gz
tar -xf day16.tar.gz
cat day16/README
download https://eldondev.com/openwrt-privoxy-qcow.img
execute qemu-system-x86_64 $(accel) -drive file=openwrt-privoxy-qcow.img,id=d0,if=none \
	-snapshot -device ide-hd,drive=d0,bus=ide.0 \
	-netdev user,id=hn0 -device e1000,netdev=hn0,id=nic1 \
	-netdev user,id=hn1,hostfwd=tcp::18118-:8118 -device e1000,netdev=hn1,id=nic2
}

function qemu2020day17 {
download https://www.qemu-advent-calendar.org/2020/download/day17.tar.gz
tar -xf day17.tar.gz
cat creek/adv-cal.txt
execute qemu-system-ppc -monitor none -parallel none -M mpc8544ds -kernel creek/creek.bin
}

function qemu2020day18 {
download https://www.qemu-advent-calendar.org/2020/download/day18.tar.gz
tar -xf day18.tar.gz
cat doom/README
execute qemu-system-x86_64 -drive file=doom/doom.img,format=raw
}

function qemu2020day19 {
download https://www.qemu-advent-calendar.org/2020/download/day19.tar.gz
tar -xf day19.tar.gz
cat aflatoxin/adv-cal.txt
execute qemu-system-i386 -net none -audiodev dsound,id=dsound -machine pcspk-audiodev=dsound \
	-drive file=aflatoxin/AFLAtoxin.bin,format=raw,if=floppy
}

function qemu2020day20 {
download https://www.qemu-advent-calendar.org/2020/download/day20.tar.gz
tar -xf day20.tar.gz
cat kpara8/adv-cal.txt
execute qemu-system-i386 -net none -drive file=kpara8/kpara8.bin,format=raw,if=floppy
}

function qemu2020day21 {
download https://www.qemu-advent-calendar.org/2020/download/day21.tar.gz
tar -xf day21.tar.gz
cat day21/README
execute qemu-system-x86_64 -kernel day21/vmlinuz -initrd day21/initramfs.linux_amd64.cpio \
	-append ip=dhcp -nic user,hostfwd=tcp:127.0.0.1:10222-:22
}

function qemu2020day22 {
download https://www.qemu-advent-calendar.org/2020/download/day22.tar.xz
tar -xf day22.tar.xz
cat day22/README
execute qemu-system-x86_64 $(accel) -m 1G -drive if=virtio,file=day22/ventoy.qcow2
}

function qemu2020day23 {
download https://www.qemu-advent-calendar.org/2020/download/day23.tar.gz
tar -xf day23.tar.gz
cat day23/README
execute qemu-system-x86_64 -L day23/bios -nodefaults -name ELKS -machine isapc -cpu 486,tsc \
	-m 1M -vga std -rtc base=utc \
	-netdev user,id=mynet,hostfwd=tcp:127.0.0.1:8080-10.0.2.15:80,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23 \
	-device ne2k_isa,irq=12,netdev=mynet -drive if=ide,format=raw,file=day23/hd32mbr-fat.bin \
	-drive if=ide,format=qcow2,file=day23/scratch.qcow2
}

function qemu2020day24 {
download https://www.qemu-advent-calendar.org/2020/download/hippo.tar.gz
tar -xf hippo.tar.gz
cat hippo/adv-cal.txt
execute qemu-system-ppc64 -monitor none -parallel none -M virtex-ml507 -m 512 \
	-dtb hippo/virtex440-ml507.dtb -kernel hippo/hippo.linux
}

function qemu2018day01 {
download https://www.qemu-advent-calendar.org/2018/download/day01.tar.xz
tar -xf day01.tar.xz
cat day01/adv-cal.txt
execute qemu-system-i386 -net none -audiodev dsound,id=dsound -machine pcspk-audiodev=dsound \
	-drive file=day01/fbird.img,format=raw,if=floppy
}

function qemu2018day02 {
download https://www.qemu-advent-calendar.org/2018/download/day02.tar.xz
tar -xf day02.tar.xz
cat day02/adv-cal.txt
execute qemu-system-xtensa -net none -monitor none -parallel none -M lx60 -cpu dc233c \
	-kernel day02/santas-sleigh-ride.elf
}

function qemu2018day03 {
download https://www.qemu-advent-calendar.org/2018/download/day03.tar.xz
tar -xf day03.tar.xz
cat day03/readme.txt
execute qemu-system-x86_64 -m 1G $(accel) -netdev user,id=net0,bootfile=http://boot.netboot.xyz \
	-device virtio-net-pci,netdev=net0 -boot n
}

function qemu2018day04 {
download https://www.qemu-advent-calendar.org/2018/download/day04.tar.xz
tar -xf day04.tar.xz
cat day04/readme.txt
execute qemu-system-ppc64 --net none --boot order=d,strict=on -g 800x600x8 --cdrom day04/snake.iso
}

function qemu2018day05 {
download https://www.qemu-advent-calendar.org/2018/download/day05.tar.xz
tar -xf day05.tar.xz
cat day05/readme.txt
execute qemu-system-i386 -drive file=day05/pc-mos.img,format=raw,if=floppy \
	-audiodev dsound,id=dsound -machine pcspk-audiodev=dsound -rtc base="1994-12-05T09:00:00"
}

function qemu2018day06 {
download https://www.qemu-advent-calendar.org/2018/download/day06.tar.xz
tar -xf day06.tar.xz
cat day06/adv-cal.txt
execute qemu-system-arm -net none -parallel none -M versatilepb -kernel day06/120_below.zImage \
	-dtb day06/versatile-pb.dtb
}

function qemu2018day07 {
download https://www.qemu-advent-calendar.org/2018/download/day07.tar.xz
tar -xf day07.tar.xz
cat day07/adv-cal.txt
execute qemu-system-m68k -monitor none -parallel none -M mcf5208evb -kernel day07/sanity-clause.elf
}

function qemu2018day08 {
download https://www.qemu-advent-calendar.org/2018/download/day08.tar.xz
tar -xf day08.tar.xz
cat day08/readme.txt
execute qemu-system-i386 -m 32 -M isapc $(accel) -cpu pentium -no-acpi \
	-audiodev dsound,id=dsound -machine pcspk-audiodev=dsound \
	-net nic,model=ne2k_isa -net user -drive if=ide,file=day08/hd.qcow2
}

function qemu2018day09 {
download https://www.qemu-advent-calendar.org/2018/download/day09.tar.xz
tar -xf day09.tar.xz
cat day09/adv-cal.txt
execute qemu-system-sh4 -monitor none -parallel none -net none -M r2d \
	-kernel day09/zImage -append loglevel=3 -serial null -serial stdio
}

function qemu2018day10 {
download https://www.qemu-advent-calendar.org/2018/download/day10.tar.xz
tar -xf day10.tar.xz
cat day10/readme.txt
execute qemu-system-i386 -net none -M q35 $(accel) -cdrom day10/gamebro.iso
}

function qemu2018day11 {
download https://www.qemu-advent-calendar.org/2018/download/day11.tar.xz
tar -xf day11.tar.xz
cat day11/adv-cal.txt
execute qemu-system-sparc -monitor none -parallel none -net none -M SS-20 -m 256 -kernel day11/zImage.elf
}

function qemu2018day13 {
download https://www.qemu-advent-calendar.org/2018/download/day13.tar.xz
tar -xf day13.tar.xz
cat day13/adv-cal.txt
execute qemu-system-mips -net none -parallel none -M malta -kernel day13/vmlinux \
	-device usb-kbd -device usb-mouse -vga cirrus -device ES1370
}

function qemu2018day14 {
download https://www.qemu-advent-calendar.org/2018/download/day14.tar.xz
tar -xf day14.tar.xz
cat day14/adv-cal.txt
execute qemu-system-nios2 -monitor none -parallel none -net none -kernel day14/vmlinux.elf
}

function qemu2018day15 {
download https://www.qemu-advent-calendar.org/2018/download/day15.tar.xz
tar -xf day15.tar.xz
cat day15/adv-cal.txt
execute qemu-system-ppc -net none -parallel none -monitor none -M g3beige -kernel day15/invaders.elf
}

function qemu2018day16 {
download https://www.qemu-advent-calendar.org/2018/download/day16.tar.xz
tar -xf day16.tar.xz
cat day16/adv-cal.txt
execute qemu-system-aarch64 -net none -parallel none -monitor none -M vexpress-a9 \
	-kernel day16/winter.zImage -dtb day16/vexpress-v2p-ca9.dtb
}

function qemu2018day17 {
download https://www.qemu-advent-calendar.org/2018/download/day17.tar.xz
tar -xf day17.tar.xz
cat day17/adv-cal.txt
execute qemu-system-microblaze -monitor none -parallel none -kernel day17/ballerina.bin
}

function qemu2018day18 {
download https://www.qemu-advent-calendar.org/2018/download/day18.tar.xz
tar -xf day18.tar.xz
cat day18/adv-cal.txt
local KILLMSG=1
execute qemu-system-arm -M canon-a1100 -net none -display none -serial stdio -bios day18/barebox.canon-a1100.bin
}

function qemu2018day19 {
download https://www.qemu-advent-calendar.org/2018/download/day19.tar.xz
tar -xf day19.tar.xz
cat day19/adv-cal.txt
execute qemu-system-ppc64 -device VGA -monitor none -M ppce500 -cpu e5500 -net none \
	-device pci-ohci -device usb-kbd -kernel day19/uImage
}

function qemu2018day20 {
download https://www.qemu-advent-calendar.org/2018/download/day20.tar.xz
tar -xf day20.tar.xz
cat day20/adv-cal.txt
execute qemu-system-or1k -net none -monitor none -parallel none -kernel day20/vmlinux
}

function qemu2018day21 {
download https://www.qemu-advent-calendar.org/2018/download/day21.tar.xz
tar -xf day21.tar.xz
cat day21/README.txt
local KILLMSG=1
execute qemu-system-aarch64 -kernel day21/bootstrap.elf -nographic -cpu cortex-a57 -m 1024 \
	-net none -M virt,virtualization=true
}

function qemu2018day22 {
download https://www.qemu-advent-calendar.org/2018/download/day22.tar.xz
tar -xf day22.tar.xz
cat day22/adv-cal.txt
export MSYS2_ARG_CONV_EXCL='*'
execute qemu-system-mips64 -net none -parallel none -M malta -device ES1370 \
        -device usb-kbd -device usb-mouse -device cirrus-vga,vgamem_mb=16 \
        -hda day22/ri-li.qcow2 -kernel day22/vmlinux -append root=/dev/hda
unset MSYS2_ARG_CONV_EXCL
}

function qemu2018day23 {
download https://www.qemu-advent-calendar.org/2018/download/day23.tar.xz
tar -xf day23.tar.xz
cat day23/adv-cal.txt
execute qemu-system-sparc64 -net none -parallel none -kernel day23/vmlinux
}

function qemu2016day01 {
download https://www.qemu-advent-calendar.org/2016/download/day01.tar.xz
tar -xf day01.tar.xz
cat mikeos/readme.txt
execute qemu-system-i386 -drive file=mikeos/mikeos.flp,format=raw,if=floppy \
	-audiodev dsound,id=dsound -machine pcspk-audiodev=dsound
}

function qemu2016day03 {
download https://www.qemu-advent-calendar.org/2016/download/day03.tar.xz
tar -xf day03.tar.xz
cat freegem/readme.txt
execute qemu-system-i386 $(accel) -m 32 -hda freegem/freegem.qcow2 \
	-audiodev dsound,id=dsound -machine pcspk-audiodev=dsound
}

function qemu2016day04 {
download https://www.qemu-advent-calendar.org/2016/download/day04.tar.xz
tar -xf day04.tar.xz
cat reactos/readme.txt
download https://sourceforge.net/projects/reactos/files/ReactOS/0.4.3/ReactOS-0.4.3-live.zip/download \
	ReactOS-0.4.3-live.zip
unzip ReactOS-0.4.3-live.zip
execute qemu-system-i386 $(accel) -m 512 -net nic,model=rtl8139 -net user -vga std -device AC97 \
	-usb -device usb-tablet -cdrom ReactOS-0.4.3-live.iso
}

function qemu2016day05 {
download https://www.qemu-advent-calendar.org/2016/download/day05.tar.xz
tar -xf day05.tar.xz
cat hanoi/readme.txt
execute qemu-system-ppc64 --boot order=d,strict=on -g 800x600x8 --cdrom hanoi/hanoi.iso
}

function qemu2016day07 {
download https://www.qemu-advent-calendar.org/2016/download/day07.tar.xz
tar -xf day07.tar.xz
cat sorry-ass/readme.txt
execute qemu-system-i386 -drive file=sorry-ass/sorryass.bin,format=raw,if=floppy \
	-audiodev dsound,id=dsound -machine pcspk-audiodev=dsound
}

function qemu2016day09 {
download https://www.qemu-advent-calendar.org/2016/download/day09-v2.tar.xz
tar -xf day09-v2.tar.xz
cat kolibrios/readme.txt
execute qemu-system-i386 $(accel) -device e1000 \
        -net user -usb -device intel-hda -device hda-duplex \
        -boot d -cdrom kolibrios/kolibri-v2.iso
}

function qemu2016day10 {
download https://www.qemu-advent-calendar.org/2016/download/day10.tar.xz
tar -xf day10.tar.xz
cat epic-pinball/readme.txt
execute qemu-system-x86_64 epic-pinball/freedos2016.qcow2 -device sb16 -vga std,retrace=precise -display sdl
}

function qemu2016day11 {
download https://www.qemu-advent-calendar.org/2016/download/day11.tar.xz
tar -xf day11.tar.xz
cat genode/README
execute qemu-system-i386 -serial stdio -cdrom genode/Genode_on_seL4.iso -m 2G -vga cirrus $(accel)
}

function qemu2016day13 {
download https://www.qemu-advent-calendar.org/2016/download/day12.tar.xz
tar -xf day12.tar.xz
cat tetros/readme.txt
execute qemu-system-i386 $(accel) -m 32 -drive "if=ide,file=tetros/tetros.img,format=raw"
}

function qemu2016day14 {
download https://www.qemu-advent-calendar.org/2016/download/day14.tar.xz
tar -xf day14.tar.xz
cat acorn/readme.txt
local KILLMSG=1
execute qemu-system-x86_64 $(accel) -net nic,model=virtio -net user,hostfwd=tcp::8080-:80 \
	-smp 4 -nographic -m 128 -drive file=acorn/acorn.img,format=raw,if=ide -k en-us
}

function qemu2016day15 {
download https://www.qemu-advent-calendar.org/2016/download/day15.tar.xz
tar -xf day15.tar.xz
cat ofpong/readme.txt
execute qemu-system-ppc64 -M pseries-2.1 -boot order=d,strict=on -cdrom ofpong/ofpong.iso
}

function qemu2016day16 {
download https://www.qemu-advent-calendar.org/2016/download/day16.tar.xz
tar -xf day16.tar.xz
cat tccboot/readme.txt
execute qemu-system-x86_64 $(accel) -m 512 -net nic,model=rtl8139 -net user -vga std \
	-device AC97 -cdrom tccboot/tccboot.iso
}

function qemu2016day17 {
download https://www.qemu-advent-calendar.org/2016/download/day17.tar.xz
tar -xf day17.tar.xz
cat minoca-os/readme.txt
execute qemu-system-x86_64 minoca-os/minoca-os.qcow2
}

function qemu2016day18 {
download https://www.qemu-advent-calendar.org/2016/download/day18.tar.xz
tar -xf day18.tar.xz
cat redox/readme.txt
download https://github.com/redox-os/redox/releases/download/0.0.5/harddrive.bin.gz
gunzip harddrive.bin.gz
execute qemu-system-i386 -M q35 $(accel) -vga std -device AC97 -smp 4 -m 1024 \
     -net nic,model=e1000 -net user -drive file="harddrive.bin",format=raw
}

function qemu2016day19 {
download https://www.qemu-advent-calendar.org/2016/download/day19.tar.xz
tar -xf day19.tar.xz
cat bootchess/readme.txt
execute qemu-system-i386 -drive file="bootchess"/BootChess.bin,format=raw,if=floppy
}

function qemu2016day20 {
# -hda haiku-r1alpha4.vmdk: Could not open 'haiku-r1alpha4.vmdk': 
#  Der Prozess kann nicht auf die Datei zugreifen, da sie von einem anderen Prozess verwendet wird.
download https://www.qemu-advent-calendar.org/2016/download/day20.tar.xz
tar -xf day20.tar.xz
download http://prdownloads.sourceforge.net/open-beos/haiku-r1alpha4.1-vmdk.tar.xz?download \
	haiku-r1alpha4.1-vmdk.tar.xz
tar -xf haiku-r1alpha4.1-vmdk.tar.xz
cat haiku/readme.txt
execute qemu-system-i386 $(accel) -soundhw hda -m 512 -hda "haiku-r1alpha4.vmdk" \
	-hdb "blank-bfs-2048mb.vmdk"
}

function qemu2016day21 {
download https://www.qemu-advent-calendar.org/2016/download/day21.tar.xz
tar -xf day21.tar.xz
cat wireguard/readme.txt
execute qemu-system-i386 -nodefaults -machine q35 $(accel) -smp 2 -m 96M -monitor none \
        -kernel wireguard/wireguard-test-4f257956-d81f-43f3-8fd8-1475360f58b8.kernel -append "console=hvc0"
}
		
function qemu2016day22 {
download https://www.qemu-advent-calendar.org/2016/download/day22.tar.xz
tar -xf day22.tar.xz
cat trianglix/readme.txt
execute qemu-system-x86_64 $(accel) -m 1024 -vga std -drive file=trianglix/trianglix.qcow2,format=qcow2
}

function qemu2016day23 {
download https://www.qemu-advent-calendar.org/2016/download/day23.tar.xz
tar -xf day23.tar.xz
cat zx-spectrum/readme.txt
execute qemu-system-x86_64 $(accel) -device ES1370 -drive if=ide,file=zx-spectrum/zxspectrum.qcow2
}

function qemu2016day24 {
download https://www.qemu-advent-calendar.org/2016/download/day24.tar.xz
tar -xf day24.tar.xz
cat day24/readme.txt
(
	cd day24 ; 
	execute qemu-system-i386 $(accel) -device AC97 -device VGA,addr=07.0 \
		-kernel kernel -initrd null,null,null,music.ogg
)
}

function qemu2014day24 {
download https://www.qemu-advent-calendar.org/2014/download/day24.tar.xz
tar -xf day24.tar.xz
extractReadme day24/run
(
	cd day24 ; 
	execute qemu-system-i386 $(accel) -kernel kernel \
		-initrd null,null,null,music.ogg,win.ogg,loss.ogg -device AC97 -vga std 
)
}

function qemu2014day23 {
download https://www.qemu-advent-calendar.org/2014/download/pebble-qemu-preview.tar.xz
tar -xf pebble-qemu-preview.tar.xz
extractReadme pebble-qemu-preview/run
cat pebble-qemu-preview/README
execute qemu-system-x86_64 $(accel) -rtc base=localtime -vga std -m 256 -usb \
	-hda pebble-qemu-preview/pebble_qemu_preview.vdi
}

function qemu2014day22 {
download https://www.qemu-advent-calendar.org/2014/download/s390-moon-buggy.tar.xz
tar -xf s390-moon-buggy.tar.xz
extractReadme s390-moon-buggy/run
local KILLMSG=1
execute qemu-system-s390x -nographic -kernel s390-moon-buggy/s390-bb.kernel \
	-initrd s390-moon-buggy/s390-moon-buggy.initrd
}

function qemu2014day20 {
download https://www.qemu-advent-calendar.org/2014/download/helenos.tar.xz
tar -xf helenos.tar.xz
extractReadme helenos/run
execute qemu-system-x86_64 $(accel) -net nic,model=e1000 \
	-net user,hostfwd=::2223-:2223,hostfwd=::8080-:8080 \
	-usb -device intel-hda -device hda-duplex -boot d -cdrom helenos/HelenOS-0.6.0-rc3-amd64.iso
}

function qemu2014day19 {
download https://www.qemu-advent-calendar.org/2014/download/mandelbrot.tar.xz
tar -xf mandelbrot.tar.xz
extractReadme mandelbrot/run
execute qemu-system-ppc64 -M mac99 -drive file=mandelbrot/mandelbrot.raw,format=raw
}

function qemu2014day18 {
download https://www.qemu-advent-calendar.org/2014/download/ceph.tar.xz
tar -xf ceph.tar.xz
extractReadme ceph/run
execute qemu-system-x86_64 $(accel) -m 1024M -drive file=ceph/ceph.qcow2,format=qcow2 \
	-netdev user,id=net0,hostfwd=tcp::10022-:22 -device virtio-net-pci,netdev=net0
}

function qemu2014day17 {
download https://www.qemu-advent-calendar.org/2014/download/bb_debian.tar.xz
tar -xf bb_debian.tar.xz
extractReadme bb_debian/run
execute qemu-system-i386 $(accel) -m 512 -vga std -device intel-hda -device hda-duplex bb_debian/bb_debian.qcow2
}

function qemu2014day16 {
download https://www.qemu-advent-calendar.org/2014/download/tempest-showroom.tar.xz
tar -xf tempest-showroom.tar.xz
extractReadme tempest-showroom/run
execute qemu-system-i386 $(accel) -cdrom tempest-showroom/tempest-showroom_v0.9.7.iso
}

function qemu2014day15 {
download https://www.qemu-advent-calendar.org/2014/download/plan9.tar.xz
tar -xf plan9.tar.xz
extractReadme plan9/run
execute qemu-system-i386 $(accel) -m 1024 plan9/plan9.qcow2
}

function qemu2014day14 {
download https://www.qemu-advent-calendar.org/2014/download/invaders.tar.xz
tar -xf invaders.tar.xz
extractReadme invaders/run
execute qemu-system-x86_64 $(accel) -kernel invaders/invaders.exec
}

function qemu2014day13 {
download https://www.qemu-advent-calendar.org/2014/download/2nd-reality.tar.xz
tar -xf 2nd-reality.tar.xz
extractReadme 2nd-reality/run
execute qemu-system-i386 -vga std,retrace=precise -device gus 2nd-reality/2nd-reality.qcow2
}

function qemu2014day12 {
download https://www.qemu-advent-calendar.org/2014/download/oberon.tar.xz
tar -xf oberon.tar.xz
extractReadme oberon/run
execute qemu-system-i386 $(accel) oberon/oberon.qcow2
}

function qemu2014day11 {
download https://www.qemu-advent-calendar.org/2014/download/osv-redis.tar.xz
tar -xf osv-redis.tar.xz
extractReadme osv-redis/run
execute qemu-system-x86_64 $(accel) -m 256 \
                        -netdev user,id=user0,hostfwd=tcp::18000-:8000,hostfwd=tcp::16379-:6379 \
                        -device virtio-net-pci,netdev=user0 \
                        osv-redis/osv-redis-memonly-v0.16.qemu.qcow2 &
sleep 1
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
echo "Open Browser with http://localhost:18000/ on 'Server started' message"
echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
wait
}

function qemu2014day10 {
download https://www.qemu-advent-calendar.org/2014/download/512.tar.xz
tar -xf 512.tar.xz
extractReadme 512/run
execute qemu-system-x86_64 -cpu Nehalem $(accel) -vga std -audiodev dsound,id=dsound \
	-machine pcspk-audiodev=dsound -drive file=512/512.img,if=floppy,format=raw
}

function qemu2014day09 {
download https://www.qemu-advent-calendar.org/2014/download/ubuntu-core-alpha.tar.xz
tar -xf ubuntu-core-alpha.tar.xz
extractReadme ubuntu-core-alpha/run
execute qemu-system-x86_64 $(accel) -m 1024 \
                        -drive if=virtio,file=ubuntu-core-alpha/ubuntu-core-alpha-01.img,format=qcow2 \
                        -netdev user,id=user0,hostfwd=tcp::18000-:80,hostfwd=tcp::12222-:22 \
                        -device virtio-net-pci,netdev=user0 
}

function qemu2014day07 {
download https://www.qemu-advent-calendar.org/2014/download/qemu-xmas-minix3.tar.xz
tar -xf qemu-xmas-minix3.tar.xz
extractReadme qemu-xmas-minix3/run.sh
execute qemu-system-x86_64 qemu-xmas-minix3/minix3.qcow2
}

function qemu2014day06 {
download https://www.qemu-advent-calendar.org/2014/download/fractal-mbr.tar.xz
tar -xf fractal-mbr.tar.xz
extractReadme fractal-mbr/run
execute qemu-system-i386 -drive file=fractal-mbr/phosphene.mbr,format=raw $(accel)
}

function qemu2014day05 {
download https://www.qemu-advent-calendar.org/2014/download/arm64.tar.xz
tar -xf arm64.tar.xz
extractReadme arm64/run
local KILLMSG=1
cat arm64/README
(
	cd arm64 ; 
	export MSYS2_ARG_CONV_EXCL='*'
	execute qemu-system-aarch64 -m 1024 -cpu cortex-a57 -machine virt -nographic -kernel Image \
		-append 'root=/dev/vda2 rw rootwait mem=1024M console=ttyAMA0,38400n8' \
		-drive if=none,id=image,file=armv8.qcow2 -netdev user,id=user0,hostfwd=tcp::5555-:22 \
		-device virtio-net-device,netdev=user0 -device virtio-blk-device,drive=image
	unset MSYS2_ARG_CONV_EXCL
)
}

function qemu2014day04 {
download https://www.qemu-advent-calendar.org/2014/download/stxmas.tar.xz
tar -xf stxmas.tar.xz
extractReadme stxmas/run
execute qemu-system-i386 -drive file=stxmas/stxmas.img,format=raw -device ES1370
}

function qemu2014day03 {
download https://www.qemu-advent-calendar.org/2014/download/pi.tar.xz
tar -xf pi.tar.xz
extractReadme pi/run
execute qemu-system-i386 -drive file=pi/pi.vfd,format=raw
}

function qemu2014day02 {
download https://www.qemu-advent-calendar.org/2014/download/freedos.tar.xz
tar -xf freedos.tar.xz
extractReadme freedos/run
execute qemu-system-i386 freedos/freedos.qcow2
}

function qemu2014day01 {
download https://www.qemu-advent-calendar.org/2014/download/qemu-xmas-slackware.tar.xz
tar -xf qemu-xmas-slackware.tar.xz
xzcat qemu-xmas-slackware/slackware.qcow2.xz > qemu-xmas-slackware/slackware.qcow2
extractReadme qemu-xmas-slackware/run
cat qemu-xmas-slackware/README
execute qemu-system-x86_64 $(accel) -m 16M -drive if=ide,format=qcow2,file="qemu-xmas-slackware/slackware.qcow2" \
	-netdev user,id=slirp -device ne2k_isa,netdev=slirp -serial stdio "$@"
}

BASE="$(pwd)"
installPackage qemu
case $BLOCK in
	2020)
		DIR=$BASE/qemu-advent-calendar/2020
		perform qemu2020day04
		perform qemu2020day05
		perform qemu2020day06
		perform qemu2020day07
		perform qemu2020day08
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
		DIR=$BASE/qemu-advent-calendar/2018
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
		;;
	2016)
		DIR=$BASE/qemu-advent-calendar/2016
		perform qemu2016day01
		perform qemu2016day03
		[ -n "$IGNORESIZE" ] && perform qemu2016day04
		perform qemu2016day05
		perform qemu2016day07
		perform qemu2016day09
		perform qemu2016day10
		perform qemu2016day11
		[ -n "$IGNORESIZE" ] && perform qemu2016day13
		perform qemu2016day14
		perform qemu2016day15
		perform qemu2016day16
		[ -n "$IGNORESIZE" ] && perform qemu2016day17
		perform qemu2016day18
		perform qemu2016day19
		[ -n "$IGNORESIZE" ] && perform qemu2016day20
		perform qemu2016day21
		[ -n "$IGNORESIZE" ] && perform qemu2016day22
		perform qemu2016day23
		perform qemu2016day24
		;;
	2014)
		DIR=$BASE/qemu-advent-calendar/2014
		perform qemu2014day01
		perform qemu2014day02
		perform qemu2014day03
		perform qemu2014day04
		[ -n "$IGNORESIZE" ] && perform qemu2014day05
		perform qemu2014day06
		[ -n "$IGNORESIZE" ] && perform qemu2014day07
		[ -n "$IGNORESIZE" ] && perform qemu2014day09
		perform qemu2014day10
		perform qemu2014day11
		perform qemu2014day12
		perform qemu2014day13
		perform qemu2014day14
		[ -n "$IGNORESIZE" ] && perform qemu2014day15
		perform qemu2014day16
		[ -n "$IGNORESIZE" ] && perform qemu2014day17
		[ -n "$IGNORESIZE" ] && perform qemu2014day18
		perform qemu2014day19
		perform qemu2014day20
		perform qemu2014day22
		perform qemu2014day23
		perform qemu2014day24
		;;
	HD)
		DIR=$BASE/qemu-desktop
		installPackage spice
		installPackage spice-gtk
		perform qemuInstalledDesktopSDL
		perform qemuInstalledDesktopGTK
		perform qemuInstalledDesktopVNC1
		perform qemuInstalledDesktopVNC2
		perform qemuInstalledDesktopSPICE1
		perform qemuInstalledDesktopSPICE2
		;;
	*)
		BLOCK=DVD
		DIR=$BASE/qemu-desktop
		installPackage spice
		installPackage spice-gtk
		perform qemuLiveDesktopSPICE
		perform qemuLiveDesktopSDL
		perform qemuLiveDesktopGTK
		perform qemuLiveDesktopVNC
		perform qemuLiveDesktopUEFI
		;;
esac

