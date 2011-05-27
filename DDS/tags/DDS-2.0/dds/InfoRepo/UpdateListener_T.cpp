/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UpdateListener_T.h"
#include "dds/DCPS/debug.h"

namespace OpenDDS {
namespace Federator {

template<class DataType, class ReaderType>
UpdateListener<DataType, ReaderType>::UpdateListener(UpdateProcessor<DataType>& processor)
  : federationId_(NIL_REPOSITORY), receiver_(processor)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::UpdateListener\n")));
  }
}

template<class DataType, class ReaderType>
UpdateListener<DataType, ReaderType>::~UpdateListener()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::~UpdateListener\n")));
  }
}

template<class DataType, class ReaderType>
RepoKey&
UpdateListener<DataType, ReaderType>::federationId()
{
  return this->federationId_;
}

template<class DataType, class ReaderType>
RepoKey
UpdateListener<DataType, ReaderType>::federationId() const
{
  return this->federationId_;
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::on_data_available(
  DDS::DataReader_ptr reader)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_data_available\n")));
  }

  try {
    // Get the type specific reader.
    typename ReaderType::_var_type dataReader = ReaderType::_narrow(reader);

    if (CORBA::is_nil(dataReader.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) UpdateListener::on_data_available - _narrow failed.\n")));
      return;
    }

    // Process all available data.
    while (true) {
      DataType*           sample = new DataType();
      DDS::SampleInfo*  info   = new DDS::SampleInfo();
      DDS::ReturnCode_t status = dataReader->read_next_sample(*sample, *info);

      if (status == DDS::RETCODE_OK) {
        // Check if we should process the sample.
        if ((this->federationId() != NIL_REPOSITORY)
            && (this->federationId() != sample->sender)) {

          // Delegate processing to the federation manager.
          this->receiver_.add(sample, info);
        }

      } else if (status == DDS::RETCODE_NO_DATA) {
        break;

      } else {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: UpdateListener::on_data_available: read status==%d\n"),
                   status));
        break;
      }
    }

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("(%P|%t) UpdateListener::read - ");
  }
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::on_requested_deadline_missed(
  DDS::DataReader_ptr /* reader */,
  const DDS::RequestedDeadlineMissedStatus & /* status */)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ")
               ACE_TEXT("Federatorer::on_requested_deadline_missed\n")));
  }
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::on_requested_incompatible_qos(
  DDS::DataReader_ptr /* reader */,
  const DDS::RequestedIncompatibleQosStatus & /* status */)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::")
               ACE_TEXT("on_requested_incompatible_qos\n")));
  }
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::on_liveliness_changed(
  DDS::DataReader_ptr /* reader */,
  const DDS::LivelinessChangedStatus & /* status */)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_liveliness_changed\n")));
  }
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::on_subscription_matched(
  DDS::DataReader_ptr /* reader */,
  const DDS::SubscriptionMatchedStatus & /* status */)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_subscription_matched\n")));
  }
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::on_sample_rejected(
  DDS::DataReader_ptr /* reader */,
  const DDS::SampleRejectedStatus& /* status */)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_sample_rejected\n")));
  }
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::on_sample_lost(
  DDS::DataReader_ptr /* reader */,
  const DDS::SampleLostStatus& /* status */)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_sample_lost\n")));
  }
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::stop()
{
  this->receiver_.stop();
}

template<class DataType, class ReaderType>
void
UpdateListener<DataType, ReaderType>::join()
{
  this->receiver_.wait();
}

} // namespace Federator
} // namespace OpenDDS
