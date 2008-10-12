// -*- C++ -*-

//=============================================================================
/**
 *  @file    Topic_Manager.inl
 *
 *  $Id$
 * 
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _TOPIC_MANAGER_INL_
#define _TOPIC_MANAGER_INL_

ACE_INLINE std::string
Topic_Manager::name () const
{
  return manager_impl_->name ();
}

ACE_INLINE void
Topic_Manager::create_topic (Domain_Manager & dm)
{
  manager_impl_->create_topic (dm);
}

ACE_INLINE void
Topic_Manager::delete_topic (Domain_Manager & dm)
{
  manager_impl_->delete_topic (dm);
}

ACE_INLINE DDS::DataReader_ptr
Topic_Manager::datareader (const Subscription_Manager & sm)
{
  return manager_impl_->datareader (sm);
}

ACE_INLINE DDS::DataWriter_ptr 
Topic_Manager::datawriter (const Publication_Manager & pm)
{
  return manager_impl_->datawriter (pm);
}

#endif /* _TOPIC_MANAGER_INL_ */
