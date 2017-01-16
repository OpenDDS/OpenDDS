/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SENDREPONSELISTENER_H
#define OPENDDS_DCPS_SENDREPONSELISTENER_H

#include "dds/DCPS/dcps_export.h"
#include "TransportSendListener.h"
#include "dds/DCPS/MessageTracker.h"
#include "dds/DCPS/PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class SendResponseListener
 *
 * @brief Simple listener to discard response samples.
 *
 * This is a simple listener implementation used to release response
 * samples once they have been either delivered or dropped.  No
 * special actions are taken to distinguish between the two results.
 */
class OpenDDS_Dcps_Export SendResponseListener
  : public TransportSendListener {
public:
  explicit SendResponseListener(const OPENDDS_STRING& msg_src);
  virtual ~SendResponseListener();

  virtual void data_delivered(const DataSampleElement* sample);
  virtual void data_dropped(const DataSampleElement* sample,
                            bool dropped_by_transport);

  virtual void control_delivered(ACE_Message_Block* sample);
  virtual void control_dropped(ACE_Message_Block* sample,
                               bool dropped_by_transport);

  void notify_publication_disconnected(const ReaderIdSeq&) {}
  void notify_publication_reconnected(const ReaderIdSeq&) {}
  void notify_publication_lost(const ReaderIdSeq&) {}
  void notify_connection_deleted(const RepoId&) {}
  void remove_associations(const ReaderIdSeq&, bool) {}

  void track_message();

private:
  void _add_ref() {}
  void _remove_ref() {}

  MessageTracker tracker_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_SENDRESPONSELISTENER_H */
