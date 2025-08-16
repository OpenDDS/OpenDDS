if ${OPENDDS_ANDROID_HOST:-false}; then return; fi
export OPENDDS_ANDROID_HOST='true'

# Detect Host OS
case $OSTYPE in
  'linux-gnu'*)
    host_os='linux'
    logical_cores=$(nproc)
    ;;

  'darwin'*)
    host_os='macos'
    logical_cores=$(sysctl -n hw.logicalcpu)
    ;;

  *)
    echo "Error: Unsupported OSTYPE: \"$OSTYPE\"" 1>&2
    exit 1
    ;;
esac
