#!/bin/bash

site_dir="${site_dir:-_site}"
check_path="$site_dir"

while getopts "p:" opt; do
  case "$opt" in
    p)
      check_path="$OPTARG"
      ;;
  esac
done
shift $((OPTIND - 1))

# Absolute URLs to the website with a domain won't work unless we are serving
# from that address at the same time. Use --url-swap to replace the URLs with
# just an absolute path.
url_swap_list=() # Format is REGEX:REPLACE_STRING
if grep -RI --quiet 'href="/pages/OpenDDS/OpenDDS/' "$site_dir"
then
  url_swap_list+=(
    '^https?\:\/\/github\.com\/pages\/OpenDDS\/OpenDDS:'
    '^\/pages\/OpenDDS\/OpenDDS:'
  )
else
  url_swap_list+=(
    '^https?\:\/\/localhost\:4000:'
  )
fi
url_swap_arg="$(printf ",%s" "${url_swap_list[@]}")"
url_swap_arg="${url_swap_arg:1}"

# Format is path string or regex within //
# Paths start at the directory containing the website, $site_dir
file_ignore_list=(
  "$site_dir/documents/ExcelRTD/userguide.html"
  "$site_dir/documents/Monitor/userguide.html"
  "$site_dir/documents/Bench/userguide.html"
  "/^$site_dir\/perf\//"
)
file_ignore_arg="$(printf ",%s" "${file_ignore_list[@]}")"
file_ignore_arg="${file_ignore_arg:1}"

url_ignore_list=(
  '/zoom\.us\//'
  '/brighttalk\.com\//'
  # New pages will have invalid "Edit this page on GitHub" links until they are
  # merged into the main repo's gh-pages.
  '/github\.com\/OpenDDS\/OpenDDS\/blob\/gh-pages\//'
  '/allaboutcookies\.org\//'
  '/dds-foundation\.org/'
  '/axcioma\.org\//'
)
url_ignore_arg="$(printf ",%s" "${url_ignore_list[@]}")"
url_ignore_arg="${url_ignore_arg:1}"

# NOTE: $(:) is a nop command that can be used like an inline comment.
exec bundle exec htmlproofer "$check_path" \
  --root-dir "$site_dir" \
  $(: TODO: --enforce-https \ ) \
  --swap-urls "$url_swap_arg" \
  --ignore-files "$file_ignore_arg" \
  --ignore-urls "$url_ignore_arg" \
  $(: 'Removing this adds almost 500 errors. Ignore for the time being.') \
  --ignore-missing-alt \
  $(: 'Sites like github.com wont like it if we make too many requests too quickly.') \
  --hydra '{ "max_concurrency": 1 }' \
  "$@"
