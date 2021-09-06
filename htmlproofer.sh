file_ignore_list=(
  '_site/documents/ExcelRTD/userguide.html'
  '_site/documents/Monitor/userguide.html'
  '_site/documents/Bench/userguide.html'
)
file_ignore_arg="$(printf ",%s" "${file_ignore_list[@]}")"
file_ignore_arg="${file_ignore_arg:1}"

url_ignore_list=(
  '/zoom\.us\//'
)
url_ignore_arg="$(printf ",%s" "${url_ignore_list[@]}")"
url_ignore_arg="${url_ignore_arg:1}"

# NOTE: $(:) is a nop command that can be used like an inline comment.
exec htmlproofer _site \
  --check-html \
  --check-img-http \
  $(: TODO: --enforce-https \ ) \
  --file-ignore "$file_ignore_arg" \
  --url-ignore "$url_ignore_arg" \
  $(: 'Removing this adds almost 500 errors. Ignore for the time being.') \
  --empty-alt-ignore \
  $(: 'Sites like github.com wont like it if we make too many requests too quickly.') \
  --hydra-config '{ "max_concurrency": 1 }' \
  $(: 'Absolute URLs to the website wont work, replace with root-relative') \
  --url-swap "https?\:\/\/localhost\:4000:" \
  "$@"
