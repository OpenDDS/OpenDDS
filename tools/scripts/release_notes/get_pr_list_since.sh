#!/bin/bash
git log | sed "/${1}/q" | grep "Merge pull request #" | cut -d '#' -f 2 | cut -d ' ' -f 1 | grep "[0-9]\{4\}"
