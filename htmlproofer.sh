site_dir="${site_dir:-_site}"

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
)
url_ignore_arg="$(printf ",%s" "${url_ignore_list[@]}")"
url_ignore_arg="${url_ignore_arg:1}"

# NOTE: $(:) is a nop command that can be used like an inline comment.
exec bundle exec htmlproofer "$site_dir" \
  $(: TODO: --enforce-https \ ) \
  --report-invalid-tags \
  --report-eof-tags \
  --report-mismatched-tags \
  --url-swap "$url_swap_arg" \
  --file-ignore "$file_ignore_arg" \
  --url-ignore "$url_ignore_arg" \
  $(: 'Removing this adds almost 500 errors. Ignore for the time being.') \
  --empty-alt-ignore \
  $(: 'Sites like github.com wont like it if we make too many requests too quickly.') \
  --hydra-config '{ "max_concurrency": 1 }' \
  "$@"
