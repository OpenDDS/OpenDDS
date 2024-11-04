// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Atomic.h>

#include <ace/Task.h>

class Writer : public ACE_Task_Base
{
public:

  Writer (DistributedConditionSet_rch dcs,
          ::DDS::DataWriter_ptr writer);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

private:
  DistributedConditionSet_rch dcs_;
  ::DDS::DataWriter_var writer_;
};

#endif /* WRITER_H */
