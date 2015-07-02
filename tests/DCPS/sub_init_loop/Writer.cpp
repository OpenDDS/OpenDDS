// -*- C++ -*-
//
#include "Writer.h"
#include "MessengerTypeSupportC.h"

#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>
#include "ace/OS_NS_sys_stat.h"

Writer::Writer (::DDS::DataWriter_ptr writer
                , const char* sub_fin_file_name
                , bool verbose
                , int write_delay_ms
                , int num_instances_per_writer)
  : writer_ (::DDS::DataWriter::_duplicate (writer))
  , sub_fin_file_name_ (sub_fin_file_name)
  , verbose_ (verbose)
  , write_delay_ms_ (write_delay_ms)
  , num_instances_per_writer_ (num_instances_per_writer)
  , timeout_writes_ (0)
{
}

bool
Writer::start ()
{
  if (this->verbose_) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start \n")));
  }

  // Lanuch num_instances_per_writer threads.
  // Each thread writes one instance which uses the thread id as the
  // key value.
  if (activate (THR_NEW_LWP | THR_JOINABLE
                , this->num_instances_per_writer_) == -1) {
    ACE_ERROR_RETURN ((LM_ERROR,
                       "Writer::start(): activate failed.\n")
                      , false);
  }

  return true;
}

int
Writer::svc ()
{
  if (this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Writer::svc begins.\n")));
  }

  try
    {
      Messenger::MessageDataWriter_var message_dw =
        Messenger::MessageDataWriter::_narrow(writer_.in());

      if (CORBA::is_nil (message_dw.in ())) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           "Data Writer could not be narrowed.\n")
                          , -1);
      }

      Messenger::Message message;
      message.subject_id = 99;
      ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

      message.from       = CORBA::string_dup("Comic Book Guy");
      message.subject    = CORBA::string_dup("Review");
      message.text       = CORBA::string_dup("Worst. Movie. Ever.");
      message.count      = 0;

      char *sub_fin_file_name = const_cast<char*>(this->sub_fin_file_name_.c_str());

      if (this->verbose_) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));
      }

      // Wait for subscribers
      while (true)
        {
          ::DDS::InstanceHandleSeq handles;
          message_dw->get_matched_subscriptions(handles);
          if (handles.length() > 0) {
            break;
          }
          ACE_Time_Value small_time (0,250000);
          ACE_OS::sleep (small_time);
        }

      // Begin write cycle.
      while (true)
        {
          ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

          if (ret != ::DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                          ACE_TEXT ("write() returned %d.\n")
                          , ret));
              if (ret == ::DDS::RETCODE_TIMEOUT) {
                timeout_writes_ ++;
              }
            }

          message.count++;
          if (this->write_delay_ms_ > 0) {
            ACE_OS::sleep (ACE_Time_Value (write_delay_ms_/1000, write_delay_ms_%1000*1000));
          }

          // Wait for the subscriber to finish.
          ACE_stat stats;
          if  (ACE_OS::stat (sub_fin_file_name, &stats) != -1)
            {
              if (this->verbose_) {
                ACE_DEBUG ((LM_DEBUG,
                            "(%P|%t) Subscriber has exited. "
                            "No need to publish any further.\n"));
              }

              break;
            }
        } // while (true)
    }
  catch (CORBA::Exception& e) {
    ACE_ERROR_RETURN ((LM_ERROR, "Exception caught in svc: %C (%C).\n"
                       ,  e._name (), e._rep_id ())
                      , -1);
  }

  return 0;
}
