// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Subscription_Manager.inl
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_OPEN_DDS_SUBSCRIPTION_MANAGER_INL_
#define DDS_WRAPPER_OPEN_DDS_SUBSCRIPTION_MANAGER_INL_

ACE_INLINE DDS::Subscriber_ptr
OpenDDS_Subscription_Manager::subscriber () const
{
  return sub_.in ();
}

#endif /* DDS_WRAPPER_OPEN_DDS_SUBSCRIPTION_MANAGER_INL_ */
