#!/bin/bash

if [[ $1 == '-n' ]]; then
  dry_run=1
  shift
fi

max_len=0
for i in $@; do
  len=${#i}
  max_len=$(( len > max_len ? len : max_len ))
done

if [[ $# -eq 0 ]]; then
  echo "No packages selected to install"
  exit
fi

packages=$@
tempdir=${RUNNER_TEMP:-${TMPDIR}}/install-apt

mkdir -p ${tempdir}

function install_package() {
  start=`date +%s`
  tempfile=${tempdir}/$1.out.log
  > ${tempfile}
  if [[ ${dry_run} ]]; then
    echo sudo apt-get install -y $1
    sleep 1
  else
    sudo apt-get install -y $1 >${tempfile} 2>&1
  fi
  end=`date +%s`

  duration=$(( end - start ))
  printf "::group::%-${max_len}s | %ds\n" $1 ${duration}
  cat ${tempfile}
  echo "::endgroup::"
}

# If clang packages requested, add the appropriate llvm repository
LLVM_VERSIONS=$(echo ${packages} | tr ' ' '\n' | sed -n 's/^clang.*-\([1-9][0-9]*\)$/\1/p' | sort | uniq)
if [ -n "${LLVM_VERSIONS}" ]; then
  echo "Clang requested, adding LLVM repositories..."
  wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
  UBUNTU_CODENAME=`sudo cat /etc/os-release | sed -n s/UBUNTU_CODENAME=//p`
  for VERSION in ${LLVM_VERSIONS}; do
    REPO="deb http://apt.llvm.org/${UBUNTU_CODENAME}/ llvm-toolchain-${UBUNTU_CODENAME}-${VERSION} main"
    echo Adding repository "${REPO}"
    sudo add-apt-repository "${REPO}"
  done
  sudo apt-get update
  sudo apt-get install -y llvm
fi

echo "Installing apt packages: ${packages}"
for i in ${packages}; do
  install_package $i
done
