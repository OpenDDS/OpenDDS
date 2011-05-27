// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Subscription_Manager.cpp
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DdsDcpsDomainC.h>
#include <ace/streams.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#include <dds/DCPS/transport/simpleUnreliableDgram/SimpleUnreliableDgram.h>
#include <dds/DCPS/transport/ReliableMulticast/ReliableMulticast.h>
#endif

#include "OpenDDS_Subscription_Manager.h"
#include "Domain_Manager.h"
#include "Topic_Manager.h"
#include "Subscription_Manager.h"

#if !defined (__ACE_INLINE__)
#include "OpenDDS_Subscription_Manager.inl"
#endif

OpenDDS_Subscription_Manager::OpenDDS_Subscription_Manager (
  const Domain_Manager & dm,
  OpenDDS::DCPS::TransportIdType transport_impl_id,
  const DDS::SubscriberQos & qos)
  : dm_ (dm)
{
  this->init (qos);

  this->register_transport (transport_impl_id);
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
					   DDS::SubscriberListener::_nil ());
  
  // check for successful creation
  if (CORBA::is_nil (sub_.in ()))
    throw Manager_Exception ("Failed to create subscriber.");
}

void 
OpenDDS_Subscription_Manager::register_transport (
  OpenDDS::DCPS::TransportIdType transport_id)
{
  // Initialize the transport
  OpenDDS::DCPS::TransportImpl_rch transport_impl =
    OpenDDS::DCPS::TransportFactory::instance ()->obtain (
      transport_id);

  // Attach the subscriber to the transport.
  OpenDDS::DCPS::SubscriberImpl* sub_impl =
    dynamic_cast<OpenDDS::DCPS::SubscriberImpl*> (sub_.in ());

  if (0 == sub_impl) {
    throw Manager_Exception ("Failed to obtain subscriber servant");
  }

  OpenDDS::DCPS::AttachStatus status = 
    sub_impl->attach_transport(transport_impl.in());

  if (status != OpenDDS::DCPS::ATTACH_OK) 
    {
      std::string status_str;
      switch (status) {
      case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
	status_str = "ATTACH_BAD_TRANSPORT";
	break;
      case OpenDDS::DCPS::ATTACH_ERROR:
	status_str = "ATTACH_ERROR";
	break;
      case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
	status_str = "ATTACH_INCOMPATIBLE_QOS";
	break;
      default:
	status_str = "Unknown Status";
	break;
      }

      std::string error_msg (
        "Failed to attach subscriber to the transport. Status == ");
      error_msg += status_str.c_str();

      throw Manager_Exception (error_msg);
    }
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
