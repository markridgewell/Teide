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

packages="$@ tree"
tempdir=${RUNNER_TEMP:-${TMPDIR}}/install-apt
downloads_dir=$(realpath ../downloads)
installed_dir=$(realpath ../installed)

mkdir -p ${tempdir}
mkdir -p ${downloads_dir}
mkdir -p ${installed_dir}

function extract() {
  archive=$1
  dir=$2
  if [[ $1 == *.zip ]]; then
    echo "${archive}: zip file"
    unzip ${archive} -d ${dir}
  elif [[ ${archive} == *.tar.* ]]; then
    echo "${archive}: tar archive"
  tar xvf ${archive} --directory ${dir}
  fi
}

function install_from_github() {
  package=$1
  repo=$2
  pattern="$3"
  gh release download \
    --repo ${repo} \
    --pattern "${pattern}" \
    --dir ${downloads_dir}/$1
  extract ${downloads_dir}/$1/${pattern} ${installed_dir}
}

function install_package() {
  start=`date +%s`
  tempfile=${tempdir}/$1.out.log
  > ${tempfile}
  if [[ ${dry_run} ]]; then
    echo sudo apt-get install -y $1
    sleep 1
  else
    if [[ $1 == ccache ]]; then
      install_from_github $1 ccache/ccache '*-linux-x86_64.tar.xz' \
        >${tempfile} 2>&1
    elif [[ $1 == ninja-build ]]; then
      install_from_github $1 ninja-build/ninja ninja-linux.zip \
        >${tempfile} 2>&1
    else
      sudo apt-get install -y $1 >${tempfile} 2>&1
    fi
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

echo "Installing packages: ${packages}"
for i in ${packages}; do
  install_package $i
done

exec_dirs=$(find ${installed_dir} \
  -type f -executable\
  -exec dirname '{}' \; \
  | sort | uniq)

echo "Installed packages dir: $(realpath ${downloads_dir})"
tree ${installed_dir}
echo
echo "Directories with executable files:"
printf '%s\n' "${exec_dirs[@]}"

for i in ${exec_dirs}; do
  echo "$i" >> "$GITHUB_PATH"
done
