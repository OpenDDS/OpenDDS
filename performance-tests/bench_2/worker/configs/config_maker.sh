#!/bin/bash

intro=$(cat snippets/process_intro.snip)
instance=$(cat snippets/instance.snip)
midriff=$(cat snippets/process_midriff.snip)
participant=$(cat snippets/participant.snip)
outro=$(cat snippets/process_outro.snip)

ilist=""
plist=""

pcount=10
if [ -z ${1+"asdf"} ]; then
  pcount=10
else
  pcount=$1
fi

for (( i=1; i<=$pcount; i++ ))
do

  itemp=$(echo "$instance" | sed "s/instance_01/instance_${i}/g")

  if [[ -z $ilist ]]; then
    ilist=$itemp
  else
    ilist=$(echo -e "$ilist,\n$itemp")
  fi

  ptemp=$(echo "$participant" | sed "s/participant_01/participant_${i}/g" | sed "s/instance_01/instance_${i}/g" | sed "s/subscriber_01/subscriber_${i}/g" | sed "s/datareader_01/datareader_${i}/g" | sed "s/publisher_01/publisher_${i}/g" | sed "s/datawriter_01/datawriter_${i}/g")

  if [[ -z $plist ]]; then
    plist=$ptemp
  else
    plist=$(echo -e "$plist,\n$ptemp")
  fi

done
echo -e "$intro\n$ilist\n$midriff\n$plist\n$outro"
