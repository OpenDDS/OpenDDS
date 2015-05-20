#/bin/bash

objdump -C -l -d -S $@ | awk -f $DDS_ROOT/tools/scripts/find_operator_new.awk | sort | uniq -c | grep -v -e MemoryPool -e SafetyProfilePool
