// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportImpl.h"
#include "ReliableMulticastTransportConfiguration.h"
#include "ReliableMulticastDataLink.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImpl.inl"
#endif /* __ACE_INLINE__ */

namespace
{
  struct TransportInterfaceData
  {
    TransportInterfaceData(const ACE_INET_Addr& address)
      : version_(1)
      , multicast_group_address_(address)
    {
    }

    char version_;
    TAO::DCPS::NetworkAddress multicast_group_address_;
  };
}

TAO::DCPS::DataLink*
TAO::DCPS::ReliableMulticastTransportImpl::find_or_create_datalink(
  const TransportInterfaceInfo& remote_info,
  int connect_as_publisher
  )
{
  TAO::DCPS::ReliableMulticastDataLink* data_link = 0;
  return data_link;
}

int
TAO::DCPS::ReliableMulticastTransportImpl::configure_i(TransportConfiguration* config)
{
  ReliableMulticastTransportConfiguration* my_config =
    dynamic_cast<ReliableMulticastTransportConfiguration*>(config);

  if (my_config == 0)
  {
    ACE_ERROR_RETURN(
      (LM_ERROR, "(%P|%t) ERROR: Failed downcast from TransportConfiguration to ReliableMulticastTransportConfiguration.\n"),
      -1);
  }

  my_config->_add_ref();
  configuration_ = my_config;
  
  return 0;
}

void
TAO::DCPS::ReliableMulticastTransportImpl::shutdown_i()
{
}

int
TAO::DCPS::ReliableMulticastTransportImpl::connection_info_i(TransportInterfaceInfo& local_info) const
{
  TransportInterfaceData transport_interface_data(configuration_->multicast_group_address_);

  local_info.transport_id = 999;
  local_info.data = TAO::DCPS::TransportInterfaceBLOB(
    sizeof(transport_interface_data),
    sizeof(transport_interface_data),
    reinterpret_cast<CORBA::Octet*>(&transport_interface_data)
    );
  return 0;
}

void
TAO::DCPS::ReliableMulticastTransportImpl::release_datalink_i(DataLink* link)
{
}
