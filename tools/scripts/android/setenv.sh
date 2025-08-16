if ${OPENDDS_ANDROID_SETENV:-false}; then return; fi
export OPENDDS_ANDROID_SETENV='true'

export workspace="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd )"

download_file() {
  echo "Downloading $1 ..."
  curl --fail --remote-name --remote-header-name --location \
    --retry 3 --silent --show-error "$1"
}

# Getting Configuration
if [ -z "${ndk+x}" -o -z "${arch+x}" -o -z "${api+x}" ]
then
  if ! [ -f ${workspace}/settings.sh ]
  then
    echo 'Warning: $ndk, $arch, or $api not set and there is no settings.sh, copying default.settings.sh' 1>&2
    cp ${workspace}/default.settings.sh ${workspace}/settings.sh
  fi
  source ${workspace}/settings.sh
fi

# Alternate settings.sh that can be committed temporarily for branches.
if [ -f override_settings.sh ]
then
  source override_settings.sh
fi

# Convert arch to target and abi
if [ "$arch" = "arm" ]
then
  export target_compiler=armv7a-linux-androideabi
  export target=arm-linux-androideabi
  export abi="armeabi-v7a"
elif [ "$arch" = "arm64" ]
then
  export target=aarch64-linux-android
  export abi="arm64-v8a"
elif [ "$arch" = "x86_64" ]
then
  export target=x86_64-linux-android
  export abi="x86_64"
elif [ "$arch" = "x86" ]
then
  export target=i686-linux-android
  export abi="x86"
elif [ "$arch" = "NONE" ] # Bypass
then
  true
else
  echo "Error: \"$arch\" is invalid, must be arm, arm64, x86, or x86_64" 1>&2
  exit 1
fi
export target_cpp_stdlib=${target_cpp_stdlib:-$target}
export target_ld=${target_ld:-$target}
export target_compiler=${target_compiler:-$target}

# Android NDK
export ndk_major_rev=$(./matrix.py --get-ndk-major $ndk)
export ndk_minor_rev=$(./matrix.py --get-ndk-minor $ndk)
source "host.sh"
case $host_os in
  'linux')
    ndk_platform_name="linux-x86_64"
    if [ $ndk_major_rev -lt 23 ]
    then
      ndk_platform_dl_name="linux-x86_64"
    else
      ndk_platform_dl_name="linux"
    fi
    ;;

  'macos')
    ndk_platform_name="darwin-x86_64"
    if [ $ndk_major_rev -lt 23 ]
    then
      ndk_platform_dl_name="darwin-x86_64"
    else
      ndk_platform_dl_name="darwin"
    fi
    ;;

  *)
    echo "Error: Unknown host_os: \"$host_os\"" 1>&2
    exit 1
    ;;
esac
export OPENDDS_ANDROID_NDK="${OPENDDS_ANDROID_NDK-"$workspace/android-ndk-$ndk"}"
export android_toolchain_root="${OPENDDS_ANDROID_NDK}/toolchains/llvm/prebuilt/$ndk_platform_name"
export android_cpp_stdlib="${android_toolchain_root}/sysroot/usr/lib/${target_cpp_stdlib}/libc++_shared.so"
export android_toolchain_bin="${android_toolchain_root}/bin"
export android_ld="${android_toolchain_bin}/${target_ld}-ld"
if [ $ndk_major_rev -ge 22 ]
then
  # ${target_ld}-ld doesn't exist in r22. GNU linker is still there under
  # different names, but we should use LLVM linker.
  export android_ld="${android_toolchain_bin}/ld.lld"
fi
export android_cc="${android_toolchain_bin}/${target_compiler}${api}-clang"
export android_cxx="${android_cc}++"
export use_toolchain=${use_toolchain-"false"}
export need_toolchain=$use_toolchain
export use_security=${use_security:-false}
if $need_toolchain
then
  export toolchain_name="$ndk-$arch-android-$api-toolchain"
  export android_toolchain="${workspace}/${toolchain_name}"
  export PATH=${PATH}:"$android_toolchain/bin"
fi

# OpenDDS
dds_dir="${DDS_ROOT-"$workspace/../../.."}"
export DDS_ROOT="$(cd "$dds_dir" >/dev/null && pwd)"
unset dds_dir

# ACE/TAO
export ace_tao=${ace_tao-'doc_group_master'}
case $ace_tao in
  'doc_group_master')
    export ace_tao_default_branch='master'
    if [ $ndk_major_rev -lt 27 ]
    then
      export force_cpp_std="c++17"
    fi
    ;;

  'doc_group_ace6_tao2')
    export ace_tao_default_branch='ace6tao2'
    ;;

  *)
    echo "Error: Invalid ace_tao: $ace_tao" 1>&2
    exit 1
    ;;
esac
mpc_dir="ACE_TAO/MPC"
ace_dir="ACE_TAO/ACE"
tao_dir="ACE_TAO/TAO"
export MPC_ROOT="${MPC_ROOT-"${DDS_ROOT}/${mpc_dir}"}"
export ACE_ROOT="${ACE_ROOT-"${DDS_ROOT}/${ace_dir}"}"
export TAO_ROOT="${TAO_ROOT-"${DDS_ROOT}/${tao_dir}"}"
export host_tools=${host_tools:-}
if [ -z "$host_tools" ]
then
  ace_target="$DDS_ROOT/build/target/ACE_TAO/ACE"
  ace_host="$DDS_ROOT/build/host/ACE_TAO/ACE"
else
  export HOST_DDS="$host_tools"
  export HOST_ACE="$host_tools/ACE_TAO/ACE"
  ace_target="$ACE_ROOT"
fi
export ace_target
export PATH=${PATH}:"$ACE_ROOT/bin"

# Optional Features
export use_java=${use_java:-false}
if $use_java
then
  export android_sdk="${ANDROID_SDK-"${workspace}/android-sdk"}"
  if [ ! -z ${android_target_api+x} ]
  then
    export target_api="$android_target_api"
  elif [ ! -z ${target_api+x} ]
  then
    export android_target_api="$target_api"
  else
    echo "Error: use_java is true, but android_target_api isn't set" 1>&2
    exit 1
  fi
fi
if $use_security
then
  export GNU_ICONV_ROOT="${GNU_ICONV_ROOT-"${workspace}/secdeps_prefix"}"
  export XERCESCROOT="${XERCESCROOT-"${workspace}/secdeps_prefix"}"
  export SSL_ROOT="${SSL_ROOT-"${workspace}/secdeps_prefix"}"
fi
export build_ace_tests=${build_ace_tests:-true}
export mpc_concurrent=${mpc_concurrent:-true}

need_iconv=true
if [[ $api -ge 28 ]]
then
  need_iconv=false
fi
