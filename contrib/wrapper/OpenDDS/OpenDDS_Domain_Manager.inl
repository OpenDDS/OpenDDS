// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Domain_Manager.inl
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_OPEN_DDS_DOMAIN_MANAGER_INL_
#define DDS_WRAPPER_OPEN_DDS_DOMAIN_MANAGER_INL_

ACE_INLINE DDS::DomainParticipant_ptr
OpenDDS_Domain_Manager::participant ()
{
  return dp_.in ();
}

#endif /* DDS_WRAPPER_OPEN_DDS_DOMAIN_MANAGER_INL_ */
