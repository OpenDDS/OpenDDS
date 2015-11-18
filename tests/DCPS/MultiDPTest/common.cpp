// -*- C++ -*-
//

#include "common.h"

#include "dds/DCPS/StaticIncludes.h"

const char* type_name = "Foo";
int num_datawriters = 2;
int num_instances_per_writer = 1;
int num_samples_per_instance = 1;
const char* topic_name[2]  = { "foo1", "foo2" };
ACE_Atomic_Op<ACE_Thread_Mutex, int> num_reads = 0;

ACE_TString synch_file_dir;
// These files need to be unlinked in the run test script before and
// after running.
ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
ACE_TString pub_finished_filename = ACE_TEXT("publisher_finished.txt");
ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
ACE_TString sub_finished_filename = ACE_TEXT("subscriber_finished.txt");
