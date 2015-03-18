rgrep string | egrep -v '(^InfoRepo|^monitor|^idl)' | grep -v 'GNUmakefile' | grep -v '\.depend\.' | grep -v ':#include' | sed 's/\/\/.*//' | grep string
