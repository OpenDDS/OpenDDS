// -*- C++ -*-
//
// $Id$

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleMcastTransport.h"
#include "SimpleMcastConfiguration.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"


#if !defined (__ACE_INLINE__)
#include "SimpleMcastTransport.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::SimpleMcastTransport::~SimpleMcastTransport()
{
  DBG_ENTRY_LVL("SimpleMcastTransport","~SimpleMcastTransport",5);
}



int
TAO::DCPS::SimpleMcastTransport::configure_socket(TransportConfiguration* config)
{
  DBG_ENTRY_LVL("SimpleMcastTransport","configure_socket",5);

  // Downcast the config argument to a SimpleMcastConfiguration*
  SimpleMcastConfiguration* mcast_config = ACE_static_cast(SimpleMcastConfiguration*,
                                                       config);

  if (mcast_config == 0)
    {
      // The downcast failed.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed downcast from TransportConfiguration "
                        "to SimpleMcastConfiguration.\n"),
                       -1);
    }

  if (mcast_config->local_address_.get_ip_address () == INADDR_ANY)
    {
      ACE_INET_Addr new_addr;
      int result = new_addr.set (
        mcast_config->local_address_.get_port_number (),
        mcast_config->local_address_.get_host_name ());

      if (result != 0)
        ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) ERROR: SimpleMcastTransport::configure_socket"
        " could not get host name!!\n"),
        -1);

      mcast_config->local_address_ = new_addr;
    }

  // Open our socket using the parameters from our mcast_config_ object.
  if (this->socket_->open_socket (mcast_config->local_address_,
                          mcast_config->multicast_group_address_,
                          mcast_config->receiver_) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: failed to open multicast socket %s:%d: %p\n",
                        mcast_config->multicast_group_address_.get_host_addr (),
                        mcast_config->multicast_group_address_.get_port_number (),
                        "open"),
                       -1);
    }

  this->local_address_ = mcast_config->local_address_;
  this->multicast_group_address_ = mcast_config->multicast_group_address_;
  this->receiver_ = mcast_config->receiver_;

  return 0;
}


int
TAO::DCPS::SimpleMcastTransport::connection_info_i
                                   (TransportInterfaceInfo& local_info) const
{
  DBG_ENTRY_LVL("SimpleMcastTransport","connection_info_i",5);

  ACE_DEBUG ((LM_DEBUG, "(%P|%t)SimpleMcastTransport::connection_info_i %s:%d\n", 
    this->multicast_group_address_.get_host_addr (), 
    this->multicast_group_address_.get_port_number ()));

  NetworkAddress network_order_address(this->multicast_group_address_);

  // Allow DCPSInfo to check compatibility of transport implemenations.
  local_info.transport_id = 3; // TBD Change magic number into a enum or constant value.
  // TBD SOON - Move the local_info.data "population" to the NetworkAddress.
  local_info.data = TAO::DCPS::TransportInterfaceBLOB
                                    (sizeof(NetworkAddress),
                                     sizeof(NetworkAddress),
                                     (CORBA::Octet*)(&network_order_address));

  return 0;
}


#if !defined (DDS_HAS_MINIMUM_BIT)
void 
TAO::DCPS::SimpleMcastTransport::set_bit_data (TransportBuiltinTopicData & data) const
{
  data.transport_id[2] = this->config_->transport_id_;
  data.transport_type = CORBA::string_dup (this->config_->transport_type_.c_str ());
  ACE_TCHAR buf [30]; 
  if (this->multicast_group_address_.addr_to_string (buf, 30) == -1)
  {
    ACE_ERROR((LM_ERROR, "(%P|%t)ERROR: ReliableMulticastTransportImpl::set_bit_data %p\n", 
                         "addr_to_string"));
  }
  else
  {
    data.endpoint = CORBA::string_dup (buf);
  }
}
#endif

