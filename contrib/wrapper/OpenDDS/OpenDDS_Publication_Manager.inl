// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Publication_Manager.inl
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_OPEN_DDS_PUBLICATION_MANAGER_INL_
#define DDS_WRAPPER_OPEN_DDS_PUBLICATION_MANAGER_INL_

ACE_INLINE DDS::Publisher_ptr
OpenDDS_Publication_Manager::publisher () const
{
  return pub_.in ();
}

#endif /* DDS_WRAPPER_OPEN_DDS_PUBLICATION_MANAGER_INL_ */
