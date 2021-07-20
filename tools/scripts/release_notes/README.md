# Release Notes Helper Scripts

## Notes

These are a couple simple bash scripts for grabbing and formatting release notes information from the git log in order to put into a spreadsheet. They were developed using:
* Ubuntu 20.04
* Bash 5.0.17
* Git 2.25.1
but can probably be used elsewhere with comparable versions of bash and git.

## Usage

There are two scripts, primarily intended to be used together:
* get_pr_list_since.sh - This script is intended to get the list of OpenDDS PRs since a particular string / commit in the log.
  * Examples:
    * ./get_pr_list_since.sh "Release 3.14"
    * ./get_pr_list_since.sh
  * Note: Since there's no way to differentiate which fork a particular PR was submitted against, this script currently filters PR #'s to 4 digits in order to avoid including PRs from forked repos. Eventually, this filter will need to be expanded / improved.
* get_pr_info.sh - This script is intended to find the commit for a particular PR, by number, and pull relevant information out into a formatted CSV line. The script has commented out various debugging echo statements which may or may not be useful to the user when trying to debug issues with the script or change the output format / include more information.
  * Examples:
    ./get_pr_info.sh 2530
* When used together, these scripts can easily (but not necessarily quickly!) give you a CSV file to use for release notes:
  * Examples:
    * ./get_pr_list_since.sh "Release 3.16" | xargs -I {} ./get_pr_info.sh {} | tee release_notes.csv
    * ./get_pr_list_since.sh 772885787256800fa23910397b96297dd7c34087 | xargs -I {} ./get_pr_info.sh {} > partial_notes.csv

# `missing-news-prs.py`

After a spreadsheet is updated, it can be downloaded as a csv file again and passed to `missing-news-prs.py`.
`missing-news-prs.py` will print out a list of partially prepared lines that are missing from the NEWS file.
It just looks for the PR numbers (`#NUMBER`), so it's fine if a PR is completly changed or combined with other PRs as long as the number is there.
