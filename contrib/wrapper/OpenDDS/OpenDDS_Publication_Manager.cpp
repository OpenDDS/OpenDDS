// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Publication_Manager.cpp
 *
 *  $Id$
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include "OpenDDS_Publication_Manager.h"
#include "Domain_Manager.h"
#include "Topic_Manager.h"
#include "Publication_Manager.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DdsDcpsDomainC.h>
#include <ace/streams.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/tcp/TcpConfiguration.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#endif

#if !defined (__ACE_INLINE__)
#include "OpenDDS_Publication_Manager.inl"
#endif

OpenDDS_Publication_Manager::OpenDDS_Publication_Manager (
  const Domain_Manager & dm,
  OpenDDS::DCPS::TransportIdType transport_impl_id,
  const DDS::PublisherQos & qos)
  : dm_ (dm)
{
  this->init (qos);

  this->register_transport (transport_impl_id);
}

OpenDDS_Publication_Manager::~OpenDDS_Publication_Manager ()
{
}

void
OpenDDS_Publication_Manager::init (const DDS::PublisherQos & qos)
{
  // create the subscriber using default QoS.
  pub_ =
    dm_.participant ()->create_publisher (qos,
                                          DDS::PublisherListener::_nil (),
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // check for successful creation
  if (CORBA::is_nil (pub_.in ()))
    throw Manager_Exception ("Failed to create publisher.");
}

void
OpenDDS_Publication_Manager::register_transport (OpenDDS::DCPS::TransportIdType transport_id)
{
  // Initialize the transport
  OpenDDS::DCPS::TransportImpl_rch transport_impl =
    OpenDDS::DCPS::TransportFactory::instance ()->obtain (
      transport_id);

  // Attach the publisher to the transport
  OpenDDS::DCPS::PublisherImpl* pub_impl =
    dynamic_cast<OpenDDS::DCPS::PublisherImpl*> (pub_.in ());

  if (0 == pub_impl)
    throw Manager_Exception ("Failed to obtain publisher servant");

  OpenDDS::DCPS::AttachStatus status =
    pub_impl->attach_transport(transport_impl.in ());

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
                             "Failed to attach publisher to the transport. Status == ");
      error_msg += status_str.c_str();

      throw Manager_Exception (error_msg);
    }
}

DDS::DataWriter_ptr
OpenDDS_Publication_Manager::access_topic (const Topic_Manager & topic,
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
OpenDDS_Publication_Manager::remove_topic (const Topic_Manager & topic)
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
OpenDDS_Publication_Manager::get_default_datawriter_qos ()
{
  // create QoS object and initialize it with the default values
  DDS::DataWriterQos qos;
  pub_->get_default_datawriter_qos (qos);

  // return the default qos
  return qos;
}
