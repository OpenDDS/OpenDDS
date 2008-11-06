// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Publication_Manager.inl
 *
 *  $Id$
 * 
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _OPEN_DDS_PUBLICATION_MANAGER_INL_
#define _OPEN_DDS_PUBLICATION_MANAGER_INL_

ACE_INLINE DDS::Publisher_ptr
OpenDDS_Publication_Manager::publisher () const
{
  return pub_.in ();
}

#endif /* _OPEN_DDS_PUBLICATION_MANAGER_INL_ */
