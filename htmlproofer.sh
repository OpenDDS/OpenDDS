site_dir=_site

if grep -RI --quiet 'href="/pages/objectcomputing/OpenDDS/' $site_dir
then
  # This gets inserted in the workflow, but we need to remove it for
  # htmlproofer.
  strip_url='\/pages\/objectcomputing\/OpenDDS'
else
  # Absolute URLs to the website wont work unless we are serving at the same
  # time, replace with root-relative.
  strip_url='https?\:\/\/localhost\:4000'
fi

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
exec bundle exec htmlproofer  $site_dir \
  --check-html \
  --check-img-http \
  $(: TODO: --enforce-https \ ) \
  --file-ignore "$file_ignore_arg" \
  --url-ignore "$url_ignore_arg" \
  $(: 'Removing this adds almost 500 errors. Ignore for the time being.') \
  --empty-alt-ignore \
  $(: 'Sites like github.com wont like it if we make too many requests too quickly.') \
  --hydra-config '{ "max_concurrency": 1 }' \
  --url-swap "$strip_url:" \
  "$@"
