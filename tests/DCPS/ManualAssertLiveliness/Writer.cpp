// -*- C++ -*-
//
#include "Writer.h"
#include "MessengerTypeSupportC.h"
#include "DataWriterListenerImpl.h"
#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/DomainParticipantImpl.h"

using namespace Messenger;
using namespace std;

extern int num_messages;
extern int assert_liveliness_period;
extern bool liveliness_lost_test;


Writer_Base::Writer_Base(DDS::DataWriter_ptr writer, const char* name)
  : writer_(DDS::DataWriter::_duplicate(writer))
  , name_(name)
{}

void
Writer_Base::start()
{
  DDS::DomainParticipant_var part = DDS::Publisher_var(writer_->get_publisher())->get_participant();
  id_ = OPENDDS_STRING(OpenDDS::DCPS::GuidConverter(dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(part.in())->get_repoid(writer_->get_instance_handle())));

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::start %C\n"), get_id()));
  if (activate(THR_NEW_LWP | THR_JOINABLE, 1) == -1) {
    cerr << "Writer_Base::start(): activate failed" << endl;
    exit(1);
  }
}

void
Writer_Base::end()
{
  wait();

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::end %C\n"), get_id()));
}

int
Writer_Base::svc()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %C svc() begins. %C\n"), name_, get_id()));

  try {
    DDS::InstanceHandleSeq handles;
    while (true) {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() > 0)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0, 200000));
    }

    pre_loop();

    for (int i = 0; i < num_messages; ++i) {
      if (!liveliness_lost_test || i == 0 || i == num_messages - 1) {
        in_loop(i);
        if (i == 0) {
          // Since an arbitrary amount of time was spent in the get_matched_subscriptions
          // loop above, any number of liveliness lost callbacks may have already occurred.
          // These are not the callbacks that the test is counting (liveliness_lost_test).
          DDS::DataWriterListener_var dwl = writer_->get_listener();
          DataWriterListenerImpl* listener = dynamic_cast<DataWriterListenerImpl*>(dwl.in());
          listener->reset_liveliness_lost_callbacks();
        }
      }

      ACE_OS::sleep(assert_liveliness_period);
    }

    while (true) {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        break;
      else
        ACE_OS::sleep(1);
    }

  } catch (const CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl << e << endl;
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::svc finished %C.\n"), name_));

  return 0;
}

Write_Samples::Write_Samples(DDS::DataWriter_ptr writer, const char* name)
  : Writer_Base(writer, name)
{
}

void
Write_Samples::pre_loop()
{
  message_dw_ = MessageDataWriter::_narrow(writer_);
  if (CORBA::is_nil(message_dw_)) {
    cerr << "MessageDataWriter could not be narrowed (" << name_ << ")" << endl;
    exit(1);
  }

  message_.subject_id = 99;
  handle_ = message_dw_->register_instance(message_);

  message_.from = name_;
  message_.subject = "Review";
  message_.text = "Worst. Movie. Ever.";
  message_.count = 0;
}

void
Write_Samples::in_loop(int i)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %C writing msg. %C\n"), name_, get_id()));

  DDS::ReturnCode_t ret;
  do {
    ret = message_dw_->write(message_, handle_);
  } while (ret == ::DDS::RETCODE_TIMEOUT);

  if (ret != ::DDS::RETCODE_OK) {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: Write_Samples: ")
                ACE_TEXT("%dth write() returned %d %C\n"),
                i, ret, name_));
  }

  ++message_.count;
}


Assert_Participant_Liveliness::Assert_Participant_Liveliness(
  DDS::DataWriter_ptr writer, const char* name)
  : Writer_Base(writer, name)
  , participant_(DDS::Publisher_var(writer->get_publisher())->get_participant())
{
}


void
Assert_Participant_Liveliness::in_loop(int i)
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Assert_Participant_Liveliness calling assert_liveliness. %C\n"), get_id()));
  const DDS::ReturnCode_t ret = participant_->assert_liveliness();

  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Assert_Participant_Liveliness: ")
               ACE_TEXT("%dth assert_liveliness() returned %d.\n"),
               i, ret));
  }
}


Assert_Writer_Liveliness::Assert_Writer_Liveliness(DDS::DataWriter_ptr writer, const char* name)
  : Writer_Base(writer, name)
{
}


void
Assert_Writer_Liveliness::in_loop(int i)
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Assert_Writer_Liveliness calling assert_liveliness. %C\n"), get_id()));
  const DDS::ReturnCode_t ret = writer_->assert_liveliness();

  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Assert_Writer_Liveliness: ")
               ACE_TEXT ("%dth assert_liveliness() returned %d.\n"),
               i, ret));
  }
}
