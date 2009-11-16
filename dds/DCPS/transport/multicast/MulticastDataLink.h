/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastTransport.h"
#include "MulticastConfiguration.h"

#include "ace/SOCK_Dgram_Mcast.h"

#include "dds/DCPS/transport/framework/DataLink.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTDATALINK_H
#define DCPS_MULTICASTDATALINK_H

namespace OpenDDS {
namespace DCPS {

class MulticastSendStrategy;
class MulticastReceiveStrategy;

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  MulticastDataLink(MulticastTransport* impl,
                    CORBA::Long priority,
                    long local_peer,
                    long remote_peer);
  ~MulticastDataLink();

  bool join(const ACE_INET_Addr& group_address, bool active);
  void leave();

  TransportConfiguration* get_configuration();

  long get_local_peer() const;
  long get_remote_peer() const;

  ACE_SOCK_Dgram_Mcast& get_socket();

protected:
  virtual void stop_i();

private:
  MulticastConfiguration* config_;

  long local_peer_;
  long remote_peer_;

  MulticastSendStrategy* send_strategy_;
  MulticastReceiveStrategy* recv_strategy_;

  ACE_SOCK_Dgram_Mcast socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTDATALINK_H */
