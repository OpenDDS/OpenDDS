// -*- C++ -*-

//=============================================================================
/**
 *  @file    Subscription_Manager.inl
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_SUBSCRIPTION_MANAGER_INL_
#define DDS_WRAPPER_SUBSCRIPTION_MANAGER_INL_

ACE_INLINE bool
Subscription_Manager::null () const
{
  return manager_impl_.null ();
}

ACE_INLINE void
Subscription_Manager::access_topic (const Topic_Manager & topic,
                                    const DDS::DataReaderQos & qos)
{
  manager_impl_->access_topic (topic, qos, manager_impl_);
}

ACE_INLINE DDS::DataReader_ptr
Subscription_Manager::lookup_datareader (const std::string & topic_name)
{
  return manager_impl_->lookup_datareader (topic_name);
}

ACE_INLINE void
Subscription_Manager::remove_topic (const Topic_Manager & topic)
{
  return manager_impl_->remove_topic (topic);
}

ACE_INLINE DDS::DataReaderQos
Subscription_Manager::get_default_datareader_qos ()
{
  return manager_impl_->get_default_datareader_qos ();
}

ACE_INLINE DDS::Subscriber_ptr
Subscription_Manager::subscriber () const
{
  return manager_impl_->subscriber ();
}

#endif /* DDS_WRAPPER_SUBSCRIPTION_MANAGER_INL_ */
