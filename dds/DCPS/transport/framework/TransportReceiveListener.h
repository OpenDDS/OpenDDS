/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H
#define OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;
class WriterIdSeq;

class OpenDDS_Dcps_Export TransportReceiveListener {
public:

  virtual ~TransportReceiveListener();

  virtual void data_received(const ReceivedDataSample& sample) = 0;

  virtual void notify_subscription_disconnected(const WriterIdSeq& pubids) = 0;
  virtual void notify_subscription_reconnected(const WriterIdSeq& pubids) = 0;
  virtual void notify_subscription_lost(const WriterIdSeq& pubids) = 0;
  virtual void notify_connection_deleted(const RepoId& peerId) = 0;

  virtual void remove_associations(const WriterIdSeq& pubids, bool notify) = 0;

  virtual void _add_ref() = 0;
  virtual void _remove_ref() = 0;

protected:

  TransportReceiveListener();
};

typedef RcHandle<TransportReceiveListener> TransportReceiveListener_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H */
