#!/usr/bin/env bash

set -o pipefail
set -o errexit
set -o nounset

echo get_ndk.sh ===============================================================

source setenv.sh

ndk_dir=android-ndk-$ndk
ndk_zip=$ndk_dir-$ndk_platform_dl_name.zip

if [ ! \( -d $ndk_dir -o -L $ndk_dir \) ]
then
  if [ -d ../$ndk_dir -o -L ../$ndk_dir ]
  then
    ln -s ../$ndk_dir
    exit 0
  elif [ -d ../../$ndk_dir -o -L ../../$ndk_dir ]
  then
    ln -s ../../$ndk_dir
    exit 0
  fi
  url=https://dl.google.com/android/repository/$ndk_zip
  download_file "$url"
  echo "Done, Unziping $ndk_zip..."
  unzip -qq $ndk_zip
  echo "Done"
  rm -f $ndk_zip
fi
