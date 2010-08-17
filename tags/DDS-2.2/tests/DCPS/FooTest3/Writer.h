// -*- C++ -*-
//
// $Id$
#ifndef WRITER_H
#define WRITER_H

#include "InstanceDataMap.h"
#include "dds/DdsDcpsPublicationC.h"
#include "ace/Task.h"


class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer,
          int num_thread_to_write = 1,
          int num_writes_per_thread = 1,
          int multiple_instances = 0,
          int writer_id = -1);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  long writer_id () const;

  InstanceDataMap& data_map ();

private:

  InstanceDataMap       data_map_;
  ::DDS::DataWriter_var writer_;
  int num_thread_to_write_;
  int num_writes_per_thread_;
  int multiple_instances_;
  long writer_id_;
};

#endif /* WRITER_H */
