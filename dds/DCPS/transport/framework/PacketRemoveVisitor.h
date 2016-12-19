/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PACKETREMOVEVISTOR_H
#define OPENDDS_DCPS_PACKETREMOVEVISTOR_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/GuidUtils.h"
#include "BasicQueueVisitor_T.h"
#include "TransportDefs.h"
#include "TransportReplacedElement.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export PacketRemoveVisitor
  : public BasicQueueVisitor<TransportQueueElement> {
public:

  PacketRemoveVisitor(const TransportQueueElement::MatchCriteria& match,
                      ACE_Message_Block*&          unsent_head_block,
                      ACE_Message_Block*           header_block,
                      TransportReplacedElementAllocator& allocator,
                      MessageBlockAllocator& mb_allocator,
                      DataBlockAllocator& db_allocator);

  virtual ~PacketRemoveVisitor();

  /// The BasicQueue<T>::accept_remove_visitor() method will call
  /// this visit_element() method for each element in the queue.
  virtual int visit_element_ref(TransportQueueElement*& element);

  RemoveResult status() const;

private:

  /// The sample that needs to be removed.
  const TransportQueueElement::MatchCriteria& match_;

  /// The head block of the chain of unsent blocks in the packet.
  ACE_Message_Block*& head_;

  /// The packet header block that was duplicate()'d to form the
  /// first block in the packet.
  ACE_Message_Block* header_block_;

  /// Holds the status of our visit.
  RemoveResult status_;

  /// This is the message block in the chain that corresponds to the
  /// current (non-head) element being visited.
  ACE_Message_Block* current_block_;

  /// This is the message block in the chain that has its cont() set
  /// to the current_block_.
  ACE_Message_Block* previous_block_;

  /// Cached allocator for TransportReplaceElement.
  TransportReplacedElementAllocator& replaced_element_allocator_;
  /// Cached allocator for DataSampleHeader message block
  MessageBlockAllocator& replaced_element_mb_allocator_;
  /// Cached allocator for DataSampleHeader data block
  DataBlockAllocator& replaced_element_db_allocator_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "PacketRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_PACKETREMOVEVISTOR_H */
