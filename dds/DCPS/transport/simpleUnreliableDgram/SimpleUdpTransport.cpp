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


  ACE_INET_Addr address = udp_config->local_address_;

  // Open our socket using address which will be reset the port if the port is 0.
  if (this->socket_->open_socket(address) != 0)
    {

      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: failed to open udp socket %C:%d: %p\n",
                        address.get_host_addr (),
                        address.get_port_number (),
                        ACE_TEXT("open")),
                       -1);
    }
 
  unsigned short port = address.get_port_number ();
  std::stringstream out;
  out << port;

  // As default, the acceptor will be listening on INADDR_ANY but advertise with the fully
  // qualified hostname and actual listening port number.
  if (udp_config->local_address_.is_any ())
    {
      ACE_TString hostname = get_fully_qualified_hostname ();

      udp_config->local_address_.set (port, hostname.c_str());
      udp_config->local_address_str_ = hostname;
      udp_config->local_address_str_ += ACE_TEXT(":");
      udp_config->local_address_str_ += ACE_TEXT_CHAR_TO_TCHAR(out.str().c_str());
    }

  // Now we got the actual listening port. Update the port nnmber in the configuration
  // if it's 0 originally.
  else if (udp_config->local_address_.get_port_number () == 0)
    {
      udp_config->local_address_.set_port_number (port);

      if (! udp_config->local_address_str_.empty ())
      {
        ACE_TString::size_type pos = udp_config->local_address_str_.find (ACE_TEXT(':'));
        ACE_TString str = udp_config->local_address_str_.substr (0, pos + 1);
        str += ACE_TEXT_CHAR_TO_TCHAR(out.str().c_str());
        udp_config->local_address_str_ = str;
      }
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



