source setenv.sh

failed=false
for var in android_cc android_cxx android_ld android_cpp_stdlib
do
  file=${!var}
  if [ ! -x "$file" ]
  then
    if ! $failed
    then
      echo $api ----------------------------------------------------------------------
      failed=true
    fi
    echo -e "Missing $var=\e[31m$file\e[0m"
  fi
done

if $failed
then
  exit 1
fi
