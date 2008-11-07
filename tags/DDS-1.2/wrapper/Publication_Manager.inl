// -*- C++ -*-

//=============================================================================
/**
 *  @file    Publication_Manager.inl
 *
 *  $Id$
 * 
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _PUBLICATION_MANAGER_INL_
#define _PUBLICATION_MANAGER_INL_

ACE_INLINE DDS::DataWriter_ptr
Publication_Manager::access_topic (const Topic_Manager & topic)
{
  return manager_impl_->access_topic (topic,
				      manager_impl_);
}

ACE_INLINE void 
Publication_Manager::remove_topic (const Topic_Manager & topic)
{
  return manager_impl_->remove_topic (topic);
}

ACE_INLINE DDS::Publisher_ptr
Publication_Manager::publisher () const
{
  return manager_impl_->publisher ();
}

#endif /* _PUBLICATION_MANAGER_INL_ */
