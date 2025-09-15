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
    echo "Extracting zip file"
    unzip ${archive} -d ${dir}
  elif [[ ${archive} == *.tar.* ]]; then
    echo "Extracting tar archive"
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
    --dir ${downloads_dir}/${package} \
    --clobber
  extract ${downloads_dir}/${package}/${pattern} ${installed_dir}
}

function install_from_package_manager() {
  echo "Installing package \"$1\" from default provider..."
  sudo apt-get install -y $1
}

function install-ccache() {
  echo "Installing ccache from GitHub..."
  install_from_github $1 ccache/ccache '*-linux-x86_64.tar.xz'
}

function install-ninja-build() {
  echo "Installing ninja from GitHub..."
  install_from_github $1 ninja-build/ninja ninja-linux.zip
}

function install-clang() {
  echo "Installing clang..."
  # install_from_github $1 llvm/llvm-project LLVM-*-Linux-X64.tar.xz
  package=$1
  repo=llvm/llvm-project
  pattern="LLVM-*-Linux-X64.tar.xz"
  llvm_version=$(echo $package | tr ' ' '\n' | sed -n 's/^clang.*-\([1-9][0-9]*\)$/\1/p' | sort | uniq)
  echo "Downloading LLVM release ${llvm_version}"
  time gh release download llvmorg-${llvm_version}\
    --repo ${repo} \
    --pattern "${pattern}" \
    --dir ${downloads_dir}/${package} \
    --clobber
  time tar xvf ${downloads_dir}/${package}/${pattern} \
    --directory ${installed_dir} \
    --wildcards */bin/llvm-profdata \
    --occurrence=1
  return
  if sudo apt-get install $1; then
    sudo apt-get install -y llvm
    return
  fi

  # If clang not already installed, add the appropriate llvm repository
  if [ -n "${llvm_version}" ]; then
    echo "Clang requested, adding LLVM repositories..."
    wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
    ubuntu_codename=`sudo cat /etc/os-release | sed -n s/UBUNTU_CODENAME=//p`
    repo="deb http://apt.llvm.org/${ubuntu_codename}/ llvm-toolchain-${ubuntu_codename}-${llvm_version} main"
    echo Adding repository "${repo}"
    sudo add-apt-repository "${repo}"
    sudo apt-get update
    sudo apt-get install -y llvm
  fi
}

function install_package() {
  if declare -F install-$1 >/dev/null; then
    install-$1 $1
  elif [[ $1 == clang* ]]; then
    install-clang $1
  else
    install_from_package_manager $1
  fi
}

echo "Installing packages"
for i in ${packages}; do
  start=`date +%s`
  tempfile=${tempdir}/$i.out.log
  > ${tempfile}

  install_package $i >${tempfile} 2>&1

  end=`date +%s`
  duration=$(( end - start ))
  printf "::group::%-${max_len}s | %ds\n" $i ${duration}
  cat ${tempfile}
  echo "::endgroup::"
done

exec_dirs=$(find ${installed_dir} \
  -type f -executable\
  -exec dirname '{}' \; \
  | sort | uniq)

echo "Installed packages dir: $(realpath ${downloads_dir})"
tree ${installed_dir}
echo
echo "Directories with executable files (added to PATH):"
printf '%s\n' "${exec_dirs[@]}"

for i in ${exec_dirs}; do
  echo "$i" >> "$GITHUB_PATH"
done
