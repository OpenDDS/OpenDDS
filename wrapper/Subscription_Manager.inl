// -*- C++ -*-

//=============================================================================
/**
 *  @file    Subscription_Manager.inl
 *
 *  $Id$
 * 
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _SUBSCRIPTION_MANAGER_INL_
#define _SUBSCRIPTION_MANAGER_INL_

ACE_INLINE void
Subscription_Manager::access_topic (const Topic_Manager & topic)
{
  manager_impl_->access_topic (topic, manager_impl_);
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

ACE_INLINE DDS::Subscriber_ptr
Subscription_Manager::subscriber () const
{
  return manager_impl_->subscriber ();
}

#endif /* _SUBSCRIPTION_MANAGER_INL_ */
