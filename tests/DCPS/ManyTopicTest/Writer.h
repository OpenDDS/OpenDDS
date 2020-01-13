// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsPublicationC.h"
#include "ace/Task.h"
#include "dds/DCPS/TypeSupportImpl.h"

class Writer : public ACE_Task_Base
{
public:

  Writer(DDS::DataWriter* writer,
         int num_thread_to_write = 1,
         int num_writes_per_thread = 100);

  void start();

  void end();

  bool is_finished() const;
  int wait_match(const DDS::DataWriter_var& dw, unsigned int count);

protected:

  void rsleep(const int wait = 100000) ; // microseconds
  void rsleep1() ;

  int num_thread_to_write_;
  int num_writes_per_thread_;
  bool finished_sending_;
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

  explicit TypedWriter(const DWVar& dw, int num_thread_to_write = 1,
                       int num_writes_per_thread = 100)
  : Writer(dw, num_thread_to_write, num_writes_per_thread)
  , writer_(dw)
  , init_instance_(noop)
  , next_sample_(noop)
  {}

  void init_instance_handler(const DSModifier& init) { init_instance_ = init; }
  void next_sample_handler(const DSModifier& next) { next_sample_ = next; }

private:
  int svc();

  static void noop(DSample&, int) {}

  const DWVar writer_;
  DSModifier init_instance_, next_sample_;
};

template<typename TSI>
int TypedWriter<TSI>::svc()
{
  try
  {
    finished_sending_ = false;

    ::DDS::Topic_var topic = writer_->get_topic();

    ACE_DEBUG((LM_DEBUG,"(%P|%t) %C: TypedWriter::svc begins.\n",
      CORBA::String_var(topic->get_name()).in()));

    const DDS::DataWriter_var dw = DDS::DataWriter::_duplicate(writer_);
    wait_match(dw, 1);

    DSample foo;
    init_instance_(foo, 0);
    rsleep1();

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) %T TypedWriter::svc starting to write.\n")));

    ::DDS::InstanceHandle_t handle = writer_->register_instance(foo);

    for (int i = 0; i < num_writes_per_thread_; ++i)
    {
      rsleep();
      next_sample_(foo, i);
      writer_->write(foo, handle);
    }

  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception("Exception caught in svc:");
  }

  finished_sending_ = true;

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::svc finished.\n")));
  return 0;
}


#endif /* WRITER_H */
