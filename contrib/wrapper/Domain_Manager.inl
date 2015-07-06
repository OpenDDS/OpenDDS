// -*- C++ -*-

//=============================================================================
/**
 *  @file    Domain_Manager.inl
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_DOMAIN_MANAGER_INL_
#define DDS_WRAPPER_DOMAIN_MANAGER_INL_

ACE_INLINE std::string
Manager_Exception::reason () const
{
  return reason_;
}

ACE_INLINE void
Domain_Manager::run ()
{
  manager_impl_->run ();
}

ACE_INLINE bool
Domain_Manager::null () const
{
  return manager_impl_.null ();
}

ACE_INLINE void
Domain_Manager::shutdown ()
{
  manager_impl_->shutdown ();
}

ACE_INLINE Subscription_Manager
Domain_Manager::subscription_manager (const DDS::SubscriberQos & qos)
{
  return manager_impl_->subscription_manager (manager_impl_, qos);
}

ACE_INLINE Subscription_Manager
Domain_Manager::builtin_topic_subscriber ()
{
  return manager_impl_->builtin_topic_subscriber (manager_impl_);
}

ACE_INLINE Publication_Manager
Domain_Manager::publication_manager (const DDS::PublisherQos & qos)
{
  return manager_impl_->publication_manager (manager_impl_, qos);
}

ACE_INLINE DDS::DomainParticipant_ptr
Domain_Manager::participant ()
{
  return manager_impl_->participant ();
}

#endif /* DDS_WRAPPER_DOMAIN_MANAGER_INL_ */
