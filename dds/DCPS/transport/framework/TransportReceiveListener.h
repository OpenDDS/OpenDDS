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
#include "dds/DCPS/RcObject.h"
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;
class WriterIdSeq;

class OpenDDS_Dcps_Export TransportReceiveListener
  : public virtual RcObject {
public:

  virtual ~TransportReceiveListener();

  virtual void data_received(const ReceivedDataSample& sample) = 0;

  virtual void notify_subscription_disconnected(const WriterIdSeq& pubids) = 0;
  virtual void notify_subscription_reconnected(const WriterIdSeq& pubids) = 0;
  virtual void notify_subscription_lost(const WriterIdSeq& pubids) = 0;

  virtual void remove_associations(const WriterIdSeq& pubids, bool notify) = 0;

  virtual void transport_discovery_change() {}

protected:

  TransportReceiveListener();
};

typedef RcHandle<TransportReceiveListener> TransportReceiveListener_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H */
