// -*- C++ -*-

//=============================================================================
/**
 *  @file    Publication_Manager.inl
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_PUBLICATION_MANAGER_INL_
#define DDS_WRAPPER_PUBLICATION_MANAGER_INL_

ACE_INLINE bool
Publication_Manager::null () const
{
  return manager_impl_.null ();
}

ACE_INLINE DDS::DataWriter_ptr
Publication_Manager::access_topic (const Topic_Manager & topic,
                                   const DDS::DataWriterQos & qos)
{
  return manager_impl_->access_topic (topic,
                                      qos,
                                      manager_impl_);
}

ACE_INLINE void
Publication_Manager::remove_topic (const Topic_Manager & topic)
{
  return manager_impl_->remove_topic (topic);
}

ACE_INLINE DDS::DataWriterQos
Publication_Manager::get_default_datawriter_qos ()
{
  return manager_impl_->get_default_datawriter_qos ();
}

ACE_INLINE DDS::Publisher_ptr
Publication_Manager::publisher () const
{
  return manager_impl_->publisher ();
}

#endif /* DDS_WRAPPER_PUBLICATION_MANAGER_INL_ */
