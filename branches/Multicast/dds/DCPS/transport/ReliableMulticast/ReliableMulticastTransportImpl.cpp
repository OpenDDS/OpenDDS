/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportImpl.h"
#include "ReliableMulticastTransportConfiguration.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/DirectPriorityMapper.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImpl.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::DataLink*
OpenDDS::DCPS::ReliableMulticastTransportImpl::find_or_create_datalink(
  RepoId                  /*local_id*/,
  const AssociationData*  remote_association,
  CORBA::Long             priority,
  bool                    active)
{
  const TransportInterfaceInfo& remote_info = remote_association->remote_data_;

  // Get the remote address from the "blob" in the remote_info struct.
  NetworkAddress network_order_address;

  ACE_InputCDR cdr((const char*)remote_info.data.get_buffer(), remote_info.data.length());

  if (cdr >> network_order_address == 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t)ReliableMulticastTransportImpl::find_or_create_datalink failed "
               "to de-serialize the NetworkAddress\n"));
    return 0;
  }

  ACE_INET_Addr multicast_group_address;

  network_order_address.to_addr(multicast_group_address);

  VDBG_LVL((LM_DEBUG, "(%P|%t)ReliableMulticastTransportImpl::find_or_create_datalink remote addr str \"%s\"\n",
            network_order_address.addr_.c_str()), 2);

  OpenDDS::DCPS::ReliableMulticastDataLink_rch data_link;

  ReliableMulticastDataLinkMap::iterator iter =
    data_links_.find(PriorityKey(priority, multicast_group_address));

  if (iter != data_links_.end()) {
    data_link = iter->second;
    return data_link._retn();
  }

  /// @TODO: The thread can be conditioned with priority at this point.

  data_link = new OpenDDS::DCPS::ReliableMulticastDataLink(
    reactor_task_,
    *(configuration_.in()),
    multicast_group_address,
    *this,
    priority);
  data_links_[ PriorityKey(priority, multicast_group_address)] = data_link;

  if (!data_link->connect(active)) {
    ACE_ERROR_RETURN(
      (LM_ERROR, "(%P|%t) ERROR: Failed to connect data link.\n"),
      0);
  }

  //
  // We only set the DiffServ codepoint on the sending side.  Since this
  // Transport implementation distinguishes between the sending and
  // receiving roles at this level, we only set on the publication end of
  // connections.
  //
  if (active) {
    //
    // Set the DiffServ codepoint according to the TRANSPORT_PRIORITY
    // policy value.  We need to do this *after* the call to connect as
    // that is where the underlying socket is actually created and
    // initialized.
    //
    DirectPriorityMapper mapping(priority);
    data_link->set_dscp_codepoint(mapping.codepoint(), data_link->socket());
  }

  return data_link._retn();
}

int
OpenDDS::DCPS::ReliableMulticastTransportImpl::configure_i(TransportConfiguration* config)
{
  ReliableMulticastTransportConfiguration* my_config =
    dynamic_cast<ReliableMulticastTransportConfiguration*>(config);

  if (my_config == 0) {
    ACE_ERROR_RETURN(
      (LM_ERROR, "(%P|%t) ERROR: Failed downcast from TransportConfiguration to ReliableMulticastTransportConfiguration.\n"),
      -1);
  }

  reactor_task_ = reactor_task();

  if (reactor_task_.is_nil()) {
    ACE_ERROR_RETURN(
      (LM_ERROR, "(%P|%t) ERROR: This transport requires a reactor.\n"),
      -1);
  }

  my_config->_add_ref();
  configuration_ = my_config;

  return 0;
}

void
OpenDDS::DCPS::ReliableMulticastTransportImpl::shutdown_i()
{
  for (
    ReliableMulticastDataLinkMap::iterator iter = data_links_.begin();
    iter != data_links_.end();
    ++iter) {
    iter->second->transport_shutdown();
  }

  data_links_.clear();
  reactor_task_ = 0;
}

int
OpenDDS::DCPS::ReliableMulticastTransportImpl::connection_info_i(TransportInterfaceInfo& local_info) const
{
  NetworkAddress network_order_address(configuration_->multicast_group_address_str_);

  VDBG_LVL((LM_DEBUG, "(%P|%t)ReliableMulticastTransportImpl::connection_info_i %s\n",
            configuration_->multicast_group_address_str_.c_str()), 2);

  ACE_OutputCDR cdr;
  cdr << network_order_address;
  size_t len = cdr.total_length();

  // Allow DCPSInfo to check compatibility of transport implemenations.
  local_info.transport_id = 999; // TBD Change magic number into a enum or constant value.
  local_info.data = OpenDDS::DCPS::TransportInterfaceBLOB
                    (len,
                     len,
                     (CORBA::Octet*)(cdr.buffer()));

  return 0;

}

void
OpenDDS::DCPS::ReliableMulticastTransportImpl::release_datalink_i(
  DataLink* link, bool release_pending)
{
  ACE_UNUSED_ARG(release_pending);

  OpenDDS::DCPS::ReliableMulticastDataLink* data_link =
    dynamic_cast<OpenDDS::DCPS::ReliableMulticastDataLink*>(link);

  if (data_link == 0) {
    ACE_ERROR(
      (LM_ERROR, "(%P|%t) ERROR: Framework requested removal of a data link we didn't create.\n"));
    return;
  }

  for (
    ReliableMulticastDataLinkMap::iterator iter = data_links_.begin();
    iter != data_links_.end();
    ++iter) {
    if (iter->second.in() == data_link) {
      data_links_.erase(iter);
      return;
    }
  }
}
