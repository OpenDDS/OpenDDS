// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Subscription_Manager.inl
 *
 *  $Id$
 * 
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _OPEN_DDS_SUBSCRIPTION_MANAGER_INL_
#define _OPEN_DDS_SUBSCRIPTION_MANAGER_INL_

ACE_INLINE DDS::Subscriber_ptr 
OpenDDS_Subscription_Manager::subscriber () const
{
  return sub_.in ();
}

#endif /* _OPEN_DDS_SUBSCRIPTION_MANAGER_INL_ */
