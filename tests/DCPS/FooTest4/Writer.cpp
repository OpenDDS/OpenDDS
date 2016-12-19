// -*- C++ -*-
//
// -*- C++ -*-
//
#include "Writer.h"
#include "../common/TestException.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

const int default_key = 101010;


Writer::Writer(::DDS::DataReader_ptr reader,
               int num_writes_per_thread,
               int multiple_instances,
               int /*instance_id*/)
: num_writes_per_thread_(num_writes_per_thread),
  multiple_instances_(multiple_instances),
  reader_(reader)
{
}

void
Writer::start()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start \n")));

  try {
    OpenDDS::DCPS::DataReaderImpl* dr_servant =
      dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(reader_);

    ::Xyz::Foo foo;
    foo.x = 0.0;
    foo.y = 0.0;

    ::OpenDDS::DCPS::SequenceNumber seq;

    if (!multiple_instances_) {
      foo.key = default_key;
    }

    for (int i = 0; i < num_writes_per_thread_; ++i) {
      ++seq;

      foo.x = (float)i;
      foo.y = (float)i;

      if (multiple_instances_) {
        foo.key = i + 1;
      }

      ACE_Time_Value now = ACE_OS::gettimeofday();

      ACE_OS::printf("\"writing\" foo.x = %f foo.y = %f, foo.key = %d\n",
                     foo.x, foo.y, foo.key);
      OpenDDS::DCPS::ReceivedDataSample sample(0);

      sample.header_.message_length_ = sizeof(foo);
      sample.header_.message_id_ = OpenDDS::DCPS::SAMPLE_DATA;
      sample.header_.sequence_ = seq.getValue();

      // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
      // generated here are for the sole purpose of verifying internal behavior.
      OpenDDS::DCPS::RepoIdBuilder builder(sample.header_.publication_id_);

      builder.participantId(1);
      builder.entityKey(1);
      builder.entityKind(OpenDDS::DCPS::ENTITYKIND_OPENDDS_NIL_WRITER);

      sample.header_.source_timestamp_sec_ = static_cast<ACE_INT32>(now.sec());
      sample.header_.source_timestamp_nanosec_ = now.usec() * 1000;

      sample.sample_ = new ACE_Message_Block(sizeof(foo));

      ::OpenDDS::DCPS::Serializer ser(sample.sample_);
      ser << foo;

      dr_servant->data_received(sample);
    }
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("Exception caught in svc:");
  }
}

