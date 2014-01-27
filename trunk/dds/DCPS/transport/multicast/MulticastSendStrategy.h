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
  : public TransportSendStrategy
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  , public ACE_Handler {
#else
  {
#endif
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

#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  // Callback from async send.
  virtual void handle_write_dgram(const ACE_Asynch_Write_Dgram::Result& res);
#endif

private:
  MulticastDataLink* link_;

#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  ACE_Asynch_Write_Dgram async_writer_;
  bool async_init_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_MULTICASTSENDSTRATEGY_H */
