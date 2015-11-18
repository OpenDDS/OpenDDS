// -*- C++ -*-
//
#ifndef COMMON_H
#define COMMON_H

#include "ace/SString.h"
#include "ace/Atomic_Op.h"

const long domain_id = 411;
extern const char* type_name;
extern int num_datawriters;
extern int num_instances_per_writer;
extern int num_samples_per_instance;
extern const char* topic_name[2];
extern ACE_Atomic_Op<ACE_Thread_Mutex, int> num_reads;

extern ACE_TString synch_file_dir;
// These files need to be unlinked in the run test script before and
// after running.
extern ACE_TString pub_ready_filename;
extern ACE_TString pub_finished_filename;
extern ACE_TString sub_ready_filename;
extern ACE_TString sub_finished_filename;


#endif /* COMMON_H */
