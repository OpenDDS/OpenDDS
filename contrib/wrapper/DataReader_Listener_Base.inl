// -*- C++ -*-

//=============================================================================
/**
 *  @file    DataReader_Listener_Base.inl
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_DATAREADER_LISTENER_BASE_INL_
#define DDS_WRAPPER_DATAREADER_LISTENER_BASE_INL_

ACE_INLINE void
DataReader_Listener_Base::on_data_available(DDS::DataReader_ptr)
{
  // no op
}

ACE_INLINE void
DataReader_Listener_Base::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  // no op
}

ACE_INLINE void
DataReader_Listener_Base::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  // no op
}

ACE_INLINE void
DataReader_Listener_Base::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  // no op
}

ACE_INLINE void
DataReader_Listener_Base::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  // no op
}

ACE_INLINE void
DataReader_Listener_Base::on_sample_rejected (
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  // no op
}

ACE_INLINE void
DataReader_Listener_Base::on_sample_lost (
    DDS::DataReader_ptr,
    const DDS::SampleLostStatus&)
{
  // no op
}


#endif /* DDS_WRAPPER_DATAREADER_LISTENER_BASE_INL_ */
