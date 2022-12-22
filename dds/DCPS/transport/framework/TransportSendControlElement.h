/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTSENDCONTROLELEMENT_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTSENDCONTROLELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "TransportDefs.h"
#include "TransportQueueElement.h"
#include "ace/Synch_Traits.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportSendListener;
class TransportSendControlElement;
class DataSampleElement;

typedef Cached_Allocator_With_Overflow<TransportSendControlElement, ACE_SYNCH_NULL_MUTEX>
TransportSendControlElementAllocator;

class OpenDDS_Dcps_Export TransportSendControlElement : public TransportQueueElement {
public:

  TransportSendControlElement(int initial_count,
                              const RepoId& publisher_id,
                              TransportSendListener* listener,
                              const DataSampleHeader& header,
                              Message_Block_Ptr msg_block);

  TransportSendControlElement(int initial_count,
                              const DataSampleElement* dcps_elem);

  virtual ~TransportSendControlElement();

  /// Overridden to always return true for Send Control elements.
  virtual bool requires_exclusive_packet() const;

  /// Accessor for the publisher id.
  virtual RepoId publication_id() const;

  virtual ACE_Message_Block* duplicate_msg() const;

  /// Accessor for the ACE_Message_Block
  virtual const ACE_Message_Block* msg() const;

  const DataSampleHeader& header() const { return header_; }
  // Only allow const access to the header.  Modifying the header
  // would require remarshaling.
  //DataSampleHeader& header() { return header_; }

  const TransportSendListener* listener() const { return listener_; }

  virtual const ACE_Message_Block* msg_payload() const;

  virtual SequenceNumber sequence() const;

  /// Is the element a "control" sample from the specified pub_id?
  virtual bool is_control(RepoId pub_id) const;
  virtual bool owned_by_transport ();

  virtual bool is_request_ack() const { return header_.message_id_ == REQUEST_ACK; }

  virtual bool is_last_fragment() const { return !header_.more_fragments(); }

protected:

  virtual void release_element(bool dropped_by_transport);

private:

  /// The publisher of the control message
  RepoId publisher_id_;

  /// The TransportSendListener object to call back upon.
  TransportSendListener* listener_;

  /// The OpenDDS DCPS header for this control message
  DataSampleHeader header_;

  /// The control message.
  Message_Block_Ptr msg_;

  /// If constructed from a DataSampleElement, keep it around for release
  const DataSampleElement* const dcps_elem_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportSendControlElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTSENDCONTROLELEMENT_H */
