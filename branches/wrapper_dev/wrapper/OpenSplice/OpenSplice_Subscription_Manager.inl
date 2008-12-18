// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenSplice_Subscription_Manager.inl
 *
 *  $Id$
 * 
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _OPEN_SPLICE_SUBSCRIPTION_MANAGER_INL_
#define _OPEN_SPLICE_SUBSCRIPTION_MANAGER_INL_

ACE_INLINE DDS::Subscriber_ptr 
OpenSplice_Subscription_Manager::subscriber () const
{
  return sub_.in ();
}

#endif /* _OPEN_SPLICE_SUBSCRIPTION_MANAGER_INL_ */
