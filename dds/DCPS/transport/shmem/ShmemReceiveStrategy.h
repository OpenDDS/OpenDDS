/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMRECEIVESTRATEGY_H
#define OPENDDS_SHMEMRECEIVESTRATEGY_H

#include "Shmem_Export.h"

#include "ace/Event_Handler.h"
#include "ace/INET_Addr.h"

#include "dds/DCPS/transport/framework/TransportReceiveStrategy_T.h"
#include <map>

namespace OpenDDS {
namespace DCPS {

class ShmemDataLink;
struct ShmemData;

class OpenDDS_Shmem_Export ShmemReceiveStrategy
  : public TransportReceiveStrategy<> {
public:
  explicit ShmemReceiveStrategy(ShmemDataLink* link);

  void read();

protected:
  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address,
                                ACE_HANDLE fd);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();
  virtual void stop_i();

private:
  ShmemDataLink* link_;
  SequenceNumber expected_;
  std::string bound_name_;
  ShmemData* current_data_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_SHMEMRECEIVESTRATEGY_H */
