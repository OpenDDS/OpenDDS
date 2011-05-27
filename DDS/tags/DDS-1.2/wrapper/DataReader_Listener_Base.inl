// -*- C++ -*-

//=============================================================================
/**
 *  @file    DataReader_Listener_Base.inl
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _DATAREADER_LISTENER_BASE_INL_
#define _DATAREADER_LISTENER_BASE_INL_

ACE_INLINE void 
DataReader_Listener_Base::on_data_available(DDS::DataReader_ptr)
  throw (CORBA::SystemException)
{
  // no op
}

ACE_INLINE void 
DataReader_Listener_Base::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
  throw (CORBA::SystemException)
{
  // no op
}

ACE_INLINE void
DataReader_Listener_Base::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
  // no op
}

ACE_INLINE void 
DataReader_Listener_Base::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
  // no op
}

ACE_INLINE void 
DataReader_Listener_Base::on_subscription_match (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus &)
  throw (CORBA::SystemException)
{
  // no op
}

ACE_INLINE void 
DataReader_Listener_Base::on_sample_rejected (
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{
  // no op
}

ACE_INLINE void 
DataReader_Listener_Base::on_sample_lost (
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{
  // no op
}


#endif /* DATA_READER_LISTENER_IMPL_INL_ */
