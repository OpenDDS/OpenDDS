// -*- C++ -*-
//

#include "common.h"

#include <dds/DCPS/StaticIncludes.h>

const char* type_name = "Foo";
int num_datawriters = 2;
int num_instances_per_writer = 1;
int num_samples_per_instance = 1;
const char* topic_name[2]  = { "foo1", "foo2" };
ACE_Atomic_Op<ACE_Thread_Mutex, int> num_reads = 0;
const ACE_TCHAR* mmap_file = ACE_TEXT("tmp_file");
const char* obj_name = "state";
