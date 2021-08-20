# Release Notes Helper Scripts

These are scripts for grabbing and formatting information for the release notes.

## `get_pr_info.sh`

`get_pr_info.sh` gets a list of pull requests from GitHub and turns that into a CSV file that can be edited by the team.

### Requirements

- A Unix-like environment with:
  - Bash
  - Git CLI client
  - Python 3.6+
  - The [GitHub CLI command(`gh`)](https://cli.github.com/).
    It should be authorized for a GitHub account.
  - The [`jq` JSON Processor](https://stedolan.github.io/jq/)

### Usage

`get_pr_info.sh` will always get a list of merged PRs into master on objectcomputing/OpenDDS.
The main thing to be concerned with is how far back this list should go, but the script will try to pick this automatically:

- With no extra files in directory the script will pick the time of the last major release.
  - It keeps a `releases.json` for this. Remove it to force it to download the releases again.
- If there is an existing `newest_prs.json`, it will use the time of the newest PR in the list.
  This means it will be a list of PRs merged since the last time the script was ran.

The time can also be manually specified using one of the following options:

- `-t ISO8601` where `ISO8601` is the full ISO 8601 format of the time to use, for example: `2021-07-29T03:14:28+00:00`.
- `-p PR` where `PR` is the number of a merged PR.
- `-c COMMITISH` where `COMMITISH` is something like a hash or tag of the Git commit whose commit time will be used.

If successful, it will create a `newest_prs.json` that it can use as the starting point next time and a `newest_prs.csv` that can be added to the spreadsheet.
If there are no new PRs it will say that and won't change any other existing files.

If a new `newest_prs.csv` was created, then it can be appended a Google Sheets spreadsheet by going to "File" -> "Import" -> "Upload", adding the file, then make sure to set "Import location" to "Append to current sheet".

## `missing-news-prs.py`

After a spreadsheet is updated, it can be downloaded as a csv file again and passed to `missing-news-prs.py`.
`missing-news-prs.py` will print out a list of partially prepared lines that are missing from the NEWS file.
It just looks for the PR numbers (`#NUMBER`), so it's fine if a PR is completely changed or combined with other PRs as long as the number is there.
