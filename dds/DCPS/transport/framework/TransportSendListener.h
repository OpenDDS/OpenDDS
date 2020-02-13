/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTSENDLISTENER_H
#define OPENDDS_DCPS_TRANSPORTSENDLISTENER_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcHandle_T.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/Message_Block_Ptr.h"
#include "TransportDefs.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/SequenceNumber.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataSampleElement;
struct DataSampleHeader;
typedef ACE_Message_Block DataSample;

class DataLinkSet;
typedef RcHandle<DataLinkSet> DataLinkSet_rch;

class ReaderIdSeq;

class OpenDDS_Dcps_Export TransportSendListener
  : public virtual RcObject {
public:

  virtual ~TransportSendListener();

  virtual void data_delivered(const DataSampleElement* sample);
  virtual void data_dropped(const DataSampleElement* sample,
                            bool dropped_by_transport);

  virtual void control_delivered(const Message_Block_Ptr& sample);
  virtual void control_dropped(const Message_Block_Ptr& sample,
                               bool dropped_by_transport);

  virtual void notify_publication_disconnected(const ReaderIdSeq& subids) = 0;
  virtual void notify_publication_reconnected(const ReaderIdSeq& subids) = 0;
  virtual void notify_publication_lost(const ReaderIdSeq& subids) = 0;

  virtual void remove_associations(const ReaderIdSeq& subids, bool notify) = 0;

  /// Hook for the listener to override a normal control message with
  /// customized messages to different DataLinks.
  virtual SendControlStatus send_control_customized(
    const DataLinkSet_rch& links,
    const DataSampleHeader& header,
    ACE_Message_Block* msg,
    void* extra);

  struct InlineQosData {
    DDS::PublisherQos  pub_qos;
    DDS::DataWriterQos dw_qos;
    OPENDDS_STRING     topic_name;
  };

  virtual void retrieve_inline_qos_data(InlineQosData& qos_data) const;

  virtual void transport_discovery_change() {}

protected:

  TransportSendListener();
};

typedef RcHandle<TransportSendListener> TransportSendListener_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORTSENDLISTENER_H */
