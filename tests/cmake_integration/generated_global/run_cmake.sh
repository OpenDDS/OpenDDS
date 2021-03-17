#!/bin/bash

rm -rf build
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=/home/calabresec/jfti/OpenDDS ..
cd ..
