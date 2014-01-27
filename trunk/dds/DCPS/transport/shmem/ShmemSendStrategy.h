/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMSENDSTRATEGY_H
#define OPENDDS_SHMEMSENDSTRATEGY_H

#include "Shmem_Export.h"

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

#include "ace/OS_NS_Thread.h"

#include <string>

namespace OpenDDS {
namespace DCPS {

class ShmemDataLink;
struct ShmemData;

class OpenDDS_Shmem_Export ShmemSendStrategy
  : public TransportSendStrategy {
public:
  explicit ShmemSendStrategy(ShmemDataLink* link);

  virtual bool start_i();
  virtual void stop_i();

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

private:
  ShmemDataLink* link_;
  std::string bound_name_;
  ACE_sema_t peer_semaphore_;
  ShmemData* current_data_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_SHMEMSENDSTRATEGY_H */
