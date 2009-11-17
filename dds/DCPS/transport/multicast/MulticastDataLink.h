/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTDATALINK_H
#define DCPS_MULTICASTDATALINK_H

#include "Multicast_Export.h"

#include "MulticastConfiguration.h"
#include "MulticastConfiguration_rch.h"
#include "MulticastSendStrategy_rch.h"
#include "MulticastReceiveStrategy_rch.h"

#include "ace/SOCK_Dgram_Mcast.h"

#include "dds/DCPS/transport/framework/DataLink.h"

namespace OpenDDS {
namespace DCPS {

class MulticastTransport;

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  MulticastDataLink(MulticastTransport* impl,
                    CORBA::Long priority,
                    long local_peer,
                    long remote_peer);

  bool join(const ACE_INET_Addr& group_address, bool active);

  MulticastConfiguration* get_configuration();

  long get_local_peer() const;
  long get_remote_peer() const;

  ACE_SOCK_Dgram_Mcast& get_socket();

protected:
  virtual void stop_i();

private:
  MulticastConfiguration_rch config_;

  long local_peer_;
  long remote_peer_;

  MulticastSendStrategy_rch send_strategy_;
  MulticastReceiveStrategy_rch recv_strategy_;

  ACE_INET_Addr group_address_;
  ACE_SOCK_Dgram_Mcast socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTDATALINK_H */
