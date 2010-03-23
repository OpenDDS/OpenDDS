/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTRECEIVESTRATEGY_H
#define DCPS_MULTICASTRECEIVESTRATEGY_H

#include "Multicast_Export.h"

#include "ace/Event_Handler.h"

#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"

namespace OpenDDS {
namespace DCPS {

class MulticastDataLink;

class OpenDDS_Multicast_Export MulticastReceiveStrategy
  : public TransportReceiveStrategy,
    public ACE_Event_Handler {
public:
  explicit MulticastReceiveStrategy(MulticastDataLink* link);

  virtual ACE_HANDLE get_handle() const;
  virtual int handle_input(ACE_HANDLE fd);

protected:
  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address);

  virtual bool check_header(const TransportHeader& header);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();
  virtual void stop_i();

private:
  MulticastDataLink* link_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTRECEIVESTRATEGY_H */
