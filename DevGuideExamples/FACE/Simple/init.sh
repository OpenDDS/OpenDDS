#!/bin/bash

# ./init.sh FaceMessage.idl idl_build/FaceMessageTypeSupport.idl idl_build/

# needed to avoid error on library generation
source /opt/OpenDDS/setenv.sh

idl_path=$1
typeSupport_path=$2
output=$3

cd "$(dirname "$idl_path")" || exit

opendds_idl --idl-version 4 -GfaceTS -Lface "$idl_path" -o "$output"
tao_idl --idl-version 4 -I"$DDS_ROOT" -I"$TAO_ROOT"/orbsvcs "$idl_path" -o "$output"
tao_idl --idl-version 4 -I"$DDS_ROOT" -I"$TAO_ROOT"/orbsvcs -I"$(dirname $(realpath $idl_path))" "$typeSupport_path" -o "$output"
