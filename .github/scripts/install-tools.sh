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

function panic() {
  echo "::error::$*" >&2
  exit 1
}

if [[ $# -eq 0 ]]; then
  panic "No packages selected to install"
fi

if [[ ${RUNNER_OS} != Linux && ${RUNNER_OS} != Windows ]]; then
  panic "RUNNER_OS not specified, must be one of Linux or Windows"
fi

packages=($@)

tempdir=${RUNNER_TEMP:-${TMPDIR}}/install-tools
if [[ ${RUNNER_OS} == Windows ]]; then
  tempdir=$(cygpath -u ${tempdir})
fi

downloads_dir=${tempdir}/downloads
installed_dir=$(realpath ../installed)

function ensure_empty() {
  rm -fr "$1"
  mkdir -p "$1"
}

ensure_empty ${tempdir}
ensure_empty ${downloads_dir}
ensure_empty ${installed_dir}

function extract() {
  archive=$1
  dir=$2
  if [[ ! -f "${archive}" ]]; then
    panic "File not found: \"${archive}\""
  elif [[ "${archive}" == *.zip ]]; then
    echo "Extracting zip file"
    unzip "${archive}" -d ${dir}
  elif [[ "${archive}" == *.tar.* ]]; then
    echo "Extracting tar archive"
    tar xvf "${archive}" --directory ${dir}
  else
    panic "I don't know how to extract the file ${archive}"
  fi
}

function install_from_github_noextract() {
  package=$1; shift
  repo=$1; shift
  pattern=($@)
  gh release download \
    --repo ${repo} \
    ${pattern[@]/#/--pattern } \
    --dir ${downloads_dir}/${package} \
    || panic "Failed to download release from GitHub"
  archive=(${downloads_dir}/${package}/*)
  if [[ ${#archive[@]} -gt 1 ]]; then
    panic "Multiple files downloaded! ${archive[@]}"
  fi
}

function install_from_github() {
  package=$1
  install_from_github_noextract $@
  archive=(${downloads_dir}/${package}/*)
  extract "${archive}" ${installed_dir}
}

function install_from_package_manager() {
  echo "Installing package \"$1\" from default provider..."
  sudo apt-get install -y $1 \
    || panic "Failed to install package from apt-get"
}

function install-ccache() {
  echo "Installing ccache from GitHub..."
  if [[ ${RUNNER_OS} == Linux ]]; then
    pattern='*-linux-x86_64.tar.xz'
  elif [[ ${RUNNER_OS} == Windows ]]; then
    pattern='*-windows-x86_64.zip'
  fi
  install_from_github $1 ccache/ccache ${pattern}
}

function install-OpenCppCoverage() {
  echo "Installing OpenCppCoverage from GitHub..."
  # Windows only
  install_from_github_noextract $1 OpenCppCoverage/OpenCppCoverage *-x64-*.exe || return $?
  installer=(${downloads_dir}/${package}/*)
  echo "Running installer ${installer}..."
  ${installer} //VERYSILENT //SUPPRESSMSGBOXES //NORESTART //SP- //DIR="$(cygpath -w ${installed_dir}/OpenCppCoverage)" \
    || panic "OpenCppCoverage installer returned an error"
}

function install-ninja-build() {
  echo "Installing ninja from GitHub..."
  install_from_github $1 ninja-build/ninja ninja-linux.zip
}

function install-clang() {
  echo "Installing clang..."
  return
  # install_from_github $1 llvm/llvm-project LLVM-*-Linux-X64.tar.xz
  package=$1
  repo=llvm/llvm-project
  llvm_version=$(echo $package | sed -n 's/^clang.*-\([1-9][0-9]*\)$/\1/p')
  release_tag=$(gh release list \
    --repo llvm/llvm-project \
    --json tagName \
    --exclude-drafts \
    --exclude-pre-releases \
    --jq """
      map(
        select(
          .tagName | startswith(\"llvmorg-${llvm_version}\")
          )
      ).[0].tagName
    """)

  echo "Downloading LLVM release ${llvm_version}"
  echo "Release tag: ${release_tag}"
  time gh release download ${release_tag} \
    --repo ${repo} \
    --pattern "LLVM-*-Linux-X64.tar.xz" \
    --pattern "clang+llvm-*x86_64-linux*" \
    --dir ${downloads_dir}/${package}
  archive=(${downloads_dir}/${package}/*)
  if [[ ${#archive[@]} -gt 1 ]]; then
    panic "Multiple files downloaded! ${archive[@]}"
  fi
  echo "Extracting ${archive}"
  time tar xvf ${downloads_dir}/${package}/* \
    --directory ${installed_dir} \
    --wildcards */bin/llvm-profdata \
    --occurrence=1

  return 0
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
  return $?
}

echo "Installing packages"
error_count=0
for i in ${packages[@]}; do
  start=`date +%s`
  tempfile=${tempdir}/$i.out.log
  > ${tempfile}

  # Run install_package in a subshell, to catch per-package errors
  if (install_package $i >${tempfile} 2>&1); then
    end=`date +%s`
    duration=$(( end - start ))
    status="${duration}s"
  else
    status="FAILED"
    error_count=$(( error_count + 1 ))
  fi

  printf "::group::%-${max_len}s | %s\n" $i ${status}
  cat ${tempfile}
  echo "::endgroup::"
done

echo "Packages with errors: ${error_count}"

exec_dirs=$(find ${installed_dir} \
  -type f -executable\
  -exec dirname '{}' \; \
  | sort | uniq)

echo "Installed packages dir: $(realpath ${downloads_dir})"
if [[ ${RUNNER_OS} == Linux ]]; then
  tree ${installed_dir}
elif [[ ${RUNNER_OS} == Windows ]]; then
  cmd //c tree //f //a $(cygpath -w ${installed_dir})
fi
echo

echo "Directories with executable files (added to PATH):"
for i in ${exec_dirs}; do
  if [[ ${RUNNER_OS} == Linux ]]; then
    echo "$i" | tee -a ${GITHUB_PATH}
  elif [[ ${RUNNER_OS} == Windows ]]; then
    echo $(cygpath -w "$i") | tee -a ${GITHUB_PATH}
  fi
done

exit ${error_count}
