// -*- C++ -*-
//
#ifndef COMMON_H
#define COMMON_H

#include <dds/DCPS/Atomic.h>

#include <ace/SString.h>

const long domain_id = 111;
extern const char* type_name;
extern int num_datawriters;
extern int num_instances_per_writer;
extern int num_samples_per_instance;
extern const char* topic_name[2];
extern OpenDDS::DCPS::Atomic<int> num_reads;

extern ACE_TString synch_file_dir;
// This file need to be unlinked in the run test script before and
// after running.
extern ACE_TString pub_finished_filename;

#endif /* COMMON_H */
