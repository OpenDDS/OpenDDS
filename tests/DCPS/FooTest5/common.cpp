// -*- C++ -*-
//
// $Id$

#include "common.h"


const char* MY_TOPIC    = "foo";
const char* MY_TOPIC_FOR_UDP = "fooudp";
const char* MY_TOPIC_FOR_MULTICAST = "foomulticast";
const char* MY_TYPE     = "Foo";
const char* MY_TYPE_FOR_UDP = "FooUdp";
const char* MY_TYPE_FOR_MULTICAST = "FooMulticast";
const ACE_TCHAR* reader_address_str = ACE_TEXT("localhost:0");
const ACE_TCHAR* writer_address_str = ACE_TEXT("localhost:0");
int reader_address_given = 0;
int writer_address_given = 0;

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

int use_take = 0;
int num_samples_per_instance = 1;
int num_instances_per_writer = 1;
int num_datareaders = 1;
int num_datawriters = 1;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 1;
// default to using TCP
int using_udp = 0;
int using_multicast = 0;
int sequence_length = 10;
int no_key = 0;
InstanceDataMap results;
ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> num_reads = 0;
long op_interval_ms = 0;
long blocking_ms = 0;
int mixed_trans = 0;
int test_bit = 0;

ACE_TString synch_file_dir;

// These files need to be unlinked in the run test script before and
// after running.
ACE_TString pub_ready_filename = ACE_TEXT("publisher_ready.txt");
ACE_TString pub_finished_filename = ACE_TEXT("publisher_finished.txt");
ACE_TString sub_ready_filename = ACE_TEXT("subscriber_ready.txt");
ACE_TString sub_finished_filename = ACE_TEXT("subscriber_finished.txt");
