#! /bin/bash
read -p "Are you sure you want to rebuild opencv? [Y/N]" reply
[[ ! "$reply" =~ ^[Yy]$ ]] && exit 0
echo "making now...."
makepkg-mingw -sCLf --skipchecksums

zsts=($(find . -name "*.zst"))
if [ "${#zsts[@]}" -ne "0" ]; then
	cmd="pacman -U $zsts"
	echo $cmd
	# $cmd
fi
