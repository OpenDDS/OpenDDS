// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "MessengerTypeSupportC.h"
#include "DataWriterListenerImpl.h"

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/TimeTypes.h>
#include <dds/DCPS/ConditionVariable.h>

#include <ace/Synch_Traits.h>
#include <ace/Task.h>

class Writer : public ACE_Task_Base
{
public:
  Writer(const DDS::DataWriter_var& dw);
  bool start(); // Launch 2 threads
  void end();
  virtual int svc();

private:
  typedef ACE_SYNCH_MUTEX Mutex;
  typedef ACE_Guard<Mutex> Lock;
  int get_key();

  Messenger::MessageDataWriter_var mdw_;
  Mutex key_mutex_;
  int key_n_;
};

#endif /* WRITER_H */
