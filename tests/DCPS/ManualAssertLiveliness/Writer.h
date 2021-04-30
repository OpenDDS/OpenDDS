// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "MessengerTypeSupportC.h"

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/PoolAllocator.h>

#include <ace/Task.h>

class Writer_Base : public ACE_Task_Base
{
public:
  Writer_Base(DDS::DataWriter_ptr writer, const char* name);
  void start();
  void end();

protected:
  virtual void pre_loop() {}
  virtual void in_loop(int i) = 0;
  virtual void post_loop() {}

  ::DDS::DataWriter_var writer_;
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
  void post_loop();

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
