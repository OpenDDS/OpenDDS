// -*- C++ -*-
//
// $Id$

#include  "SimpleUnreliableDgram_pch.h"
#include  "SimpleUdpTransport.h"
#include  "SimpleUdpConfiguration.h"
#include  "dds/DCPS/transport/framework/NetworkAddress.h"
#include  <vector>


#if !defined (__ACE_INLINE__)
#include "SimpleUdpTransport.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::SimpleUdpTransport::~SimpleUdpTransport()
{
  DBG_ENTRY_LVL("SimpleUdpTransport","~SimpleUdpTransport",5);
}


int
TAO::DCPS::SimpleUdpTransport::configure_socket(TransportConfiguration* config)
{
  DBG_ENTRY_LVL("SimpleUdpTransport","configure_i",5);

  // Downcast the config argument to a SimpleUdpConfiguration*
  SimpleUdpConfiguration* udp_config = ACE_static_cast(SimpleUdpConfiguration*,
                                                       config);

  if (udp_config == 0)
    {
      // The downcast failed.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed downcast from TransportConfiguration "
                        "to SimpleUdpConfiguration.\n"),
                       -1);
    }

  if (udp_config->local_address_.get_ip_address () == INADDR_ANY)
    {
      ACE_INET_Addr new_addr;
      int result = new_addr.set (
				 udp_config->local_address_.get_port_number (),
				 udp_config->local_address_.get_host_name ());

      if (result != 0)
        ACE_ERROR_RETURN((LM_ERROR,
			  "(%P|%t) ERROR: SimpleUdpTransport::configure_socket"
			  " could not get host name!!\n"),
			 -1);

      udp_config->local_address_ = new_addr;
    }

  // Open our socket using the local_address_ from our udp_config_ object.
  if (this->socket_->open_socket(udp_config->local_address_) != 0)
    {

      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: failed to open udp socket %s:%d: %p\n",
                        this->local_address_.get_host_addr (),
                        this->local_address_.get_port_number (),
                        "open"),
                       -1);
    }

  this->local_address_ = udp_config->local_address_;

  return 0;
}


int
TAO::DCPS::SimpleUdpTransport::connection_info_i
                                   (TransportInterfaceInfo& local_info) const
{
  DBG_ENTRY_LVL("SimpleUdpTransport","connection_info_i",5);

  ACE_DEBUG ((LM_DEBUG, "(%P|%t)SimpleUdpTransport::connection_info_i %s:%d\n", 
    this->local_address_.get_host_addr (), 
    this->local_address_.get_port_number ()));

  NetworkAddress network_order_address(this->local_address_);

  // Allow DCPSInfo to check compatibility of transport implemenations.
  local_info.transport_id = 2; // TBD Change magic number into a enum or constant value.
  // TBD SOON - Move the local_info.data "population" to the NetworkAddress.
  local_info.data = TAO::DCPS::TransportInterfaceBLOB
                                    (sizeof(NetworkAddress),
                                     sizeof(NetworkAddress),
                                     (CORBA::Octet*)(&network_order_address));

  return 0;
}



