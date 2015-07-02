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

#include "TopicMDataReaderListener.h"
#include <dds/monitor/monitorTypeSupportC.h>
#include <dds/monitor/monitorTypeSupportImpl.h>

#include <iostream>

using std::cout;
using std::endl;

TopicMDataReaderListenerImpl::TopicMDataReaderListenerImpl()
{
}

TopicMDataReaderListenerImpl::~TopicMDataReaderListenerImpl()
{
}

void TopicMDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
throw(CORBA::SystemException)
{
  try {
    OpenDDS::DCPS::TopicReportDataReader_var topicm_dr =
      OpenDDS::DCPS::TopicReportDataReader::_narrow(reader);

    if (CORBA::is_nil(topicm_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    OpenDDS::DCPS::TopicReport topicr;
    DDS::SampleInfo si;

    DDS::ReturnCode_t status = topicm_dr->take_next_sample(topicr, si) ;

    if (status == DDS::RETCODE_OK) {
      cout << "SampleInfo.sample_rank = " << si.sample_rank << endl;
      cout << "SampleInfo.instance_state = " << si.instance_state << endl;

      if (si.valid_data) {
        cout << "TopicReport:" << endl
             << "  dp_id      = " << topicr.dp_id           << endl
             << "  topic_id   = " << topicr.topic_id        << endl
             << "  topic_name = " << topicr.topic_name.in() << endl
             << "  topic_type = " << topicr.type_name.in()  << endl;

      } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));

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

void TopicMDataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
}

void TopicMDataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void TopicMDataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
}

void TopicMDataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
}

void TopicMDataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void TopicMDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}
