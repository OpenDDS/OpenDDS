// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenSplice_Publication_Manager.inl
 *
 *  $Id$
 * 
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef _OPEN_SPLICE_PUBLICATION_MANAGER_INL_
#define _OPEN_SPLICE_PUBLICATION_MANAGER_INL_

ACE_INLINE DDS::Publisher_ptr
OpenSplice_Publication_Manager::publisher () const
{
  return pub_.in ();
}

#endif /* _OPEN_SPLICE_PUBLICATION_MANAGER_INL_ */
