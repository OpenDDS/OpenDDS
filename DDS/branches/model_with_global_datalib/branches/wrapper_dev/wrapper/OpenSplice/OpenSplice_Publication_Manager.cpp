// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenSplice_Publication_Manager.cpp
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include <ccpp_dds_dcps.h>
#include "OpenSplice_Publication_Manager.h"
#include "Domain_Manager.h"
#include "Topic_Manager.h"
#include "Publication_Manager.h"

#if !defined (__ACE_INLINE__)
#include "OpenSplice_Publication_Manager.inl"
#endif

OpenSplice_Publication_Manager::OpenSplice_Publication_Manager (
    const Domain_Manager & dm,
    const DDS::PublisherQos & qos)
  : dm_ (dm)
{
  // create the subscriber using default QoS.
  pub_ = 
    dm_.participant ()->create_publisher (qos,
                                          DDS::PublisherListener::_nil (),
					  DDS::ANY_STATUS);

  // check for successful creation
  if (CORBA::is_nil (pub_.in ()))
    throw Manager_Exception ("OpenSplice_Publication_Manager ctor failed to "
			     "create publisher.");

}

OpenSplice_Publication_Manager::~OpenSplice_Publication_Manager ()
{
}

DDS::DataWriter_ptr
OpenSplice_Publication_Manager::access_topic (
    const Topic_Manager & topic,
    const DDS::DataWriterQos & qos,
    const Publication_Manager_Ptr & ref)
{
  // Create a modifiable copy of the Topic_Manager
  Topic_Manager tm (topic);

  tm.create_topic (dm_);

  // return a new datawriter created by the topic manager
  return tm.datawriter (Publication_Manager (ref), qos);
}

void
OpenSplice_Publication_Manager::remove_topic (const Topic_Manager & topic)
{
  // Create a modifiable copy of the Topic_Manager
  Topic_Manager tm (topic);

  // first find and remove associated data writer
  DDS::DataWriter_var dw =
    pub_->lookup_datawriter (tm.name ().c_str ());

  pub_->delete_datawriter (dw.in ());

  // use topic manager to create the topic
  tm.delete_topic (dm_);
}

DDS::DataWriterQos
OpenSplice_Publication_Manager::get_default_datawriter_qos ()
{
  DDS::DataWriterQos qos;
  pub_->get_default_datawriter_qos (qos);

  return qos;
}
