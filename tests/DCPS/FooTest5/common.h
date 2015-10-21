// -*- C++ -*-
//
#ifndef COMMON_H
#define COMMON_H

#include  "InstanceDataMap.h"
#include  "ace/Time_Value.h"
#include  "ace/SString.h"
#include  "ace/Atomic_Op.h"
#include  <string>

const long  MY_DOMAIN = 411;
extern const char* MY_TOPIC;
extern const char* MY_TOPIC_FOR_UDP;
extern const char* MY_TOPIC_FOR_MULTICAST;
extern const char* MY_TYPE;
extern const char* MY_TYPE_FOR_UDP;
extern const char* MY_TYPE_FOR_MULTICAST;
extern const ACE_Time_Value max_blocking_time;

extern int num_samples_per_instance;
extern int num_instances_per_writer;
extern int num_datareaders;
extern int num_datawriters;
extern int max_samples_per_instance;
extern int history_depth;
// default to using TCP
extern int using_udp;
extern int using_multicast;
extern int using_rtps_transport;
extern int using_shmem;
extern int sequence_length;
extern int no_key;
extern InstanceDataMap results;
extern ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> num_reads;
extern long op_interval_ms;
extern long blocking_ms;
extern int mixed_trans;

extern ACE_TString synch_file_dir;
// These files need to be unlinked in the run test script before and
// after running.
extern ACE_TString pub_ready_filename;
extern ACE_TString pub_finished_filename;
extern ACE_TString sub_ready_filename;
extern ACE_TString sub_finished_filename;


#endif /* COMMON_H */
