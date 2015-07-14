// -*- C++ -*-
//
#include "Writer.h"
#include "MessengerTypeSupportC.h"

#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/High_Res_Timer.h"

using namespace Messenger;
using namespace std;

const int num_instances_per_writer = 1;

Writer::Writer(::DDS::DataWriter_ptr writer
               , size_t msg_cnt, size_t sub_cnt)
: writer_ (::DDS::DataWriter::_duplicate (writer)),
  finished_instances_ (0),
  timeout_writes_ (0),
  msg_cnt_ (msg_cnt),
  sub_cnt_ (sub_cnt)
{
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start \n")));
  // Lanuch num_instances_per_writer threads.
  // Each thread writes one instance which uses the thread id as the
  // key value.
  if (activate (THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    cerr << "Writer::start(): activate failed" << endl;
    exit(1);
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
              ACE_TEXT("(%P|%t) Writer::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try
    {
      MessageDataWriter_var message_dw
        = MessageDataWriter::_narrow(writer_.in());
      if (CORBA::is_nil (message_dw.in ())) {
        cerr << "Data Writer could not be narrowed"<< endl;
        exit(1);
      }

      Messenger::Message message;
      message.subject_id = 99;
      ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

      message.from       = CORBA::string_dup("Comic Book Guy");
      message.subject    = CORBA::string_dup("Review");
      message.text       = CORBA::string_dup("Worst. Movie. Ever.");
      message.count      = 0;

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));

      for (size_t sub_count = 0; sub_count < sub_cnt_; sub_count++)
        {
          // Wait for subscriber to start up
          while (true)
            {
              writer_->get_matched_subscriptions(handles);
              if (handles.length() > 0)
                break;
              else
                ACE_OS::sleep(ACE_Time_Value(0,200000));
            }

          // Push messages
          for (size_t msg_count = 0; msg_count < msg_cnt_; msg_count++)
            {
              ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

              if (ret != ::DDS::RETCODE_OK) {
                ACE_ERROR ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                            ACE_TEXT ("%dth write() returned %d.\n"),
                            msg_count, ret));
                if (ret == ::DDS::RETCODE_TIMEOUT) {
                  timeout_writes_ ++;
                }
              }
              message.count++;
            }

          // Wait for subscriber to shut down
          while (1)
            {
              writer_->get_matched_subscriptions(handles);
              if (handles.length() == 0)
                break;
              else
                ACE_OS::sleep(ACE_Time_Value(0,200000));
            }
        }
    }
  catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  finished_instances_ ++;

  return 0;
}


bool
Writer::is_finished () const
{
  return finished_instances_ == num_instances_per_writer;
}

int
Writer::get_timeout_writes () const
{
  return timeout_writes_.value ();
}
