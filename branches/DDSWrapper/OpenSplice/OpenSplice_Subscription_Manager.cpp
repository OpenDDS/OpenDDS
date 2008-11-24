// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenSplice_Subscription_Manager.cpp
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include <ccpp_dds_dcps.h>
#include "OpenSplice_Subscription_Manager.h"
#include "Domain_Manager.h"
#include "Topic_Manager.h"
#include "Subscription_Manager.h"

#if !defined (__ACE_INLINE__)
#include "OpenSplice_Subscription_Manager.inl"
#endif

OpenSplice_Subscription_Manager::OpenSplice_Subscription_Manager (
    const Domain_Manager & dm,
    const DDS::SubscriberQos & qos)
  : dm_ (dm)
{
  // create the subscriber using default QoS.
  sub_ = 
    dm_.participant ()->create_subscriber (qos,
					   DDS::SubscriberListener::_nil (),
					   DDS::ANY_STATUS);
  
  // check for successful creation
  if (CORBA::is_nil (sub_.in ()))
    throw Manager_Exception ("OpenSplice_Subscription_Manager ctor failed to "
			     "create subscriber.");
}

OpenSplice_Subscription_Manager::OpenSplice_Subscription_Manager (
  const Domain_Manager & dm,
  DDS::Subscriber_ptr sub)
  : dm_ (dm), sub_ (sub)
{
}

OpenSplice_Subscription_Manager::~OpenSplice_Subscription_Manager ()
{
}

void
OpenSplice_Subscription_Manager::access_topic (
    const Topic_Manager & topic,
    const DDS::DataReaderQos & qos,
    const Subscription_Manager_Ptr & ref)
{
  // Create a modifiable copy of the Topic_Manager
  Topic_Manager tm (topic);

  // use topic manager to create the topic
  tm.create_topic (dm_);

  // the returned data reader is not used and therefore just stored
  // in a var class for following deletion
  DDS::DataReader_var dr =
    tm.datareader (Subscription_Manager (ref), qos);
}

DDS::DataReader_ptr 
OpenSplice_Subscription_Manager::lookup_datareader (const std::string & topic_name)
{
  return sub_->lookup_datareader (topic_name.c_str ());
}

void
OpenSplice_Subscription_Manager::remove_topic (const Topic_Manager & topic)
{
  // Create a modifiable copy of the Topic_Manager
  Topic_Manager tm (topic);

  // first find and remove associated data writer
  DDS::DataReader_var dr =
    sub_->lookup_datareader (tm.name ().c_str ());

  sub_->delete_datareader (dr.in ());

  // use topic manager to create the topic
  tm.delete_topic (dm_);
}

DDS::DataReaderQos
OpenSplice_Subscription_Manager::get_default_datareader_qos ()
{
  DDS::DataReaderQos qos;
  sub_->get_default_datareader_qos (qos);

  return qos;
}
