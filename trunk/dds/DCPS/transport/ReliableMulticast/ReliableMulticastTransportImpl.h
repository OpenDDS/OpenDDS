/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORT_H
#define OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORT_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "ReliableMulticastTransportConfiguration.h"
#include "ReliableMulticastDataLink.h"
#include "ReliableMulticastRcHandles.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include "dds/DCPS/transport/framework/PriorityKey.h"
#include <map>

namespace OpenDDS {
namespace DCPS {

class ReliableMulticastTransportConfiguration;

class ReliableMulticast_Export ReliableMulticastTransportImpl
  : public TransportImpl {
public:
  ReliableMulticastTransportImpl();
  virtual ~ReliableMulticastTransportImpl();

protected:
  virtual OpenDDS::DCPS::DataLink* find_or_create_datalink(
    const TransportInterfaceInfo& remote_info,
    int connect_as_publisher,
    int priority);

  virtual int configure_i(TransportConfiguration* config);

  virtual void shutdown_i();

  virtual int connection_info_i(TransportInterfaceInfo& local_info) const;

  virtual void release_datalink_i(OpenDDS::DCPS::DataLink* link,
                                  bool release_pending);

  virtual bool acked(RepoId, RepoId);
  virtual void remove_ack(RepoId pub_id, RepoId sub_id);

private:
  ReliableMulticastTransportConfiguration_rch configuration_;
  // JSP: Add transport configuration storage
  typedef std::map<
  PriorityKey,
  OpenDDS::DCPS::ReliableMulticastDataLink_rch
  > ReliableMulticastDataLinkMap;
  ReliableMulticastDataLinkMap data_links_;
  TransportReactorTask_rch reactor_task_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "ReliableMulticastTransportImpl.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICASTTRANSPORT_H */
