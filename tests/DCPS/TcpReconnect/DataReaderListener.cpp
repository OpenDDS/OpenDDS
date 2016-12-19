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

#include "Args.h"
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <iostream>

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0)
  , valid_(true)
  , reliable_(is_reliable())
{
  std::cout << "Transport is " << (reliable_ ? "" : "UN-") << "RELIABLE" <<  std::endl;
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

bool
DataReaderListenerImpl::is_reliable()
{
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return !(gc->instances_[0]->transport_type_ == "udp");
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
throw(CORBA::SystemException)
{
  ++num_reads_;

  try {
    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(message_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    Messenger::Message message;
    DDS::SampleInfo si;

    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si) ;

    if (status == DDS::RETCODE_OK) {
      std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
      std::cout << "SampleInfo.instance_state = " << si.instance_state << std::endl;

      if (si.valid_data) {
        if (!counts_.insert(message.count).second) {
          std::cout << "ERROR: Repeat ";
          valid_ = false;
        }

        phases_.insert(message.phase_number);

        std::cout << "Message: subject    = " << message.subject.in() << std::endl
                  << "         subject_id = " << message.subject_id   << std::endl
                  << "         from       = " << message.from.in()    << std::endl
                  << "         count      = " << message.count        << std::endl
                  << "         phase      = " << message.phase_number << std::endl
                  << "         text       = " << message.text.in()    << std::endl;

        if (std::string("Comic Book Guy") != message.from.in() &&
            std::string("OpenDDS-Java") != message.from.in()) {
          std::cout << "ERROR: Invalid message.from" << std::endl;
          valid_ = false;
        }
        if (std::string("Review") != message.subject.in()) {
          std::cout << "ERROR: Invalid message.subject" << std::endl;
          valid_ = false;
        }
        if (std::string("Worst. Movie. Ever.") != message.text.in()) {
          std::cout << "ERROR: Invalid message.text" << std::endl;
          valid_ = false;
        }
        if (message.subject_id != 99) {
          std::cout << "ERROR: Invalid message.subject_id" << std::endl;
          valid_ = false;
        }
      } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));

      } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));

      } else {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: on_data_available()")
                   ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                   si.instance_state));
        valid_ = false;
      }

    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: unexpected status: %d\n"),
                 status));
      valid_ = false;
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    ACE_OS::exit(-1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
}

void DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}

bool DataReaderListenerImpl::is_valid() const
{
  CORBA::Long expected = 0;
  Phases::const_iterator phases_itr = phases_.begin();
  bool valid_phases = true;

  while (phases_itr != phases_.end() && expected < (stub_kills + 1)) {
    if (expected != *phases_itr) {
      if (expected < *phases_itr) {
        std::cout << "ERROR: did not receive message for phase: " << expected << std::endl;
        expected = *phases_itr;
        valid_phases = false;
        continue;
      } else {
        //This shouldn't happen ever.  Phase can't be less than expected.
        std::cout << "ERROR: received message for phase " << *phases_itr
                  << "when expecting for phase " << expected << std::endl;
        valid_phases = false;
        ++phases_itr;
      }
    }
    ++expected;
    ++phases_itr;
  }

  if (phases_itr != phases_.end()) {
    std::cout << "ERROR: received messages with phase higher than expected values"
              << "(current phases_iter is " << *phases_itr << ")" << std::endl;
    valid_phases = false;
  }

  return valid_ && valid_phases;
}
