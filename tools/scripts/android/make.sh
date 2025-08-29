# Try to craft the optimal make command and export it as $make
set -e

source "host.sh"

make_command="make"

make="$make_command -j $logical_cores"

# If Make version is at least 4, sync job output
function make_version {
  $make_command --version | grep -Eo '[0-9]+\.[0-9]+' | head -n 1
}
function make_version_cmp {
  expr $(make_version) "$1" "$2" > /dev/null
  return $?
}
if make_version_cmp '>=' 4
then
  make="$make --output-sync"
fi

export make
