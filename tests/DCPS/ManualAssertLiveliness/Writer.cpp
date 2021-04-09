// -*- C++ -*-
//
#include "Writer.h"

#include "MessengerTypeSupportC.h"
#include "DataWriterListenerImpl.h"

#include "tests/Utils/ExceptionStreams.h"

#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/DomainParticipantImpl.h>

#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>

using namespace Messenger;
using namespace std;

extern int pub_num_messages;
extern int pub_assert_liveliness_period;
extern bool liveliness_lost_test;

Writer_Base::Writer_Base(DDS::DataWriter_ptr writer, const char* name)
  : writer_(DDS::DataWriter::_duplicate(writer))
  , name_(name)
{}

void
Writer_Base::start()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::start %C\n"), name_));

  DDS::DomainParticipant_var part = DDS::Publisher_var(writer_->get_publisher())->get_participant();
  OpenDDS::DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(part.in());

  OpenDDS::DCPS::LogGuid lg(dpi->get_repoid(writer_->get_instance_handle()));
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::start %C is %C\n"), name_, lg.c_str()));

  if (activate(THR_NEW_LWP | THR_JOINABLE, 1) == -1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Writer_Base::start(): activate failed\n")));
    exit(1);
  }
}

void
Writer_Base::end()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::end %C, calling wait\n"), name_));
  wait();
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::end %C, wait finished\n"), name_));
}

int
Writer_Base::svc()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::svc() %C begins\n"), name_));

  pre_loop();

  for (int i = 0; i < pub_num_messages; ++i) {
    if (!liveliness_lost_test || i == 0 || i == pub_num_messages - 1) {
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

    ACE_OS::sleep(pub_assert_liveliness_period);
  }

  post_loop();

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer_Base::svc() %C finished\n"), name_));

  return 0;
}

Write_Samples::Write_Samples(DDS::DataWriter_ptr writer, const char* name)
  : Writer_Base(writer, name)
  , handle_(DDS::HANDLE_NIL)
{}

void
Write_Samples::pre_loop()
{
  message_dw_ = MessageDataWriter::_narrow(writer_);

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
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %C writing msg\n"), name_));

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

void
Write_Samples::post_loop()
{
  message_dw_ = MessageDataWriter::_narrow(writer_);

  const DDS::Duration_t timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
  const DDS::ReturnCode_t ret =  message_dw_->wait_for_acknowledgments(timeout);
  if (ret != ::DDS::RETCODE_OK) {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: wait_for_acknowledgements: ")
                ACE_TEXT("%C returned %d\n"),
                name_, ret));
  }
}

Assert_Participant_Liveliness::Assert_Participant_Liveliness(
  DDS::DataWriter_ptr writer, const char* name)
  : Writer_Base(writer, name)
  , participant_(DDS::Publisher_var(writer->get_publisher())->get_participant())
{}

void
Assert_Participant_Liveliness::in_loop(int i)
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) %C Assert_Participant_Liveliness calling assert_liveliness\n"), name_));
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
{}

void
Assert_Writer_Liveliness::in_loop(int i)
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) %C Assert_Writer_Liveliness calling assert_liveliness\n"), name_));
  const DDS::ReturnCode_t ret = writer_->assert_liveliness();

  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Assert_Writer_Liveliness: ")
               ACE_TEXT ("%dth assert_liveliness() returned %d.\n"),
               i, ret));
  }
}
