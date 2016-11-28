// -*- C++ -*-
//
#include "Writer.h"
//#include "TestException.h"
#include "testMessageTypeSupportC.h"
#include "testMessageTypeSupportImpl.h"
#include "dds/DCPS/Service_Participant.h"

#include "ace/Condition_T.h"
#include "ace/Condition_Recursive_Thread_Mutex.h"
#include "ace/OS_NS_unistd.h"

extern ACE_Condition<ACE_Recursive_Thread_Mutex> done_condition_;

void write (long id,
            int size,
            int num_messages,
            ::DDS::DataWriter_ptr writer)
{
  profilingTest::testMsg data;
  data.msgID = id;
  data.contents.length(size);

  for (int i = 0; i < size; i ++)
  {
    data.contents[i] = (char) i % 256;
  }

  ::profilingTest::testMsgDataWriter_var pt_dw
    = ::profilingTest::testMsgDataWriter::_narrow(writer);
  ACE_ASSERT (! CORBA::is_nil (pt_dw.in ()));

  ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));

  ::DDS::InstanceHandle_t handle
      = pt_dw->register_instance(data);

  for (int i = 0; i < num_messages; i ++)
  {
    data.count = i;
    pt_dw->write(data, handle);
  }
}




Writer::Writer(::DDS::DataWriter_ptr writer,
               int num_messages,
               int data_size,
               int num_readers,
               int writer_id)
               : writer_ (::DDS::DataWriter::_duplicate (writer)),
  num_messages_ (num_messages),
  data_size_ (data_size),
  num_readers_(num_readers),
  writer_id_ (writer_id),
  finished_sending_ (false)
{
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Writer::start \n")));
  if (activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) Writer::start, %p.\n"),
                ACE_TEXT("activate")));
  }
}

void
Writer::end ()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::end \n")));
  wait ();
}


int
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc begins samples with %d bytes.\n"),
              data_size_));

  try
  {
    int num_connected_subs = 0;
    ::DDS::InstanceHandleSeq handles;
    while (num_connected_subs != num_readers_)
      {
        ACE_OS::sleep(1);
        writer_->get_matched_subscriptions(handles);
        num_connected_subs = handles.length();
      }

        write (writer_id_,
                  data_size_,
                  num_messages_,
                  writer_.in ());


  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
  }

  ::DDS::InstanceHandleSeq handles;
  while (!finished_sending_)
    {
      ACE_OS::sleep(1);
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        {
          finished_sending_ = true;
        }
    }

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc finished.\n")));
  done_condition_.signal(); // tell publisher look if I am finished.
  return 0;
}

long
Writer::writer_id () const
{
  return writer_id_;
}


bool
Writer::is_finished () const
{
  return finished_sending_;
}
