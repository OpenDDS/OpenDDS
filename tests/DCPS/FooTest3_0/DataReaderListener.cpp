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

#include "DataReaderListener.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportImpl.h"

#include <iostream>

DataReaderListenerImpl::DataReaderListenerImpl()
  : samples_read_(0),
    samples_disposed_(0)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  try {
    Xyz::FooDataReader_var foo_dr =
      Xyz::FooDataReader::_narrow(reader);

    if (CORBA::is_nil(foo_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    Xyz::Foo foo;
    DDS::SampleInfo si;

    const DDS::ReturnCode_t status = foo_dr->take_next_sample(foo, si) ;

    if (status == DDS::RETCODE_OK) {
      ++samples_read_;
      std::cout << "SampleInfo.valid_data = " << si.valid_data << std::endl;
      std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
      std::cout << "SampleInfo.instance_state = " << OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state) << std::endl;

      if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance %d is disposed\n"), foo.a_long_value));
        ++samples_disposed_;
      } else if (si.valid_data) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: Foo sample %d processed\n"), foo.sample_sequence));
      } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));
      } else {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: on_data_available()")
                   ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                   si.instance_state));
      }

    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: unexpected status: %d\n"),
                 status));
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    ACE_OS::exit(-1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
}

void DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}
