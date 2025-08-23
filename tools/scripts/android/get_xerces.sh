#!/usr/bin/env bash

set -o pipefail
set -o errexit
set -o nounset

echo get_xerces.sh ============================================================

source setenv.sh

intre='^[0-9]+$'
if [[ ! $api =~ $intre ]]
then
  echo 'for get_xerces.sh $api must be defined' 1>&2
  exit 1
fi

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

# Get GNU libiconv
if $need_iconv
then
  basename="libiconv-1.16"
  tarname="$basename.tar.gz"
  url="https://ftp.gnu.org/pub/gnu/libiconv/$tarname"
  ourname="iconv_source"
  get
fi

if [ ! -d xerces_source ]
then
  git clone --recursive --depth 1 \
    ${XERCES_REPO:-https://github.com/OpenDDS/xerces-c} \
    --branch ${XERCES_BRANCH:-xerces-3.2-android} xerces_source
fi
