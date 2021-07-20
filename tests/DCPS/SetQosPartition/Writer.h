// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "MessengerTypeSupportC.h"

#include <dds/DCPS/PoolAllocator.h>
#include <dds/DdsDcpsPublicationC.h>

#include <ace/Task.h>

class Writer : public ACE_Task_Base {
public:
  Writer(const ::DDS::DataWriter_var& writer, const OPENDDS_STRING& name, int write_count);

  void start ();
  void end ();
  virtual int svc ();

private:
  const ::DDS::DataWriter_var writer_;
  const OPENDDS_STRING name_;
  const int write_count_;
};

#endif /* WRITER_H */
