// -*- C++ -*-

//=============================================================================
/**
 *  @file    Subscription_Manager.cpp
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "Subscription_Manager.h"

#if !defined (__ACE_INLINE__)
#include "Subscription_Manager.inl"
#endif

Subscription_Manager::Subscription_Manager ()
  : manager_impl_ (0)
{
}

Subscription_Manager::Subscription_Manager (Subscription_Manager_Ptr impl)
  : manager_impl_ (impl)
{
}

Subscription_Manager::Subscription_Manager (const Subscription_Manager & copy)
  : manager_impl_ (copy.manager_impl_)
{
}

void
Subscription_Manager::operator= (const Subscription_Manager & copy)
{
  // check for self assignment
  if (this != &copy)
    {
      manager_impl_ = copy.manager_impl_;
    }
}
