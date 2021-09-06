site_dir="${site_dir:-_site}"

# Absolute URLs to the website with a domain wont work unless we are serving
# from that address at the same time. Use --url-swap to replace the URLs with
# just an absolute path.
url_swap_list=()
if grep -RI --quiet 'href="/pages/objectcomputing/OpenDDS/' "$site_dir"
then
  url_swap_list+=(
    '^https?\:\/\/github\.com\/pages\/objectcomputing\/OpenDDS:'
    '^\/pages\/objectcomputing\/OpenDDS:'
  )
else
  url_swap_list+=(
    '^https?\:\/\/localhost\:4000:'
  )
fi
url_swap_arg="$(printf ",%s" "${url_swap_list[@]}")"
url_swap_arg="${url_swap_arg:1}"

file_ignore_list=(
  "$site_dir/documents/ExcelRTD/userguide.html"
  "$site_dir/documents/Monitor/userguide.html"
  "$site_dir/documents/Bench/userguide.html"
)
file_ignore_arg="$(printf ",%s" "${file_ignore_list[@]}")"
file_ignore_arg="${file_ignore_arg:1}"

url_ignore_list=(
  '/zoom\.us\//'
)
url_ignore_arg="$(printf ",%s" "${url_ignore_list[@]}")"
url_ignore_arg="${url_ignore_arg:1}"

# NOTE: $(:) is a nop command that can be used like an inline comment.
exec bundle exec htmlproofer "$site_dir" \
  --check-html \
  --check-img-http \
  $(: TODO: --enforce-https \ ) \
  --url-swap "$url_swap_arg" \
  --file-ignore "$file_ignore_arg" \
  --url-ignore "$url_ignore_arg" \
  $(: 'Removing this adds almost 500 errors. Ignore for the time being.') \
  --empty-alt-ignore \
  $(: 'Sites like github.com wont like it if we make too many requests too quickly.') \
  --hydra-config '{ "max_concurrency": 1 }' \
  "$@"
