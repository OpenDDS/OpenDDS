// -*- C++ -*-

//=============================================================================
/**
 *  @file    Topic_Manager.inl
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_TOPIC_MANAGER_INL_
#define DDS_WRAPPER_TOPIC_MANAGER_INL_

ACE_INLINE bool
Topic_Manager::null () const
{
  return manager_impl_.null ();
}

ACE_INLINE std::string
Topic_Manager::name () const
{
  return manager_impl_->name ();
}

ACE_INLINE void
Topic_Manager::create_topic (Domain_Manager & dm)
{
  manager_impl_->create_topic (dm);
}

ACE_INLINE void
Topic_Manager::delete_topic (Domain_Manager & dm)
{
  manager_impl_->delete_topic (dm);
}

ACE_INLINE DDS::DataReader_ptr
Topic_Manager::datareader (const Subscription_Manager & sm,
                           const DDS::DataReaderQos & qos)
{
  return manager_impl_->datareader (sm, qos);
}

ACE_INLINE DDS::DataWriter_ptr
Topic_Manager::datawriter (const Publication_Manager & pm,
                           const DDS::DataWriterQos & qos)
{
  return manager_impl_->datawriter (pm, qos);
}

#endif /* DDS_WRAPPER_TOPIC_MANAGER_INL_ */
