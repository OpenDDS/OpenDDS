// -*- C++ -*-

//=============================================================================
/**
 *  @file    Topic_Manager.cpp
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "Topic_Manager.h"

#if !defined (__ACE_INLINE__)
#include "Topic_Manager.inl"
#endif

Topic_Manager::Topic_Manager ()
  : manager_impl_ (0)
{
}

Topic_Manager::Topic_Manager (Topic_Manager_Impl * impl)
  : manager_impl_ (Topic_Manager_Ptr (impl))
{
}

Topic_Manager::Topic_Manager (Topic_Manager_Ptr impl)
  : manager_impl_ (impl)
{
}

Topic_Manager::Topic_Manager (const Topic_Manager & copy)
  : manager_impl_ (copy.manager_impl_)
{
}

void
Topic_Manager::operator= (const Topic_Manager& copy)
{
  // check for self assignment
  if (this != &copy)
    {
      manager_impl_ = copy.manager_impl_;
    }
}

Topic_Manager::~Topic_Manager ()
{
}
