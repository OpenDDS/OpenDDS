// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Subscription_Manager.cpp
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DdsDcpsDomainC.h>
#include <ace/streams.h>
#include <dds/DCPS/SubscriberImpl.h>

#include "OpenDDS_Subscription_Manager.h"
#include "Domain_Manager.h"
#include "Topic_Manager.h"
#include "Subscription_Manager.h"

#if !defined (__ACE_INLINE__)
#include "OpenDDS_Subscription_Manager.inl"
#endif

OpenDDS_Subscription_Manager::OpenDDS_Subscription_Manager (
  const Domain_Manager & dm,
  const DDS::SubscriberQos & qos)
  : dm_ (dm)
{
  this->init (qos);
}

OpenDDS_Subscription_Manager::OpenDDS_Subscription_Manager (
  const Domain_Manager & dm,
  DDS::Subscriber_ptr sub)
  : dm_ (dm), sub_ (sub)
{
}

OpenDDS_Subscription_Manager::~OpenDDS_Subscription_Manager ()
{
}

void
OpenDDS_Subscription_Manager::init (const DDS::SubscriberQos & qos)
{
  // create the subscriber using default QoS.
  sub_ =
    dm_.participant ()->create_subscriber (qos,
                                           DDS::SubscriberListener::_nil (),
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // check for successful creation
  if (CORBA::is_nil (sub_.in ()))
    throw Manager_Exception ("Failed to create subscriber.");
}


void
OpenDDS_Subscription_Manager::access_topic (
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
OpenDDS_Subscription_Manager::lookup_datareader (const std::string & topic_name)
{
  return sub_->lookup_datareader (topic_name.c_str ());
}

void
OpenDDS_Subscription_Manager::remove_topic (const Topic_Manager & topic)
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
OpenDDS_Subscription_Manager::get_default_datareader_qos ()
{
  // create QoS object and initialize it with the default values
  DDS::DataReaderQos qos;
  sub_->get_default_datareader_qos (qos);

  // return the default qos
  return qos;
}
