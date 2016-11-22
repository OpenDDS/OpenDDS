/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTREPLACEDELEMENT_H
#define OPENDDS_DCPS_TRANSPORTREPLACEDELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportQueueElement.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportReplacedElement;

typedef Cached_Allocator_With_Overflow<TransportReplacedElement,
                                       ACE_SYNCH_NULL_MUTEX>
  TransportReplacedElementAllocator;

class OpenDDS_Dcps_Export TransportReplacedElement
  : public TransportQueueElement {
public:

  TransportReplacedElement(TransportQueueElement* orig_elem,
                           TransportReplacedElementAllocator* allocator = 0,
                           MessageBlockAllocator* mb_allocator = 0,
                           DataBlockAllocator* db_allocator = 0);
  virtual ~TransportReplacedElement();

  /// Accessor for the publisher id.
  virtual RepoId publication_id() const;

  /// Accessor for the ACE_Message_Block
  virtual const ACE_Message_Block* msg() const;

  virtual const ACE_Message_Block* msg_payload() const;

  virtual bool owned_by_transport();

protected:

  virtual void release_element(bool dropped_by_transport);

private:

  /// Reference to TransportReplacedElement allocator.
  TransportReplacedElementAllocator* allocator_;
  /// Cached allocator for DataSampleHeader message block
  MessageBlockAllocator* mb_allocator_;
  /// Cached allocator for DataSampleHeader data block
  DataBlockAllocator* db_allocator_;

  /// The publication_id() from the original TransportQueueElement
  RepoId publisher_id_;

  /// A deep-copy of the msg() from the original TransportQueueElement.
  ACE_Message_Block* msg_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportReplacedElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTREPLACEDELEMENT_H */
