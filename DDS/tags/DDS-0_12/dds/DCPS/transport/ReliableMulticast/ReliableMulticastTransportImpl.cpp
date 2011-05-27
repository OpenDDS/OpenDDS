// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportImpl.h"
#include "ReliableMulticastTransportConfiguration.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"

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
  TAO::DCPS::ReliableMulticastDataLink_rch data_link;
  const TransportInterfaceData& transport_interface_data =
    reinterpret_cast<const TransportInterfaceData&>(*(remote_info.data.get_buffer()));
  ACE_INET_Addr multicast_group_address;

  transport_interface_data.multicast_group_address_.to_addr(
    multicast_group_address
    );

  ReliableMulticastDataLinkMap::iterator iter =
    data_links_.find(multicast_group_address);
  if (iter != data_links_.end())
  {
    return iter->second._retn();
  }

  data_link = new TAO::DCPS::ReliableMulticastDataLink(
    reactor_task_,
    *configuration_,
    multicast_group_address,
    *this
    );
  data_links_[multicast_group_address] = data_link;
  
  if (!data_link->connect(connect_as_publisher == 1))
  {
    ACE_ERROR_RETURN(
      (LM_ERROR, "(%P|%t) ERROR: Failed to connect data link.\n"),
      0
      );
  }

  return data_link._retn();
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
      -1
      );
  }

  reactor_task_ = reactor_task();
  if (reactor_task_.is_nil())
  {
    ACE_ERROR_RETURN(
      (LM_ERROR, "(%P|%t) ERROR: This transport requires a reactor.\n"),
      -1
      );
  }

  my_config->_add_ref();
  configuration_ = my_config;

  return 0;
}

void
TAO::DCPS::ReliableMulticastTransportImpl::shutdown_i()
{
  for (
    ReliableMulticastDataLinkMap::iterator iter = data_links_.begin();
    iter != data_links_.end();
    ++iter
    )
  {
    iter->second->transport_shutdown();
  }
  data_links_.clear();
  reactor_task_ = 0;
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
  TAO::DCPS::ReliableMulticastDataLink* data_link =
    dynamic_cast<TAO::DCPS::ReliableMulticastDataLink*>(link);

  if (data_link == 0)
  {
    ACE_ERROR(
      (LM_ERROR, "(%P|%t) ERROR: Framework requested removal of a data link we didn't create.\n")
      );
    return;
  }

  for (
    ReliableMulticastDataLinkMap::iterator iter = data_links_.begin();
    iter != data_links_.end();
    ++iter
    )
  {
    if (iter->second.in() == data_link)
    {
      data_links_.erase(iter);
      return;
    }
  }
}
