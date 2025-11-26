#!/usr/bin/env bash

set -o pipefail
set -o nounset

source "known_apis.sh"

failed=false

ndks=()
if [ -z "${ndk+x}" ]
then
  ndks=(${known_ndks_ndk_only[@]})
else
  if ! [[ ${known_ndks_ndk_only[@]} =~ "${ndk}" ]]
  then
    echo "$ndk is invalid !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    exit 0
  fi
  ndks=($ndk)
fi

archs=()
if [ -z "${arch+x}" ]
then
  arches=(${known_archs[@]})
else
  arches=($arch)
fi

for ndk in ${ndks[@]}
do
  dir="android-ndk-$ndk"
  if [ -d $dir ]
  then
    for arch in ${arches[@]}
    do
      echo "$ndk-$arch ======================================================================"
      known_apis_name="known_apis_${ndk}_${arch}"
      declare -n known_apis=$known_apis_name
      this_failed=false
      for api in ${known_apis[@]}
      do
        ndk=$ndk arch=$arch api=$api bash assert_ndk_files_for.sh
        if [ $? -ne 0 ]
        then
          this_failed=true
          failed=true
        fi
      done
      if ! $this_failed
      then
        echo "OK"
      fi
    done
  else
    echo "$dir not present !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
  fi

done

if $failed
then
  exit 1
fi
