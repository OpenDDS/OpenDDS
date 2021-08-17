#!/bin/bash

set -e

if [ ! -f mkcsv.py ]
then
  echo "ERROR: This script needs to be ran from tools/scripts/release_notes" 1>&2
  exit 1
fi

function usage {
  echo "Usage: get_pr_info.sh [-t ISO8601] [-c COMMITISH]"
  echo "See README.md for details"
}

while getopts t:c:h opt
do
  case "$opt" in
    t)
      since="$OPTARG"
      ;;
    c)
      commitish="$OPTARG"
      ;;
    h)
      usage
      exit 0
      ;;
    \?)
      usage 1>&2
      exit 1
      ;;
  esac
done

shift $(( OPTIND - 1 ))
if [ "$#" -ne 0 ]
then
  echo Too many arguments 1>&2
  usage 1>&2
  exit 1
fi

if [ -n "$since" ]
then
  true
elif [ -n "$commitish" ]
then
  echo "Using commit time of commitish argument $commitish..."
  since="$(git show -s --pretty='format:%cI' "$commitish" | tail -n 1)"
elif [ -f newest_prs.json ]
then
  echo "newest_prs.json found, using last PR in there..."
  since="$(jq --raw-output '.[-1].mergedAt' newest_prs.json)"
else
  echo "newest_prs.json not found, using last major release time.."

  if [ ! -f releases.json ]
  then
    echo "releases.json not found, getting releases from GitHub..."
    gh api repos/objectcomputing/OpenDDS/releases > releases.json
  fi

  # Get last major release
  since="$(jq --raw-output 'map(select(.prerelease == false)) | map(select(.name | test("OpenDDS \\d+\\.\\d+(\\.0)?$"))) | .[0].published_at' releases.json)"
fi

[ -z "$since" ] && exit 1

echo "Getting PRs merged since \"$since\"..."

gh pr list --limit 1000 --repo objectcomputing/OpenDDS \
  --base master --state merged --search "merged:>$since" \
  --json mergedAt,number,title,url,author | \
jq 'map(with_entries(select(.key != "author")) + (.author | with_entries(.value = .value))) | sort_by(.mergedAt)' \
  > .newest_prs.json

count=$(jq 'length' .newest_prs.json)
echo "Got $count new PRs"
if [ "$count" -eq 0 ]
then
  echo "newest_prs.json not updated"
  rm .newest_prs.json
  exit 0
fi

echo "Updating newest_prs.json..."
mv .newest_prs.json newest_prs.json

echo "Creating newest_prs.csv..."
python3 mkcsv.py newest_prs.json > newest_prs.csv

echo "Done"
