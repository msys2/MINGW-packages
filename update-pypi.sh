#!/bin/bash

git config --global user.email Who?
git config --global user.name Who?
git config --global --unset credential.helper

git fetch --all

nvchecker -c check-pypi.toml --logger json > events.json

declare -a packages=($(jq -r '. | select (.event == "updated") | .name' events.json))
declare -a package_versions=($(jq -r '. | select (.event == "updated") | .version' events.json))

git switch -c master upstream/master

i=0
for package in ${packages[@]}; do
  if [[ -d mingw-w64-${package} ]]; then
    branch_exist=$(git ls-remote --heads https://github.com/msys2/MINGW-packages.git ${package}-update | wc -l)
    if (( ! ${branch_exist} )); then
      git checkout -b ${package}-update master
    else
      git switch ${package}-update
    fi
    cd mingw-w64-${package}
    new_ver=${package_versions[${i}]}
    sed -e "s|^pkgver=.*|pkgver=${new_ver}|g" -i PKGBUILD
    sed -e "s|^pkgrel=.*|pkgrel=1|g" -i PKGBUILD
    updpkgsums
    cd ..
    sed -e "s|^  \"${package}\":[^,]*|  \"${package}\": \"${new_ver}\"|g" -i oldver-pypi.json
    if [[ `git status --porcelain -- mingw-w64-${package}/PKGBUILD` ]]; then
      git add mingw-w64-${package}/PKGBUILD
      if (( ! ${branch_exist} )); then
        git commit -m "${package}: update to ${new_ver}"
        git push -u https://${PAT_GITHUB}@github.com/msys2/MINGW-packages.git ${package}-update
      else
        git commit --amend -m "${package}: update to ${new_ver}"
        git push -f -u https://${PAT_GITHUB}@github.com/msys2/MINGW-packages.git ${package}-update
      fi
      pr_exist=$(gh pr list -H ${package}-update -R msys2/MINGW-packages | wc -l)
      if (( ${pr_exist} )); then
        gh pr create --fill -R msys2/MINGW-packages
      else
        gh pr edit ${package}-update -t "${package}: update to ${new_ver}" -R msys2/MINGW-packages
      fi
    fi
  fi
  i=$((${i}+1))
done
