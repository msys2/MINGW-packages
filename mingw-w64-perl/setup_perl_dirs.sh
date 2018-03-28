#!/bin/bash
#
# Helper script to configure directories for binaries, scripts, libraries etc.
# Uses sed to make the necessary changes in the source. Comes with a couple of
# caveats: Compared to posix, the windows-version is a bit crippled in terms of
# flexibility.
#
# 1. Shared and arch-dependant libs must reside in the same directory, because
#    of how Windows loads DLL's.
# 2. It's not enough to change config.gc; scripts get their values from this but
#    binaries are compiled with hardcoded paths configured in win32/win32.c.
#    Because... [reasons].

set -o errexit
set -o nounset
set -o errtrace
set -o pipefail

progpath="$(realpath "$0")"
progdir="${progpath%/*}"
progname="${progpath##*/}"

usage() {
    echo "Usage: ${progname} [path to src tree root]"
}

if [[ -z "${1:-}" || ! -d "${1}/win32" ]]; then
    usage
    exit 1
fi

srcdir="${1}"

bin='\\bin'
scr='\\bin'
priv='\\lib\\perl5' # shared libs
arch='\\lib\\perl5' # arch-dependant libs
html='\\share\\doc\\perl5'
core='\\core_perl'
site='\\site_perl'
vend='\\vendor_perl'

# If one want, for example perl.exe in $PREFIX/bin, set bin_core to "${bin}"
bin_core="${bin}"
bin_site="${bin}${site}"
bin_vend="${bin}${vend}"
scr_core="${scr}${core}"
scr_site="${scr}${site}"
scr_vend="${scr}${vend}"
priv_core="${priv}${core}"
priv_site="${priv}${site}"
priv_vend="${priv}${vend}"
arch_core="${arch}${core}"
arch_site="${arch}${site}"
arch_vend="${arch}${vend}"
html_core="${html}${core}"
html_site="${html}${site}"
html_vend="${html}${vend}"

declare -i \
    CORE_VER=0 \
    SITE_VER=1 \
    VEND_VER=0 \
    ARCH=0

###                                            ###
### No need to change anything below this line ###
###                                            ###

path() {
    if (( $3 == 0 )); then
        A='~'
        B='~'
    elif (( $3 == 1 )); then
        A='$('
        B=')'
    fi
    if [[ "${!1}" != *perl* ]]; then
        P='_PERL'
    else
        P=''
    fi
    printf "${A}INST_TOP${B}%s%s%s" \
        "${!1}" \
        "$(if (( $2 >= 1 )); then
            if   [[ "$1" == *site ]]; then printf "${A}INST_SITE_VER${P}${B}"
            elif [[ "$1" == *vend ]]; then printf "${A}INST_VEND_VER${P}${B}"
            else printf "${A}INST_CORE_VER${P}${B}"
            fi
        fi)" \
        "$( (( $2 >= 2 )) && printf "${A}INST_ARCH${B}")"
}

sed -i -E \
-e "/^(install|site|vendor)?prefix(exp)?=/             s/=.*/='~INST_TOP~'/" \
-e "/^d_((priv|arch)lib|(site|vendor)(arch|lib|bin|script))=/ s/=.*/='define'/" \
-e "/^libpth=/                            s/=.*/=''/" \
-e "/^installstyle=/                      s/=.*/='lib\/perl5'/" \
-e "/^perlpath=/                          s/=.*/='$(path  bin_core 2 0)"'\\perl.exe'"'/" \
-e "/^(install)?script(dir)?(exp)?=/      s/=.*/='$(path  scr_core 1 0)'/" \
-e "/^(install)?bin(exp)?=/               s/=.*/='$(path  bin_core 2 0)'/" \
-e "/^(install)?sitescript(exp)?=/        s/=.*/='$(path  scr_site 1 0)'/" \
-e "/^(install)?sitebin(exp)?=/           s/=.*/='$(path  bin_site 2 0)'/" \
-e "/^(install)?vendorscript(exp)?=/      s/=.*/='$(path  scr_vend 1 0)'/" \
-e "/^(install)?vendorbin(exp)?=/         s/=.*/='$(path  bin_vend 2 0)'/" \
-e "/^(install)?privlib(exp)?=/           s/=.*/='$(path priv_core 1 0)'/" \
-e "/^(install)?archlib(exp)?=/           s/=.*/='$(path arch_core 2 0)'/" \
-e "/^(install)?sitelib_stem=/            s/=.*/='$(path priv_site 0 0)'/" \
-e "/^(install)?sitelib(exp)?=/           s/=.*/='$(path priv_site 1 0)'/" \
-e "/^(install)?sitearch(exp)?=/          s/=.*/='$(path arch_site 2 0)'/" \
-e "/^(install)?vendorlib_stem=/          s/=.*/='$(path priv_vend 0 0)'/" \
-e "/^(install)?vendorlib(exp)?=/         s/=.*/='$(path priv_vend 1 0)'/" \
-e "/^(install)?vendorarch(exp)?=/        s/=.*/='$(path arch_vend 2 0)'/" \
-e "/^(install)?htmldir(exp)?=/           s/=.*/='$(path html      1 0)'/" \
-e "/^(install)?htmlhelpdir(exp)?=/       s/=.*/='$(path html      1 0)"'\\htmlhelp'"'/" \
-e "/^(install)?html[13]dir(exp)?=/       s/=.*/='$(path html_core 1 0)'/" \
-e "/^(install)?sitehtml[13]dir(exp)?=/   s/=.*/='$(path html_site 1 0)'/" \
-e "/^(install)?vendorhtml[13]dir(exp)?=/ s/=.*/='$(path html_vend 1 0)'/" \
-e "/^(install)?(vendor)?man1dir(exp)?=/  s/=.*/='~INST_TOP~"'\\share\\man\\man1'"'/" \
-e "/^(install)?(vendor)?man3dir(exp)?=/  s/=.*/='~INST_TOP~"'\\share\\man\\man3'"'/" \
-e "/^(install)?siteman1dir(exp)?=/       s/=.*/='~INST_TOP~"'\\local\\man\\man1'"'/" \
-e "/^(install)?siteman3dir(exp)?=/       s/=.*/='~INST_TOP~"'\\local\\man\\man3'"'/" \
-e "/^man1ext=/                           s/=.*/='1perl'/" \
-e "/^man3ext=/                           s/=.*/='3perl'/" \
-e "/^usevendorprefix=/                   s/=.*/='define'/" \
-e "/^otherlibdirs=/                      s/=.*/=' '/" \
-e "/^d_perl_otherlibdirs=/               s/=.*/='undef'/" \
-e "/^inc_version_list=/                  s/=.*/=' '/" \
-e "/^inc_version_list_init=/             s/=.*/='0'/" \
-e "/^d_inc_version_list=/                s/=.*/='undef'/" \
-e "/^cf_by=/                             s/=.*/=' '/" \
-e "/^cf_email=/                          s/=.*/=' '/" \
-e "/^perladmin=/                         s/=.*/=' '/" \
"${srcdir}"/win32/config.gc

# We have no (site|vendor)man(1|3)ext, to get the same effect, change man(1|3)ext after make install

sed -i -E \
-e "/^INST_SCRIPT\s+=/                  s/=.*/= $(path  scr_core 1 1)/" \
-e "/^INST_BIN\s+=/                     s/=.*/= $(path  bin_core 2 1)/" \
-e "/^INST_LIB\s+=/                     s/=.*/= $(path priv_core 1 1)/" \
-e "/^INST_ARCHLIB\s+=/                 s/=.*/= $(path arch_core 2 1)/" \
-e "/^INST_HTML\s+=/                    s/=.*/= $(path html      1 1)/" \
-e "s/^#?(INST_VER\s)/\1/" \
-e "s/^#?(INST_CORE_VER\s)/$( ((CORE_VER)) || echo -n '#')\1/" \
-e "s/^#?(INST_SITE_VER\s)/$( ((SITE_VER)) || echo -n '#')\1/" \
-e "s/^#?(INST_VEND_VER\s)/$( ((VEND_VER)) || echo -n '#')\1/" \
-e "s/^#?(INST_CORE_VER_PERL\s)/$( ((CORE_VER)) || echo -n '#')\1/" \
-e "s/^#?(INST_SITE_VER_PERL\s)/$( ((SITE_VER)) || echo -n '#')\1/" \
-e "s/^#?(INST_VEND_VER_PERL\s)/$( ((VEND_VER)) || echo -n '#')\1/" \
-e "s/^#?(INST_ARCH\s)/$( ((ARCH)) || echo -n '#')\1/" \
"${srcdir}"/win32/{GNUmakefile,makefile.mk}

vendlib="${priv_vend//\\\\/\\\/}"
vendlib="${vendlib#\\\/}"
sitelib="${priv_site//\\\\/\\\/}"
sitelib="${sitelib#\\\/}"
privlib="${priv_core//\\\\/\\\/}"
privlib="${privlib#\\\/}"

sed -i -E \
-e "/^#\s*define\s+MSYS_PERL_VENDORLIB/ s/\".*\"/\"${vendlib}\"/" \
-e "/^#\s*define\s+MSYS_SITELIB/        s/\".*\"/\"${sitelib}\"/" \
-e "/^#\s*define\s+MSYS_PRIVLIB/        s/\".*\"/\"${privlib}\"/" \
"${srcdir}"/win32/win32.c

exit 0

# vim: set ts=4 sw=4 et ai:
