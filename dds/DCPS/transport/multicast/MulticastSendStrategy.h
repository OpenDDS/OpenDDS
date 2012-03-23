/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTSENDSTRATEGY_H
#define DCPS_MULTICASTSENDSTRATEGY_H

#include "Multicast_Export.h"

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include "ace/Asynch_IO.h"

namespace OpenDDS {
namespace DCPS {

class MulticastDataLink;

class OpenDDS_Multicast_Export MulticastSendStrategy
  : public TransportSendStrategy, public ACE_Handler {
public:
  explicit MulticastSendStrategy(MulticastDataLink* link);

  virtual void stop_i();

protected:
  virtual void prepare_header_i();

  virtual ssize_t send_bytes_i(const iovec iov[], int n);
  ssize_t sync_send(const iovec iov[], int n);
  ssize_t async_send(const iovec iov[], int n);

  virtual size_t max_message_size() const
  {
    return UDP_MAX_MESSAGE_SIZE;
  }

  // Callback from async send.
  virtual void handle_write_dgram(const ACE_Asynch_Write_Dgram::Result& res);

private:
  MulticastDataLink* link_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_MULTICASTSENDSTRATEGY_H */
