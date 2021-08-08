/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Service_Participant.h>

#include "SubscriberListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <iostream>

extern unsigned int num_messages;

SubscriberListenerImpl::SubscriberListenerImpl()
  : verify_result_ (true)
{
}

SubscriberListenerImpl::~SubscriberListenerImpl()
{
}

void
SubscriberListenerImpl::on_data_on_readers(DDS::Subscriber_ptr subs)
{
  ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers - get qos\n"));
  DDS::SubscriberQos qos;
  ::DDS::ReturnCode_t ret = subs->get_qos (qos);
  if (ret != ::DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
                  ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                  ACE_TEXT(" ERROR: get_qos failed!\n")));
    ACE_OS::exit(-1);
  }
  ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers - get datareaders\n"));

  ::DDS::DataReaderSeq_var readers = new ::DDS::DataReaderSeq(100);

  ret = subs->get_datareaders(readers.inout(),
                              ::DDS::NOT_READ_SAMPLE_STATE,
                              ::DDS::ANY_VIEW_STATE,
                              ::DDS::ANY_INSTANCE_STATE);
  ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers got data readers\n"));

  if (ret != ::DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
                  ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                  ACE_TEXT(" ERROR: get_datareaders failed!\n")));
    ACE_OS::exit(-1);
  }

  CORBA::ULong len = readers->length ();

  if (qos.presentation.access_scope == ::DDS::GROUP_PRESENTATION_QOS) {
    ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers() ")
                ACE_TEXT("GROUP_PRESENTATION_QOS get_datareaders returned %d readers!\n"),
                  len));
    ret = subs->begin_access ();
    if (ret != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                ACE_TEXT(" ERROR: begin_access failed!\n")));
      ACE_OS::exit(-1);
    }

    // redirect datareader listener to receive DISPOSE and UNREGISTER notifications.
    if (len == 0) {
      if (subs->notify_datareaders () != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                    ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                    ACE_TEXT(" ERROR: notify_datareaders failed!\n")));
        this->verify_result_ = false;
      }
      ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers return on readers->len == 0\n"));
      return;
    }

    const size_t expecting = num_messages * 4;
    if (len != expecting) {
      ACE_ERROR((LM_ERROR,
                    ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                    ACE_TEXT(" ERROR: get_datareaders returned %d readers, expecting %d!\n"),
                    len, expecting));
      this->verify_result_ = false;
    }
    ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers len = %d\n",len));

    for (CORBA::ULong i = 0; i < len; ++i) {
      Messenger::MessageDataReader_var message_dr =
        Messenger::MessageDataReader::_narrow(readers[i]);

      Messenger::MessageSeq msg;
      ::DDS::SampleInfoSeq si;
      ret = message_dr->take(msg, si,
              DDS::LENGTH_UNLIMITED,
              ::DDS::NOT_READ_SAMPLE_STATE,
              ::DDS::ANY_VIEW_STATE,
              ::DDS::ANY_INSTANCE_STATE) ;
      if (msg.length() != 1 || si.length() != 1) {
        ACE_ERROR((LM_ERROR,
                    ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                    ACE_TEXT(" ERROR: MessageSeq %d SampleInfoSeq %d!\n"),
                    msg.length(), si.length()));
        this->verify_result_ = false;
      }

      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                    ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                    ACE_TEXT(" ERROR: read failed!\n")));
        ACE_OS::exit(-1);
      }

      ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers\n"));
      this->verify (msg[0], si[0], qos, false);
    }
    ret = subs->end_access ();
    if (ret != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                    ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                    ACE_TEXT(" ERROR: end_access failed!\n")));
      ACE_OS::exit(-1);
    }
  }
  else if (qos.presentation.access_scope == ::DDS::TOPIC_PRESENTATION_QOS) {
    ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("%N:%l:%t: SubscriberListenerImpl::on_data_on_readers() ")
                ACE_TEXT("TOPIC_PRESENTATION_QOS get_datareaders returned %d readers!\n"),
                  len));

    // redirect datareader listener to receive DISPOSE and UNREGISTER notifications.
    if (len != 2) {
      ACE_DEBUG((LM_DEBUG, "%N:%l:%t: notifying datareaders len = %d\n", len));
      if (subs->notify_datareaders () != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                    ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                    ACE_TEXT(" ERROR: notify_datareaders failed!\n")));
        this->verify_result_ = false;
      }
      ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers return after notify datareaders\n"));
    }
    else {
      ACE_DEBUG((LM_DEBUG,"%N:%l:%t: len == 2, iterating over datareader list\n"));

      for (CORBA::ULong i = 0; i < len; ++i) {
        Messenger::MessageDataReader_var message_dr =
          Messenger::MessageDataReader::_narrow(readers[i]);

          ACE_DEBUG((LM_DEBUG,"%N:%l:%t: calling message_dr->take\n"));
        Messenger::MessageSeq msg;
        ::DDS::SampleInfoSeq si;
        ret = message_dr->take(msg, si,
                DDS::LENGTH_UNLIMITED,
                ::DDS::NOT_READ_SAMPLE_STATE,
                ::DDS::ANY_VIEW_STATE,
                ::DDS::ANY_INSTANCE_STATE) ;
        if (ret != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                      ACE_TEXT("%N:%l:%t: SubscriberListenerImpl::on_data_on_readers()")
                      ACE_TEXT(" ERROR: read failed!\n")));
          continue;
        }
        ACE_DEBUG((LM_DEBUG,"%N:%l:%t: message_dr->take returned\n"));

        if (si[0].instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE
           || si[0].instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
             ACE_DEBUG((LM_DEBUG,"%N:%l:%t: not alive state detected\n"));
             this->verify(msg[0], si[0], qos, false);
             continue;
           }

        if (msg.length() != num_messages || si.length() != num_messages) {
          ACE_ERROR((LM_ERROR,
                      ACE_TEXT("%N:%l: SubscriberListenerImpl::on_data_on_readers()")
                      ACE_TEXT(" ERROR: MessageSeq %d SampleInfoSeq %d != %d\n"),
                      msg.length(), si.length(), num_messages));
          this->verify_result_ = false;
        }

        CORBA::ULong num_samples = si.length();
        ACE_DEBUG((LM_DEBUG, "%N:%l:%t: on_data_on_readers, verifying num_samples = %d\n", num_samples));
        for (CORBA::ULong i = 0; i < num_samples; ++i) {
          ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers - verify\n"));
          this->verify (msg[i], si[i], qos, i == (num_samples - 1));
          ACE_DEBUG((LM_DEBUG, "%N:%l:%t: on_data_on_readers - verify done\n"));
        }
      }
    }
  }
  else { //::DDS::INSTANCE_PRESENTATION_QOS
    ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers\n"));
    subs->notify_datareaders ();
  }

  ACE_DEBUG((LM_DEBUG,"%N:%l:%t: on_data_on_readers returning at end\n"));
}


void
SubscriberListenerImpl::verify (const Messenger::Message& msg,
                                const ::DDS::SampleInfo& si,
                                const DDS::SubscriberQos& qos,
                                const bool reset_last_timestamp)
{
  std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
  std::cout << "SampleInfo.instance_state = " << OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state) << std::endl;

  if (si.valid_data) {
    std::cout << "Message: subject    = " << msg.subject.in() << std::endl
              << "         subject_id = " << msg.subject_id   << std::endl
              << "         from       = " << msg.from.in()    << std::endl
              << "         count      = " << msg.count        << std::endl
              << "         text       = " << msg.text.in()    << std::endl;

  } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l:%t: INFO: instance is disposed\n")));
    this->verify_result_ = false;
  } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l:%t: INFO: instance is unregistered\n")));
    this->verify_result_ = false;
  } else {
    ACE_ERROR((LM_ERROR,
                ACE_TEXT("%N:%l: SubscriberListenerImpl::verify()")
                ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                si.instance_state));
    this->verify_result_ = false;
  }

  using OpenDDS::DCPS::operator<;
  if (qos.presentation.access_scope == ::DDS::GROUP_PRESENTATION_QOS) {
    static DDS::Time_t last_timestamp = { 0, 0 };

    if (si.source_timestamp < last_timestamp) {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("%N:%l SubscriberListenerImpl::verify()")
                ACE_TEXT(" ERROR: Samples taken out of order!\n")));
      this->verify_result_ = false;
    }
    last_timestamp = si.source_timestamp;
  }
  else { //TOPIC_PRESENTATION_QOS
    // two instances each from a writer, the samples from same writer should be
    // received in order.
    typedef std::map <CORBA::ULongLong, DDS::Time_t> InstanceTimeStamp;
    static InstanceTimeStamp timestamps;
    if (timestamps.find (msg.subject_id) == timestamps.end ()) {
      timestamps[msg.subject_id] = si.source_timestamp;
      return;
    }

    if (si.source_timestamp < timestamps[msg.subject_id]) {
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("%N:%l SubscriberListenerImpl::verify()")
                ACE_TEXT(" ERROR: Samples taken out of order!\n")));
      this->verify_result_ = false;
    }

    if (reset_last_timestamp) {
      timestamps.clear();
    }
    else {
      timestamps[msg.subject_id] = si.source_timestamp;
    }
  }
}

void SubscriberListenerImpl::on_data_available(DDS::DataReader_ptr /*reader*/)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_data_available()\n")));
}

void SubscriberListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
}

void SubscriberListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void SubscriberListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
}

void SubscriberListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
}

void SubscriberListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void SubscriberListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}
