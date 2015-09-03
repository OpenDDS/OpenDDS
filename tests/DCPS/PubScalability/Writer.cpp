// -*- C++ -*-
//
// -*- C++ -*-
//
#include "Writer.h"
#include "PubDriver.h"
#include "TestException.h"
#include "ace/Atomic_Op_T.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "SampleTypeTypeSupportC.h"
#include "tests/DCPS/common/TestSupport.h"

const int default_key = 101010;
ACE_Atomic_Op<ACE_SYNCH_MUTEX, CORBA::Long> key(0);

Writer::Writer(PubDriver*            /*pubdriver*/,
               ::DDS::DataWriter_ptr writer,
               int num_thread_to_write,
               int num_writes_per_thread,
               int multiple_instances,
               int writer_id,
               int have_key,
               int write_delay_msec,
               int check_data_dropped,
               int num_readers)
: writer_ (::DDS::DataWriter::_duplicate(writer)),
  writer_servant_ (0),
  num_thread_to_write_ (num_thread_to_write),
  num_writes_per_thread_ (num_writes_per_thread),
  multiple_instances_ (multiple_instances),
  writer_id_ (writer_id),
  has_key_ (have_key),
  write_delay_msec_ (write_delay_msec),
  check_data_dropped_ (check_data_dropped),
  num_readers_(num_readers),
  finished_(false)
{
  writer_servant_
    = dynamic_cast<OpenDDS::DCPS::DataWriterImpl*>(writer_.in());
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Writer::start \n")));
  if (activate (THR_NEW_LWP | THR_JOINABLE, num_thread_to_write_) == -1)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) Writer::start, %p.\n"),
                ACE_TEXT("activate")));
    throw TestException ();
  }
}

void
Writer::end ()
{
  wait ();


  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::end \n")));
}


int
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc \n")));

  // Wait for the subscriber to be ready...
  ::DDS::InstanceHandleSeq handles;
  while (1) {
    writer_->get_matched_subscriptions(handles);
    if (handles.length() >= (unsigned int) num_readers_) {
      break;
    } else {
      ACE_DEBUG((LM_DEBUG,"(%P|%t) Waiting\n"));
      ACE_OS::sleep(ACE_Time_Value(0,200000));
    }
  }

  try
  {
    ::Xyz::SampleType foo;
    foo.sample_sequence = -1;
    foo.handle_value = -1;
    foo.writer_id = writer_id_;

    if (multiple_instances_ == 1)
    {
      // Use the thread id as the instance key.
      foo.a_long_value = ++key;
    }
    else
    {
      foo.a_long_value = default_key;
    }

    ::Xyz::SampleTypeDataWriter_var foo_dw
      = ::Xyz::SampleTypeDataWriter::_narrow(writer_.in ());

    TEST_CHECK (! CORBA::is_nil (foo_dw.in ()));

    ACE_DEBUG((LM_DEBUG,"(%P|%t) Beginning writes\n"));
    for (int i = 0; i< num_writes_per_thread_; i ++)
    {
      ::DDS::InstanceHandle_t handle
        = foo_dw->register_instance(foo);
      foo.handle_value = handle;
      foo.sample_sequence = i;

      // The sequence number will be increased after the insert.
      TEST_CHECK (data_map_.insert (handle, foo) == 0);

      ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

      if (has_key_ == 1)
      {
        ::Xyz::SampleType key_holder;
        ret = foo_dw->get_key_value(key_holder, handle);

        TEST_CHECK(ret == ::DDS::RETCODE_OK);
        // check for equality
        TEST_CHECK (foo.a_long_value == key_holder.a_long_value); // It is the instance key.
      }

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG ((LM_DEBUG, "(%P|%t) write sample: %d \n", foo.sample_sequence));
      }

      ret = foo_dw->write(foo,
                          handle);
      TEST_CHECK (ret == ::DDS::RETCODE_OK);

      if (write_delay_msec_ > 0)
      {
        ACE_Time_Value delay (write_delay_msec_/1000, write_delay_msec_%1000 * 1000);
        ACE_OS::sleep (delay);
      }
    }
    ACE_DEBUG((LM_DEBUG,"(%P|%t) Finished writes\n"));
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
  }

  if (check_data_dropped_ == 1 && writer_servant_->data_dropped_count_ > 0)
  {
    while (writer_servant_->data_delivered_count_ + writer_servant_->data_dropped_count_
    < num_writes_per_thread_ * num_thread_to_write_)
    {
      ACE_OS::sleep (1);
    }

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Writer::svc data_delivered_count=%d data_dropped_count=%d\n"),
      writer_servant_->data_delivered_count_, writer_servant_->data_dropped_count_));

  }

  while (true) {
    writer_->get_matched_subscriptions(handles);
    if (handles.length() == 0) {
      break;
    } else {
      ACE_DEBUG((LM_DEBUG,"(%P|%t) Waiting for matched subscriptions to get to zero\n"));
      ACE_OS::sleep(ACE_Time_Value(0,200000));
    }
  }

  ACE_DEBUG((LM_DEBUG,"(%P|%t) Leaving writer thread\n"));

  finished_ = true;

  return 0;
}

long
Writer::writer_id () const
{
  return writer_id_;
}


InstanceDataMap&
Writer::data_map ()
{
  return data_map_;
}

