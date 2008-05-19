// -*- C++ -*-
//
// $Id$
#include "UpdateListener_T.h"
#include "FederatorManagerImpl.h"

namespace OpenDDS { namespace Federator {

template< class DataType, class ReaderType>
UpdateListener< DataType, ReaderType>::UpdateListener( ManagerImpl& manager)
 : manager_( manager)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) UpdateListener::UpdateListener\n")
  ));
}

template< class DataType, class ReaderType>
UpdateListener< DataType, ReaderType>::~UpdateListener(void)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) UpdateListener::~UpdateListener\n")
  ));
}

template< class DataType, class ReaderType>
void
UpdateListener< DataType, ReaderType>::on_data_available(
  ::DDS::DataReader_ptr reader
)
ACE_THROW_SPEC((
  CORBA::SystemException
))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_data_available\n")));
  }

  try
  {
    // Get the type specific reader.
    typename ReaderType::_var_type dataReader = ReaderType::_narrow( reader);
    if( CORBA::is_nil( dataReader.in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) UpdateListener::on_data_available - _narrow failed.\n")));
      return;
    }

    DataType            sample;
    ::DDS::SampleInfo   info;
    ::DDS::ReturnCode_t status = dataReader->read_next_sample( sample, info);

    if( status == ::DDS::RETCODE_OK) {
      // Delegate processing to the federation manager.
      this->manager_.update( sample, info);

    } else {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: UpdateListener::on_data_available: read status==%d\n"),
        status
      ));
    }

  } catch( const CORBA::Exception& ex) {
    ex._tao_print_exception("(%P|%t) UpdateListener::read - ");
  }
}

template< class DataType, class ReaderType>
void
UpdateListener< DataType, ReaderType>::on_requested_deadline_missed(
    ::DDS::DataReader_ptr /* reader */,
    const ::DDS::RequestedDeadlineMissedStatus & /* status */)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ")
               ACE_TEXT("Federatorer::on_requested_deadline_missed\n")));
  }
}

template< class DataType, class ReaderType>
void
UpdateListener< DataType, ReaderType>::on_requested_incompatible_qos(
  ::DDS::DataReader_ptr /* reader */,
  const ::DDS::RequestedIncompatibleQosStatus & /* status */)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::")
               ACE_TEXT("on_requested_incompatible_qos\n")));
  }
}

template< class DataType, class ReaderType>
void
UpdateListener< DataType, ReaderType>::on_liveliness_changed(
    ::DDS::DataReader_ptr /* reader */,
    const ::DDS::LivelinessChangedStatus & /* status */)
  ACE_THROW_SPEC(( CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_liveliness_changed\n")));
  }
}

template< class DataType, class ReaderType>
void
UpdateListener< DataType, ReaderType>::on_subscription_match(
    ::DDS::DataReader_ptr /* reader */,
    const ::DDS::SubscriptionMatchStatus & /* status */)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_subscription_match\n")));
  }
}

template< class DataType, class ReaderType>
void
UpdateListener< DataType, ReaderType>::on_sample_rejected(
    ::DDS::DataReader_ptr /* reader */,
   const DDS::SampleRejectedStatus& /* status */)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_sample_rejected\n")));
  }
}

template< class DataType, class ReaderType>
void
UpdateListener< DataType, ReaderType>::on_sample_lost(
  ::DDS::DataReader_ptr /* reader */,
  const DDS::SampleLostStatus& /* status */)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateListener::on_sample_lost\n")));
  }
}

}} // End of namespace OpenDDS::Federator

