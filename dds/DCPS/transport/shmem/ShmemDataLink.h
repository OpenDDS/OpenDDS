/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SHMEMDATALINK_H
#define OPENDDS_SHMEMDATALINK_H

#include "Shmem_Export.h"

#include "ShmemSendStrategy.h"
#include "ShmemSendStrategy_rch.h"
#include "ShmemReceiveStrategy.h"
#include "ShmemReceiveStrategy_rch.h"

#include "dds/DCPS/transport/framework/DataLink.h"

#include <string>

namespace OpenDDS {
namespace DCPS {

class ShmemInst;
class ShmemTransport;
class ReceivedDataSample;

class OpenDDS_Shmem_Export ShmemDataLink
  : public DataLink {
public:
  explicit ShmemDataLink(ShmemTransport* transport);

  void configure(ShmemInst* config);

  void send_strategy(ShmemSendStrategy* send_strategy);
  void receive_strategy(ShmemReceiveStrategy* recv_strategy);

  ShmemInst* config();

  bool open(const std::string& remote_address);

  void control_received(ReceivedDataSample& sample);

  std::string remote_address();

protected:
  ShmemInst* config_;

  ShmemSendStrategy_rch send_strategy_;
  ShmemReceiveStrategy_rch recv_strategy_;

  virtual void stop_i();

private:
  std::string remote_address_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* OPENDDS_SHMEMDATALINK_H */
