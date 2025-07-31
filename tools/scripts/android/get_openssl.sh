#!/usr/bin/env bash

set -o pipefail
set -o errexit
set -o nounset

echo get_openssl.sh ===========================================================

source setenv.sh

function get {
  if [ ! -d "$ourname" ]
  then
    if [ ! \( -f $tarname -o -L $tarname \) ]
    then
      if [ -f ../$tarname -o -L ../$tarname ]
      then
        ln -s "../$tarname" "$tarname"
      elif [ -f ../../$tarname -o -L ../../$tarname ]
      then
        ln -s "../../$tarname" "$tarname"
      else
        download_file "$url"
      fi
    fi

    if ! ${JUST_CACHE_SOURCES:-false}
    then
      tar -xzf "$tarname"
      mv "$basename" "$ourname"
      rm -f "$tarname"
    fi
  fi
}

version="3.0.2"
basename="openssl-$version"
tarname="$basename.tar.gz"
url="https://www.openssl.org/source/$tarname"
ourname="openssl_source"
get
