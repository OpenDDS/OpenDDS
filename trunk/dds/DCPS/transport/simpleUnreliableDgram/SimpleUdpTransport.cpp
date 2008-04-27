// -*- C++ -*-
//
// $Id$

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUdpTransport.h"
#include "SimpleUdpConfiguration.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include <vector>
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "SimpleUdpTransport.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::SimpleUdpTransport::~SimpleUdpTransport()
{
  DBG_ENTRY_LVL("SimpleUdpTransport","~SimpleUdpTransport",6);
}


int
OpenDDS::DCPS::SimpleUdpTransport::configure_socket(TransportConfiguration* config)
{
  DBG_ENTRY_LVL("SimpleUdpTransport","configure_i",6);

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
 
  // As default, the acceptor will be listening on INADDR_ANY but advertise with the fully 
  // qualified hostname and actual listening port number.
  if (udp_config->local_address_.is_any())
    {
      unsigned short port = udp_config->local_address_.get_port_number ();
      std::stringstream out;
      out << port;

      const std::string& hostname = get_fully_qualified_hostname ();

      udp_config->local_address_.set (port, hostname.c_str());
      udp_config->local_address_str_ = hostname;
      udp_config->local_address_str_ += ":";
      udp_config->local_address_str_ += out.str ();
    }


  this->local_address_ = udp_config->local_address_;
  this->local_address_str_ = udp_config->local_address_str_;

  return 0;
}


int
OpenDDS::DCPS::SimpleUdpTransport::connection_info_i
                                   (TransportInterfaceInfo& local_info) const
{
  DBG_ENTRY_LVL("SimpleUdpTransport","connection_info_i",6);

  VDBG_LVL ((LM_DEBUG, "(%P|%t)SimpleUdpTransport::connection_info_i %s\n",
    this->local_address_str_.c_str ()), 2);

  NetworkAddress network_order_address(this->local_address_str_);

  ACE_OutputCDR cdr;
  cdr << network_order_address;
  size_t len = cdr.total_length ();

  // Allow DCPSInfo to check compatibility of transport implemenations.
  local_info.transport_id = 2; // TBD Change magic number into a enum or constant value.
  local_info.data = OpenDDS::DCPS::TransportInterfaceBLOB
    (len,
     len,
     (CORBA::Octet*)(cdr.buffer ()));

  return 0;
}



