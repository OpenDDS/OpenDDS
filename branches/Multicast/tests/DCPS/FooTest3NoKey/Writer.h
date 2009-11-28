// -*- C++ -*-
//
// $Id$
#ifndef WRITER_H
#define WRITER_H

#include "InstanceDataMap.h"
#include "dds/DdsDcpsPublicationC.h"
#include "tests/DCPS/FooType3NoKey/FooDefC.h"
#include "tests/DCPS/FooType3NoKey/FooDefTypeSupportC.h"
#include "ace/Task.h"


class Writer : public ACE_Task_Base 
{
public:

  Writer (::DDS::DataWriter_ptr writer, 
          int num_thread_to_write = 1,
          int num_writes_per_thread = 1,
          int writer_id = -1);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  long writer_id () const;

  InstanceDataMap& data_map ();

private:

  ::Xyz::FooDataWriter_var foo_dw_; 
  InstanceDataMap       data_map_;
  ::DDS::DataWriter_var writer_;
  int num_thread_to_write_;
  int num_writes_per_thread_;
  long writer_id_;
  long handle_;
  // instance without key
  ::Xyz::Foo registered_foo_;
};

#endif /* WRITER_H */
