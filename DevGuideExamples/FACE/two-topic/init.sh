#!/bin/bash

# needed to avoid error on library generation
source /opt/OpenDDS/setenv.sh

opendds_idl --idl-version 4 -GfaceTS -Lface StockQuoter.idl
tao_idl --idl-version 4 -I$DDS_ROOT -I$TAO_ROOT/orbsvcs StockQuoter.idl
tao_idl --idl-version 4 -I$DDS_ROOT -I$TAO_ROOT/orbsvcs StockQuoterTypeSupport.idl

mkdir -p build

cd build

cmake ..
make
