#!/bin/bash

set -e

function on_exit {
  if [ ! -z "$(jobs -p)" ]; then
    kill $(jobs -p) >/dev/null 2>&1 || true
  fi
}
trap on_exit EXIT

#jobs &>/dev/null
#exec &>/dev/null

cd node_controller
./node_controller one-shot --name Jessie &
./node_controller one-shot --name James &
./node_controller one-shot --name Meowth &
cd ../test_controller
./test_controller ../example example --wait-for-nodes 4
