// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsPublicationC.h"
#include "ace/Task.h"
#include "dds/DCPS/TypeSupportImpl.h"

#include "tests/Utils/DistributedConditionSet.h"

class Writer : public ACE_Task_Base
{
public:

  Writer(DistributedConditionSet_rch dcs,
         DDS::DataWriter* writer);

  void start();
  void wait_for_done();

  int wait_match(const DDS::DataWriter_var& dw);

protected:

  void rsleep(const int wait = 100000) ; // microseconds
  void rsleep1() ;

  DistributedConditionSet_rch dcs_;
  int max_wait_ ;
};

template<typename MessageType>
class TypedWriter : public Writer
{
public:
  typedef OpenDDS::DCPS::DDSTraits<MessageType> TraitsType;
  typedef typename TraitsType::DataWriterType::_var_type DWVar;
  typedef typename TraitsType::MessageType DSample;
  typedef void (*DSModifier)(DSample&, int);

  explicit TypedWriter(DistributedConditionSet_rch dcs,
                       const OpenDDS::DCPS::String& topic_name,
                       const DWVar& dw)
  : Writer(dcs, dw)
  , topic_name_(topic_name)
  , writer_(dw)
  , init_instance_(noop)
  , next_sample_(noop)
  {}

  void init_instance_handler(const DSModifier& init) { init_instance_ = init; }
  void next_sample_handler(const DSModifier& next) { next_sample_ = next; }

private:
  int svc();

  static void noop(DSample&, int) {}

  const OpenDDS::DCPS::String topic_name_;
  const DWVar writer_;
  DSModifier init_instance_, next_sample_;
};

template<typename TSI>
int TypedWriter<TSI>::svc()
{
  try
  {
    ::DDS::Topic_var topic = writer_->get_topic();

    ACE_DEBUG((LM_DEBUG,"(%P|%t) %C: TypedWriter::svc begins.\n",
      CORBA::String_var(topic->get_name()).in()));

    const DDS::DataWriter_var dw = DDS::DataWriter::_duplicate(writer_);
    wait_match(dw);

    dcs_->wait_for(topic_name_ + " writer", topic_name_ + " reader1", "ready");
    dcs_->wait_for(topic_name_ + " writer", topic_name_ + " reader2", "ready");

    DSample foo;
    init_instance_(foo, 0);
    rsleep1();

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) %T TypedWriter::svc starting to write.\n")));

    ::DDS::InstanceHandle_t handle = writer_->register_instance(foo);

    for (int i = 0; i < 10; ++i)
    {
      rsleep();
      next_sample_(foo, i);
      writer_->write(foo, handle);
    }

    dcs_->wait_for(topic_name_ + " writer", topic_name_ + " reader1", "done");
    dcs_->wait_for(topic_name_ + " writer", topic_name_ + " reader2", "done");
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception("Exception caught in svc:");
  }

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::svc finished.\n")));
  return 0;
}


#endif /* WRITER_H */
