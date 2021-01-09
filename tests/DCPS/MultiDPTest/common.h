// -*- C++ -*-
//
#ifndef COMMON_H
#define COMMON_H

#include <ace/SString.h>
#include <ace/Atomic_Op.h>
#include <ace/Malloc_T.h>
#include <ace/MMAP_Memory_Pool.h>
#include <ace/PI_Malloc.h>

const long domain_id = 111;
extern const char* type_name;
extern int num_datawriters;
extern int num_instances_per_writer;
extern int num_samples_per_instance;
extern const char* topic_name[2];
extern ACE_Atomic_Op<ACE_Thread_Mutex, int> num_reads;

struct SharedData {
  SharedData() : pub_ready(false)
               , pub_finished(false)
               , sub_ready(false)
               , sub_finished(false)
               , timeout_writes(0)
               , timeout_writes_ready(false) {}

  bool pub_ready;
  bool pub_finished;
  bool sub_ready;
  bool sub_finished;
  int timeout_writes;
  bool timeout_writes_ready;
};

extern const char* mmap_file;
extern const char* obj_name;

typedef ACE_Malloc_T<ACE_MMAP_MEMORY_POOL, ACE_Null_Mutex, ACE_PI_Control_Block> Allocator;

#endif /* COMMON_H */
