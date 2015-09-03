// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "InstanceDataMap.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "ace/Task.h"


class PubDriver;

class Writer : public ACE_Task_Base
{
public:

  Writer (PubDriver*            pubdriver,
          ::DDS::DataWriter_ptr writer,
          int num_thread_to_write = 1,
          int num_writes_per_thread = 1,
          int multiple_instances = 0,
          int writer_id = -1,
          int have_key = 1,
          int write_delay_msec = 0,
          int data_dropped = 0,
          int num_readers = 1);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  long writer_id () const;

  InstanceDataMap& data_map ();

  bool finished() { return finished_; }

private:

  InstanceDataMap       data_map_;
  ::DDS::DataWriter_var writer_;
  ::OpenDDS::DCPS::DataWriterImpl* writer_servant_;
  int num_thread_to_write_;
  int num_writes_per_thread_;
  int multiple_instances_;
  long writer_id_;
  int has_key_;
  int write_delay_msec_;
  int check_data_dropped_;
  int num_readers_;

  bool finished_;
};

#endif /* WRITER_H */
