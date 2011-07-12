/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H
#define OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H

#include "dds/DCPS/dcps_export.h"

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
  virtual void notify_connection_deleted() = 0;

  virtual void remove_associations(const WriterIdSeq& pubids, bool notify) = 0;

protected:

  TransportReceiveListener();
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TransportReceiveListener.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORTRECEIVELISTENER_H */
