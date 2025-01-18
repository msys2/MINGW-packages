case ":$PATH" in
	*:"${MINGW_PREFIX}/lib/rustup/bin":*)
		;;
	*)
		PATH="${PATH:+$PATH:}${MINGW_PREFIX}/lib/rustup/bin"
esac
export PATH
