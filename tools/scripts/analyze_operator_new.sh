#/bin/bash

for file in "$@"
do
    echo "Processing ${file}"
    objdump -C -l -d -S "${file}" | awk -f $DDS_ROOT/tools/scripts/find_operator_new.awk | grep -v -e '^$' | sort | uniq -c | grep -v -e MemoryPool -e SafetyProfilePool -e Malloc_Allocator -e Object_Manager
done
