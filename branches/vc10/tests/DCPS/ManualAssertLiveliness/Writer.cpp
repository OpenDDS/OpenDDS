// -*- C++ -*-
//
// $Id$
#include "Writer.h"
#include "MessengerTypeSupportC.h"
#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>

using namespace Messenger;

extern int num_messages;
extern int assert_liveliness_period;
extern bool liveliness_lost_test;


Writer_Base::Writer_Base(::DDS::DataWriter_ptr writer)
: writer_ (::DDS::DataWriter::_duplicate (writer))
{}

void
Writer_Base::start ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::start \n")));
  if (activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1) {
    cerr << "Writer_Base::start(): activate failed" << endl;
    exit(1);
  }
}

void
Writer_Base::end ()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer_Base::end \n")));
  wait ();
}


Manual_By_Participant_Writer_1::Manual_By_Participant_Writer_1(::DDS::DataWriter_ptr writer)
: Writer_Base (writer)
{
}


int
Manual_By_Participant_Writer_1::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Manual_By_Participant_Writer_1::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try {

    while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() > 0)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0,200000));
    }

    ::Messenger::MessageDataWriter_var message_dw
      = ::Messenger::MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil (message_dw.in ())) {
      cerr << "Data Manual_By_Participant_Writer_1 could not be narrowed"<< endl;
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
              ACE_TEXT("(%P|%t) %T Manual_By_Participant_Writer_1::svc starting to write.\n")));
    for (int i = 0; i< num_messages; i ++)
    {
      if (liveliness_lost_test &&  i > 0 && i < num_messages - 1)
      {
        ACE_OS::sleep (assert_liveliness_period);
        continue;
      }

      ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

      if (ret != ::DDS::RETCODE_OK) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Manual_By_Participant_Writer_1::svc, ")
                    ACE_TEXT ("%dth write() returned %d.\n"),
                    i, ret));
      }

      message.count++;

      ACE_OS::sleep (assert_liveliness_period);
    }

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        break;
      else
        ACE_OS::sleep(1);
    }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Manual_By_Participant_Writer_1::svc finished.\n")));

  return 0;
}


Manual_By_Participant_Writer_2::Manual_By_Participant_Writer_2(::DDS::DomainParticipant_ptr part,
               ::DDS::DataWriter_ptr writer)
    : Writer_Base(writer),
      participant_ (::DDS::DomainParticipant::_duplicate (part))
{
}


int
Manual_By_Participant_Writer_2::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Manual_By_Participant_Writer_2::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try {

    while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() > 0)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0,200000));
    }

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Manual_By_Participant_Writer_2::svc starting to send liveliness.\n")));
    for (int i = 0; i< num_messages; i ++)
    {
      if (liveliness_lost_test &&  i > 0 && i < num_messages - 1)
      {
        ACE_OS::sleep (assert_liveliness_period);
        continue;
      }

      ::DDS::ReturnCode_t ret = this->participant_->assert_liveliness ();
      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Manual_By_Participant_Writer_2::svc, ")
                    ACE_TEXT ("%dth assert_liveliness() returned %d.\n"),
                    i, ret));
      }
      ACE_OS::sleep (assert_liveliness_period);
    }

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        break;
      else
        ACE_OS::sleep(1);
    }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Manual_By_Participant_Writer_2::svc finished.\n")));

  return 0;
}



Manual_By_Topic_Writer_1::Manual_By_Topic_Writer_1(::DDS::DataWriter_ptr writer)
: Writer_Base (writer)
{
}


int
Manual_By_Topic_Writer_1::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Manual_By_Topic_Writer_1::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try {

    while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() > 0)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0,200000));
    }

    ::Messenger::MessageDataWriter_var message_dw
      = ::Messenger::MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil (message_dw.in ())) {
      cerr << "Data Manual_By_Topic_Writer_1 could not be narrowed"<< endl;
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
              ACE_TEXT("(%P|%t) %T Manual_By_Topic_Writer_1::svc starting to write.\n")));
    for (int i = 0; i< num_messages; i ++)
    {
      if (liveliness_lost_test &&  i > 0 && i < num_messages - 1)
      {
        ACE_OS::sleep (assert_liveliness_period);
        continue;
      }

      ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

      if (ret != ::DDS::RETCODE_OK) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Manual_By_Topic_Writer_1::svc, ")
                    ACE_TEXT ("%dth write() returned %d.\n"),
                    i, ret));
      }

      message.count++;

      ACE_OS::sleep (assert_liveliness_period);
    }

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        break;
      else
        ACE_OS::sleep(1);
    }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Manual_By_Topic_Writer_1::svc finished.\n")));

  return 0;
}


Manual_By_Topic_Writer_2::Manual_By_Topic_Writer_2(::DDS::DataWriter_ptr writer)
: Writer_Base (writer)
{
}


int
Manual_By_Topic_Writer_2::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Manual_By_Topic_Writer_2::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try {

    while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() > 0)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0,200000));
    }

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Manual_By_Topic_Writer_2::svc starting send liveliness.\n")));
    for (int i = 0; i< num_messages; i ++) {
      if (liveliness_lost_test &&  i > 0 && i < num_messages - 1)
      {
        ACE_OS::sleep (assert_liveliness_period);
        continue;
      }

      ::DDS::ReturnCode_t ret = writer_->assert_liveliness ();

      if (ret != ::DDS::RETCODE_OK) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Manual_By_Topic_Writer_2::svc, ")
                    ACE_TEXT ("%dth assert_liveliness() returned %d.\n"),
                    i, ret));
      }

      ACE_OS::sleep (assert_liveliness_period);
    }

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        break;
      else
        ACE_OS::sleep(1);
    }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Manual_By_Topic_Writer_2::svc finished.\n")));

  return 0;
}




