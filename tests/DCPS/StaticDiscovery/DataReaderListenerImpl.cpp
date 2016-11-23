/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include "DataReaderListenerImpl.h"
#include "TestMsgTypeSupportC.h"
#include "TestMsgTypeSupportImpl.h"
#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/DdsDcpsCoreTypeSupportC.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include <iostream>

DataReaderListenerImpl::~DataReaderListenerImpl()
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DataReader %C is done\n", id_.c_str()));
  if (expected_samples_ && received_samples_ != expected_samples_) {
    ACE_ERROR((LM_ERROR, "ERROR: expected %d but received %d\n",
               expected_samples_, received_samples_));
  } else if (expected_samples_) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Expected number of samples received\n"));
  }
}

void
DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedDeadlineMissedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedIncompatibleQosStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleRejectedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::LivelinessChangedStatus& /*status*/)
{
}

void
DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  TestMsgDataReader_var reader_i =
    TestMsgDataReader::_narrow(reader);

  if (!reader_i) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  TestMsg message;
  DDS::SampleInfo info;

  DDS::ReturnCode_t error = reader_i->take_next_sample(message, info);

  if (error == DDS::RETCODE_OK) {
    if (info.valid_data) {
      if (++received_samples_ == expected_samples_) {
        done_callback_(builtin_read_error_);
      } else {
        ACE_DEBUG((LM_INFO, "(%P|%t) Got message %d\n", received_samples_));
      }
    }
  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" take_next_sample failed!\n")));
  }
}

void
DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SubscriptionMatchedStatus& /*status*/)
{
#ifndef DDS_HAS_MINIMUM_BIT
  if (check_bits_) {
    DDS::PublicationBuiltinTopicDataDataReader_var rdr =
      DDS::PublicationBuiltinTopicDataDataReader::_narrow(builtin_);
    DDS::PublicationBuiltinTopicDataSeq data;
    DDS::SampleInfoSeq infos;

    DDS::ReturnCode_t ret =
      rdr->read(data, infos, DDS::LENGTH_UNLIMITED,
                DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
    if (ret != DDS::RETCODE_OK && ret != DDS::RETCODE_NO_DATA) {
      ACE_DEBUG((LM_ERROR, "ERROR: %P could not read publication BIT: %d\n", ret));
      builtin_read_error_ = true;
      return;
    }

    ACE_DEBUG((LM_DEBUG, "(%P|%t) Successfully read publication BITs\n"));

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      for (CORBA::ULong i = 0; i < data.length(); ++i) {
        if (infos[i].valid_data) {

          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) Read Publication BIT with key: %x %x %x and handle %d\n"
                     "\tTopic: %C\tType: %C\n",
                     data[i].key.value[0], data[i].key.value[1],
                     data[i].key.value[2], infos[i].instance_handle,
                     data[i].topic_name.in(),
                     data[i].type_name.in()));

        }
      }
    }
  }
#endif /* DDS_HAS_MINIMUM_BIT */
}

void
DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleLostStatus& /*status*/)
{
}

#ifndef DDS_HAS_MINIMUM_BIT
void DataReaderListenerImpl::set_builtin_datareader (
  DDS::DataReader_ptr builtin)
{
  builtin_ = DDS::DataReader::_duplicate(builtin);
}
#endif /* DDS_HAS_MINIMUM_BIT */
