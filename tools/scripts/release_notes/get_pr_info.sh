#!/bin/bash
BLOCK=$(git log --date=iso | grep -B 5 -A 3 "Merge pull request #${1}")
#echo "$BLOCK"
USER=$(echo "$BLOCK" | grep "Merge pull request #${1}" | cut -d '#' -f 2 | cut -d ' ' -f 3 | cut -d '/' -f 1)
COMMIT=$(echo "$BLOCK" | grep commit | cut -d ' ' -f 2)
DATE=$(echo "$BLOCK" | grep Date | sed 's/ [ ]*/ /g' | cut -d ' ' -f 2-3)
DESC=$(echo "${BLOCK}" | tail -n 1 | sed 's/^[ ]*//')
#echo $DESC
LINES=$(git show --oneline --first-parent $COMMIT | wc -l)
#echo "PR #${1} was merged in commit $COMMIT on $DATE with approximately $LINES line changes."
echo $DATE, ${1}, \"$DESC\", no, , , , $USER

