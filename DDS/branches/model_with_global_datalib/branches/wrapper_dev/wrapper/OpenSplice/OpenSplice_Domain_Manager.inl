// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenSplice_Domain_Manager.inl
 *
 *  $Id$
 * 
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _OPEN_SPLICE_DOMAIN_MANAGER_INL_
#define _OPEN_SPLICE_DOMAIN_MANAGER_INL_

ACE_INLINE DDS::DomainParticipant_ptr
OpenSplice_Domain_Manager::participant ()
{
  return dp_.in ();
}

#endif /* _OPEN_SPLICE_DOMAIN_MANAGER_INL_ */
