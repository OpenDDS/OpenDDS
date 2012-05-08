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

namespace OpenDDS {
namespace DCPS {

class ShmemDataLink;

class OpenDDS_Shmem_Export ShmemSendStrategy
  : public TransportSendStrategy {
public:
  explicit ShmemSendStrategy(ShmemDataLink* link);

  virtual void stop_i();

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

private:
  ShmemDataLink* link_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_SHMEMSENDSTRATEGY_H */
