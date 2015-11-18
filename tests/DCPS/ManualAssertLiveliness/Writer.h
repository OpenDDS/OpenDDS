// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include <ace/Task.h>
#include "dds/DCPS/PoolAllocator.h"
#include "MessengerTypeSupportC.h"

class Writer_Base : public ACE_Task_Base
{
public:
  Writer_Base(DDS::DataWriter_ptr writer, const char* name);
  void start();
  void end();

protected:
  const char* get_id() const { return id_.c_str(); }
  virtual void pre_loop() {}
  virtual void in_loop(int i) = 0;

  ::DDS::DataWriter_var writer_;
  OPENDDS_STRING id_;
  const char* name_;

private:
  int svc();
};

class Write_Samples : public Writer_Base
{
public:
  Write_Samples(DDS::DataWriter_ptr writer, const char* name);

private:
  void pre_loop();
  void in_loop(int i);

  Messenger::MessageDataWriter_var message_dw_;
  Messenger::Message message_;
  DDS::InstanceHandle_t handle_;
};


class Assert_Participant_Liveliness : public Writer_Base
{
public:
  Assert_Participant_Liveliness(DDS::DataWriter_ptr writer, const char* name);
private:
  void in_loop(int i);
  DDS::DomainParticipant_var participant_;
};


class Assert_Writer_Liveliness : public Writer_Base
{
public:
  Assert_Writer_Liveliness(DDS::DataWriter_ptr writer, const char* name);
private:
  void in_loop(int i);
};


#endif /* WRITER_H */
