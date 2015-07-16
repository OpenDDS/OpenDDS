// -*- C++ -*-

//=============================================================================
/**
 *  @file    Publication_Manager.cpp
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "Publication_Manager.h"

#if !defined (__ACE_INLINE__)
#include "Publication_Manager.inl"
#endif

Publication_Manager::Publication_Manager ()
  : manager_impl_ (0)
{
}

Publication_Manager::Publication_Manager (const Publication_Manager_Ptr & impl)
  : manager_impl_ (impl)
{
}

Publication_Manager::Publication_Manager (const Publication_Manager & copy)
  : manager_impl_ (copy.manager_impl_)
{
}

void
Publication_Manager::operator= (const Publication_Manager & copy)
{
  // check for self assignment
  if (this != &copy)
    {
      manager_impl_ = copy.manager_impl_;
    }
}
